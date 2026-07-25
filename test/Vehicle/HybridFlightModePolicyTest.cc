#include "HybridFlightModePolicyTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtTest/QSignalSpy>

#include "FirmwarePlugin.h"
#include "HybridTransitionController.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "px4_custom_mode.h"

namespace {

const QSet<uint32_t> kQuadModes{
    PX4CustomMode::MANUAL,       PX4CustomMode::STABILIZED, PX4CustomMode::ACRO,          PX4CustomMode::RATTITUDE,
    PX4CustomMode::ALTCTL,       PX4CustomMode::OFFBOARD,   PX4CustomMode::POSCTL_POSCTL, PX4CustomMode::AUTO_LOITER,
    PX4CustomMode::AUTO_MISSION, PX4CustomMode::AUTO_RTL,   PX4CustomMode::AUTO_PRECLAND,
};

const QSet<uint32_t> kRoverModes{
    PX4CustomMode::MANUAL,        PX4CustomMode::STABILIZED,  PX4CustomMode::ACRO,         PX4CustomMode::OFFBOARD,
    PX4CustomMode::POSCTL_POSCTL, PX4CustomMode::AUTO_LOITER, PX4CustomMode::AUTO_MISSION, PX4CustomMode::AUTO_RTL,
};

QSet<uint32_t> modeIdentities(Vehicle* vehicle, const QStringList& modes)
{
    QSet<uint32_t> identities;
    for (const QString& mode : modes) {
        uint8_t baseMode = 0;
        uint32_t customMode = 0;
        if (vehicle->firmwarePlugin()->setFlightMode(mode, &baseMode, &customMode)) {
            identities.insert(customMode);
        }
    }
    return identities;
}

}  // namespace

void HybridFlightModePolicyTest::init()
{
    VehicleTestManualConnect::init();
    _connectQuadRover();
    _statusTimestamp = 100;
    _primeStableState(HybridVehicleState::Quad);
    const QStringList modes = vehicle()->flightModes();
    for (const QString& mode : modes) {
        uint8_t baseMode = 0;
        uint32_t customMode = 0;
        if (vehicle()->firmwarePlugin()->setFlightMode(mode, &baseMode, &customMode) &&
            (customMode == PX4CustomMode::MANUAL)) {
            _manualMode = mode;
            break;
        }
    }
    QVERIFY(!_manualMode.isEmpty());
}

void HybridFlightModePolicyTest::_stableQuadFiltersToMultirotorModes()
{
    const QStringList modes = vehicle()->flightModes();
    QCOMPARE(modeIdentities(vehicle(), modes), kQuadModes);
}

void HybridFlightModePolicyTest::_stableRoverFiltersToRoverModes()
{
    _primeStableState(HybridVehicleState::Rover);

    const QStringList modes = vehicle()->flightModes();
    QCOMPARE(modeIdentities(vehicle(), modes), kRoverModes);
}

void HybridFlightModePolicyTest::_unavailableStatesRejectModesAndCommands_data()
{
    QTest::addColumn<HybridVehicleState::CurrentState>("currentState");
    QTest::addColumn<HybridVehicleState::TargetState>("targetState");
    QTest::addColumn<uint8_t>("faultReason");
    QTest::addColumn<bool>("superseded");

    QTest::newRow("transitioning") << HybridVehicleState::Transitioning << HybridVehicleState::TargetRover
                                   << static_cast<uint8_t>(HYBRID_VEHICLE_FAULT_NONE) << false;
    QTest::newRow("unknown") << HybridVehicleState::Unknown << HybridVehicleState::None
                             << static_cast<uint8_t>(HYBRID_VEHICLE_FAULT_NONE) << false;
    QTest::newRow("fault") << HybridVehicleState::TransitionFault << HybridVehicleState::TargetRover
                           << static_cast<uint8_t>(HYBRID_VEHICLE_FAULT_ACTUATOR_COMMUNICATION) << false;
    QTest::newRow("superseded") << HybridVehicleState::Quad << HybridVehicleState::None
                                << static_cast<uint8_t>(HYBRID_VEHICLE_FAULT_NONE) << true;
}

