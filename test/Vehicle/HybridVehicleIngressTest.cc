#include "HybridVehicleIngressTest.h"

#include <QtTest/QSignalSpy>

#include "HybridVehicleState.h"
#include "MAVLinkLib.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleSupports.h"

namespace {

mavlink_hybrid_vehicle_status_t makeStatus(HybridVehicleState::CurrentState state, uint64_t timestamp,
                                           uint8_t faultReason = HYBRID_VEHICLE_FAULT_NONE)
{
    mavlink_hybrid_vehicle_status_t status{};
    status.timestamp = timestamp;
    status.current_state = static_cast<uint8_t>(state);
    status.target_state = HYBRID_VEHICLE_SHAPE_NONE;
    status.position_normalized = 0.25F;
    status.fault_reason = faultReason;
    return status;
}

void injectStatus(MockLink* mockLink, uint8_t systemId, uint8_t componentId,
                  const mavlink_hybrid_vehicle_status_t& status, uint8_t magic = MAVLINK_STX)
{
    mavlink_message_t message{};
    mavlink_msg_hybrid_vehicle_status_encode_chan(systemId, componentId, mockLink->mavlinkChannel(), &message, &status);
    message.magic = magic;
    mockLink->respondWithMavlinkMessage(message);
}

void injectSystemTime(MockLink* mockLink, Vehicle* vehicle, uint32_t timeBootMs)
{
    mavlink_system_time_t time{};
    time.time_boot_ms = timeBootMs;

    mavlink_message_t message{};
    mavlink_msg_system_time_encode_chan(vehicle->id(), MAV_COMP_ID_AUTOPILOT1, mockLink->mavlinkChannel(), &message,
                                        &time);
    mockLink->respondWithMavlinkMessage(message);
}

}  // namespace

void HybridVehicleIngressTest::_connectQuadRoverMockLink()
{
    QVERIFY(!_mockLink);

    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink = MockLink::startPX4QuadRoverMockLink(false, false, false);
    QVERIFY(_mockLink);
    QVERIFY(UnitTest::waitForSignal(activeVehicleSpy, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")));

    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->quadRover());
}

void HybridVehicleIngressTest::_connectPx4MockLink()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
}

void HybridVehicleIngressTest::_quadRoverFixtureExposesHybridParameters()
{
    _connectQuadRoverMockLink();
    QVERIFY(waitForParametersReady());

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    QVERIFY(parameterManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("HYBRID_TRANS_T")));
    QVERIFY(parameterManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("HYB_SENS_EN")));
    QVERIFY(parameterManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("HYB_ACT_TYPE")));
}

void HybridVehicleIngressTest::_validMavlink2StatusUpdatesVehicleState()
{
    _connectQuadRoverMockLink();

    const mavlink_hybrid_vehicle_status_t status = makeStatus(HybridVehicleState::Rover, 101);
    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, status);

    QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Rover);
}

void HybridVehicleIngressTest::_invalidSourcesAndOutOfOrderStatusAreIgnored()
{
    _connectQuadRoverMockLink();

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Rover, 101));
    QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Rover);

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_CAMERA, makeStatus(HybridVehicleState::Quad, 102));
    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_ALL, makeStatus(HybridVehicleState::Quad, 103));
    injectStatus(mockLink(), vehicle()->id() + 1, MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 104));
    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 105),
                 MAVLINK_STX_MAVLINK1);
    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 100));

    QTest::qWait(50);
    QCOMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Rover);
}

void HybridVehicleIngressTest::_systemTimeRebootResetsVehicleState()
{
    _connectQuadRoverMockLink();

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Rover, 900000));
    injectSystemTime(mockLink(), vehicle(), 5000);
    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 100));
    injectSystemTime(mockLink(), vehicle(), 20);
    QTest::qWait(300);
    QCOMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Rover);

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 101));
    injectSystemTime(mockLink(), vehicle(), 21);

    QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Unknown);
    QVERIFY(vehicle()->hybridVehicleState()->stale());

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 102));
    QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Quad);
}

void HybridVehicleIngressTest::_manualControlProfileSurvivesStatusLoss()
{
    _connectQuadRoverMockLink();
    QSignalSpy profileSpy(vehicle(), &Vehicle::manualControlProfileChanged);
    QVERIFY(profileSpy.isValid());

    QVERIFY(!vehicle()->manualControlProfileKnown());
    QVERIFY(!vehicle()->manualControlRover());
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->sendJoystickDataThreadSafe(0.F, 0.F, 0.F, 0.F, 0, 0, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN);
    QTest::qWait(50);
    QCOMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MANUAL_CONTROL), 0);

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1,
                 makeStatus(HybridVehicleState::Rover, 100, HYBRID_VEHICLE_FAULT_STALL));
    QTest::qWait(50);
    QVERIFY(!vehicle()->manualControlProfileKnown());
    QCOMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MANUAL_CONTROL), 0);

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Rover, 101));
    QTRY_VERIFY(vehicle()->manualControlProfileKnown());
    QVERIFY(vehicle()->manualControlRover());
    QVERIFY(vehicle()->supports()->negativeThrust());
    QVERIFY(profileSpy.count() >= 1);

    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->sendJoystickDataThreadSafe(0.F, 0.F, 0.F, -0.5F, 0, 0, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN);
    QTRY_COMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MANUAL_CONTROL), 1);

    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->hybridVehicleState()->stale(), 4000);
    QVERIFY(vehicle()->manualControlProfileKnown());
    QVERIFY(vehicle()->manualControlRover());
    QVERIFY(vehicle()->supports()->negativeThrust());
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->sendJoystickDataThreadSafe(0.F, 0.F, 0.F, -0.5F, 0, 0, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN);
    QTRY_COMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MANUAL_CONTROL), 1);

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1,
                 makeStatus(HybridVehicleState::TransitionFault, 102));
    QVERIFY(vehicle()->manualControlRover());
    QVERIFY(vehicle()->supports()->negativeThrust());

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Quad, 103));
    QTRY_VERIFY(!vehicle()->manualControlRover());
    QVERIFY(!vehicle()->supports()->negativeThrust());
    QVERIFY(profileSpy.count() >= 2);
}

void HybridVehicleIngressTest::_ordinaryPx4VehicleIgnoresHybridStatus()
{
    _connectPx4MockLink();

    injectStatus(mockLink(), vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, makeStatus(HybridVehicleState::Rover, 101));

    QTest::qWait(50);
    QCOMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Unknown);
}

UT_REGISTER_TEST(HybridVehicleIngressTest, TestLabel::Integration, TestLabel::Vehicle)