void HybridFlightModePolicyTest::_unavailableStatesRejectModesAndCommands()
{
    QFETCH(HybridVehicleState::CurrentState, currentState);
    QFETCH(HybridVehicleState::TargetState, targetState);
    QFETCH(uint8_t, faultReason);
    QFETCH(bool, superseded);

    if (superseded) {
        mockLink()->setHoldHybridTransitionAcks(true);
        const auto restoreHybridTransitionAcks =
            qScopeGuard([this]() { mockLink()->setHoldHybridTransitionAcks(false); });
        QVERIFY(vehicle()->hybridTransitionController()->requestTransform(HybridVehicleState::TargetRover));
        _injectAck(MAV_RESULT_IN_PROGRESS, 41);
        QTRY_COMPARE(vehicle()->hybridTransitionController()->transactionState(), HybridTransitionController::Detached);
        _injectStatus(HybridVehicleState::Transitioning, HybridVehicleState::TargetRover, HYBRID_VEHICLE_FAULT_NONE, 0,
                      42, MAV_RESULT_IN_PROGRESS, 410);
        QTRY_COMPARE(vehicle()->hybridTransitionController()->transactionState(),
                     HybridTransitionController::SupersededUnconfirmed);
    } else {
        _injectStatus(currentState, targetState, faultReason);
    }
    QCOMPARE(vehicle()->flightModes(), QStringList());
    _verifyUnavailableModeRequestIsNotSent();
}

void HybridFlightModePolicyTest::_staleStateRejectsModesAndCommands()
{
    QTest::qWait(3100);

    QVERIFY(vehicle()->hybridVehicleState()->stale());
    QCOMPARE(vehicle()->flightModes(), QStringList());
    _verifyUnavailableModeRequestIsNotSent();
}

void HybridFlightModePolicyTest::_connectQuadRover()
{
    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink = MockLink::startPX4QuadRoverMockLink(false, false, false);
    QVERIFY(_mockLink);
    QVERIFY(UnitTest::waitForSignal(activeVehicleSpy, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")));
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->quadRover());
}

void HybridFlightModePolicyTest::_injectAck(MAV_RESULT result, uint32_t sequence)
{
    mavlink_message_t message{};
    mavlink_msg_command_ack_pack_chan(vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, mockLink()->mavlinkChannel(), &message,
                                      MAV_CMD_DO_HYBRID_TRANSITION, result, 0, sequence,
                                      MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::getComponentId());
    mockLink()->respondWithMavlinkMessage(message);
}

void HybridFlightModePolicyTest::_injectStatus(HybridVehicleState::CurrentState currentState,
                                               HybridVehicleState::TargetState targetState, uint8_t faultReason,
                                               uint64_t timestamp, uint32_t sequence, MAV_RESULT commandResult,
                                               uint64_t commandTimestamp)
{
    QSignalSpy statusAcceptedSpy(vehicle()->hybridVehicleState(), &HybridVehicleState::statusAccepted);
    QVERIFY(statusAcceptedSpy.isValid());

    mavlink_hybrid_vehicle_status_t status{};
    status.timestamp = timestamp ? timestamp : ++_statusTimestamp;
    status.transition_sequence = sequence;
    status.position_normalized = 0.5F;
    status.flags = HYBRID_VEHICLE_STATUS_FLAGS_LANDED | HYBRID_VEHICLE_STATUS_FLAGS_LAND_DETECTION_FRESH;
    status.current_state = static_cast<uint8_t>(currentState);
    status.target_state = static_cast<uint8_t>(targetState);
    status.fault_reason = faultReason;
    status.command_result = commandResult;
    status.command_timestamp = commandTimestamp;

    mavlink_message_t message{};
    mavlink_msg_hybrid_vehicle_status_encode_chan(vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, mockLink()->mavlinkChannel(),
                                                  &message, &status);
    mockLink()->respondWithMavlinkMessage(message);
    QVERIFY(!statusAcceptedSpy.isEmpty() ||
            UnitTest::waitForSignal(statusAcceptedSpy, TestTimeout::longMs(), QStringLiteral("statusAccepted")));
}

void HybridFlightModePolicyTest::_primeStableState(HybridVehicleState::CurrentState currentState)
{
    _injectStatus(currentState, HybridVehicleState::None);
    QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), currentState);
    QVERIFY(vehicle()->hybridVehicleState()->hasValidStatus());
}

void HybridFlightModePolicyTest::_verifyUnavailableModeRequestIsNotSent()
{
    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->setFlightMode(_manualMode);
    QTest::qWait(20);

    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_MODE), 0);
    QCOMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SET_MODE), 0);
}

UT_REGISTER_TEST(HybridFlightModePolicyTest)
