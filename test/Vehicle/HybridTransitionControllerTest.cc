#include "HybridTransitionControllerTest.h"

#include <QtTest/QSignalSpy>

#include "HybridTransitionController.h"
#include "MAVLinkProtocol.h"
#include "MavCommandQueue.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void HybridTransitionControllerTest::init()
{
    VehicleTestManualConnect::init();
    _connectQuadRover();
    mockLink()->setHoldHybridTransitionAcks(true);
    _controller = vehicle()->hybridTransitionController();
    QVERIFY(_controller);
    _statusTimestamp = 100;
    _primeStableState(HybridVehicleState::Quad);
}

void HybridTransitionControllerTest::_connectQuadRover()
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

void HybridTransitionControllerTest::_injectAck(MAV_RESULT result, uint32_t sequence, uint8_t senderComponent,
                                                uint8_t targetSystem, uint8_t targetComponent)
{
    if (targetSystem == 0) {
        targetSystem = MAVLinkProtocol::instance()->getSystemId();
    }
    if (targetComponent == 0) {
        targetComponent = MAVLinkProtocol::getComponentId();
    }

    mavlink_message_t message{};
    (void)mavlink_msg_command_ack_pack_chan(vehicle()->id(), senderComponent, mockLink()->mavlinkChannel(), &message,
                                            MAV_CMD_DO_HYBRID_TRANSITION, result, 0, static_cast<int32_t>(sequence),
                                            targetSystem, targetComponent);
    mockLink()->respondWithMavlinkMessage(message);
}

void HybridTransitionControllerTest::_injectStatus(HybridVehicleState::CurrentState currentState,
                                                   HybridVehicleState::TargetState targetState, uint32_t sequence,
                                                   MAV_RESULT commandResult, uint64_t commandTimestamp,
                                                   uint8_t faultReason, uint64_t statusTimestamp,
                                                   bool waitForAcceptance)
{
    QSignalSpy statusAcceptedSpy(vehicle()->hybridVehicleState(), &HybridVehicleState::statusAccepted);
    QVERIFY(statusAcceptedSpy.isValid());
    mavlink_hybrid_vehicle_status_t status{};
    status.timestamp = statusTimestamp ? statusTimestamp : ++_statusTimestamp;
    status.transition_sequence = sequence;
    status.position_normalized = 0.5F;
    status.flags = HYBRID_VEHICLE_STATUS_FLAGS_LANDED | HYBRID_VEHICLE_STATUS_FLAGS_LAND_DETECTION_FRESH;
    status.current_state = static_cast<uint8_t>(currentState);
    status.target_state = static_cast<uint8_t>(targetState);
    status.fault_reason = faultReason;
    status.command_result = static_cast<uint8_t>(commandResult);
    status.command_timestamp = commandTimestamp;

    mavlink_message_t message{};
    mavlink_msg_hybrid_vehicle_status_encode_chan(vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, mockLink()->mavlinkChannel(),
                                                  &message, &status);
    mockLink()->respondWithMavlinkMessage(message);
    if (waitForAcceptance) {
        QVERIFY(!statusAcceptedSpy.isEmpty() ||
                UnitTest::waitForSignal(statusAcceptedSpy, TestTimeout::longMs(), QStringLiteral("statusAccepted")));
    }
}

void HybridTransitionControllerTest::_injectSystemTime(uint32_t timeBootMs)
{
    mavlink_system_time_t time{};
    time.time_boot_ms = timeBootMs;
    mavlink_message_t message{};
    mavlink_msg_system_time_encode_chan(vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, mockLink()->mavlinkChannel(), &message,
                                        &time);
    mockLink()->respondWithMavlinkMessage(message);
}

void HybridTransitionControllerTest::_primeStableState(HybridVehicleState::CurrentState currentState)
{
    _injectStatus(currentState, HybridVehicleState::None, 7, MAV_RESULT_ACCEPTED, 70);
    QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), currentState);
    QVERIFY(vehicle()->hybridVehicleState()->canRequestTransform());
}

void HybridTransitionControllerTest::_invalidTargetIsRejected()
{
    mockLink()->clearReceivedMavCommandCounts();
    QVERIFY(!_controller->requestTransform(HybridVehicleState::None));
    QVERIFY(!_controller->requestTransform(3));
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_HYBRID_TRANSITION), 0);
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Idle);
}

void HybridTransitionControllerTest::_requestUsesTypedCommandContract()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    QTRY_VERIFY(mockLink()->hasHybridTransitionRequest());

    const mavlink_command_long_t& request = mockLink()->lastHybridTransitionRequest();
    QCOMPARE(request.command, static_cast<uint16_t>(MAV_CMD_DO_HYBRID_TRANSITION));
    QCOMPARE(request.target_component, static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));
    QCOMPARE(request.param1, static_cast<float>(HybridVehicleState::TargetRover));
    QCOMPARE(request.param2, 0.F);
    QCOMPARE(request.param3, 0.F);
    QCOMPARE(request.param4, 0.F);
    QCOMPARE(request.param5, 0.F);
    QCOMPARE(request.param6, 0.F);
    QCOMPARE(request.param7, 0.F);
}

void HybridTransitionControllerTest::_wrongAddressDoesNotDetach()
{
    const uint8_t wrongTargetSystem = MAVLinkProtocol::instance()->getSystemId() == 1 ? 2 : 1;
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_ACCEPTED, 17, MAV_COMP_ID_AUTOPILOT1, wrongTargetSystem, MAVLinkProtocol::getComponentId());
    QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Queued);

    _injectAck(MAV_RESULT_IN_PROGRESS, 17, MAV_COMP_ID_AUTOPILOT1, wrongTargetSystem,
               MAVLinkProtocol::getComponentId());
    QTest::qWait(20);
    QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Queued);

    _injectAck(MAV_RESULT_IN_PROGRESS, 17, MAV_COMP_ID_CAMERA);
    QTest::qWait(20);
    QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Queued);
}

void HybridTransitionControllerTest::_directTerminalAcceptedIsNoMotionOnlyForCurrentShape()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetQuad));
    _injectAck(MAV_RESULT_ACCEPTED, 99);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::NoMotionAccepted);
    QCOMPARE(_controller->commandResult(), static_cast<uint>(MAV_RESULT_ACCEPTED));
    QVERIFY(!_controller->busy());
    QVERIFY(!_controller->hasExpectedSequence());

    _injectStatus(HybridVehicleState::Quad, HybridVehicleState::None, 7, MAV_RESULT_ACCEPTED, 70);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_ACCEPTED, 100);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::AwaitingStatus);
    QVERIFY(!_controller->hasExpectedSequence());
    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 100, MAV_RESULT_ACCEPTED, 1000);
    QTest::qWait(20);
    QCOMPARE(_controller->transactionState(), HybridTransitionController::AwaitingStatus);
}

void HybridTransitionControllerTest::_terminalFailuresAllowExplicitRetry()
{
    const QList<MAV_RESULT> failures{MAV_RESULT_DENIED, MAV_RESULT_TEMPORARILY_REJECTED};
    for (const MAV_RESULT failure : failures) {
        QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
        _injectAck(failure, 0);
        QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
        QCOMPARE(_controller->commandResult(), static_cast<uint>(failure));
    }
}

void HybridTransitionControllerTest::_noFirstAckEndsWithoutSuccess()
{
    mockLink()->clearReceivedMavCommandCounts();
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    QElapsedTimer waitTimer;
    waitTimer.start();
    while (vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION) &&
           (waitTimer.elapsed() < MavCommandQueue::kTestMaxWaitMs)) {
        _injectStatus(HybridVehicleState::Quad, HybridVehicleState::None, 7, MAV_RESULT_ACCEPTED, 70);
        QTest::qWait(25);
    }
    QVERIFY(!vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Idle);
    QCOMPARE(_controller->commandResult(), static_cast<uint>(MAV_RESULT_FAILED));
    QVERIFY(mockLink()->receivedMavCommandCount(MAV_CMD_DO_HYBRID_TRANSITION) >= 2);
}

void HybridTransitionControllerTest::_duplicateRequestsAreRejected()
{
    mockLink()->clearReceivedMavCommandCounts();
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    QVERIFY(!_controller->requestTransform(HybridVehicleState::TargetRover));
    QVERIFY(!_controller->requestTransform(HybridVehicleState::TargetQuad));
    QTRY_COMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_HYBRID_TRANSITION), 1);

    _injectAck(MAV_RESULT_IN_PROGRESS, 23);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    const int count = mockLink()->receivedMavCommandCount(MAV_CMD_DO_HYBRID_TRANSITION);
    QVERIFY(!_controller->requestTransform(HybridVehicleState::TargetRover));
    QVERIFY(!_controller->requestTransform(HybridVehicleState::TargetQuad));
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_HYBRID_TRANSITION), count);
}

void HybridTransitionControllerTest::_progressSequenceMatrix_data()
{
    QTest::addColumn<uint>("sequence");
    QTest::newRow("ordinary") << 17U;
    QTest::newRow("zero") << 0U;
    QTest::newRow("unsigned-wrap") << UINT32_MAX;
}

void HybridTransitionControllerTest::_progressSequenceMatrix()
{
    QFETCH(uint, sequence);
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, sequence);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    QVERIFY(_controller->hasExpectedSequence());
    QCOMPARE(_controller->expectedSequence(), sequence);

    _injectAck(MAV_RESULT_IN_PROGRESS, sequence);
    QTest::qWait(20);
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    QCOMPARE(_controller->expectedSequence(), sequence);
}

void HybridTransitionControllerTest::_terminalAcceptedWaitsForPhysicalStatus()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 31);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    _injectStatus(HybridVehicleState::Transitioning, HybridVehicleState::TargetRover, 31, MAV_RESULT_IN_PROGRESS, 310);
    _injectAck(MAV_RESULT_ACCEPTED, 31);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::AwaitingStatus);
    QVERIFY(_controller->busy());

    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 31, MAV_RESULT_ACCEPTED, 310);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
    QVERIFY(!_controller->busy());
}

void HybridTransitionControllerTest::_terminalFailedEndsDetachedTransaction()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 41);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    _injectAck(MAV_RESULT_FAILED, 41);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
    QCOMPARE(_controller->commandResult(), static_cast<uint>(MAV_RESULT_FAILED));
}

void HybridTransitionControllerTest::_statusOnlyCompletion()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 51);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 51, MAV_RESULT_ACCEPTED, 510);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
    QCOMPARE(_controller->commandResult(), static_cast<uint>(MAV_RESULT_ACCEPTED));
}

void HybridTransitionControllerTest::_mismatchingStatusDoesNotComplete()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 61);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 61, MAV_RESULT_FAILED, 610);
    QTest::qWait(20);
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 61, MAV_RESULT_ACCEPTED, 70);
    QTest::qWait(20);
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Detached);
}

void HybridTransitionControllerTest::_firstProgressStatusOwnsTimestampAssociation()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 65);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    _injectStatus(HybridVehicleState::Transitioning, HybridVehicleState::TargetRover, 65, MAV_RESULT_IN_PROGRESS, 650);
    QTRY_COMPARE(vehicle()->hybridVehicleState()->commandTimestamp(), 650);
    _injectStatus(HybridVehicleState::Transitioning, HybridVehicleState::TargetRover, 65, MAV_RESULT_IN_PROGRESS, 651);
    QTRY_COMPARE(vehicle()->hybridVehicleState()->commandTimestamp(), 651);

    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 65, MAV_RESULT_ACCEPTED, 651);
    QTest::qWait(20);
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 65, MAV_RESULT_ACCEPTED, 650);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
}

void HybridTransitionControllerTest::_supersedingSequenceRequiresIndependentResync()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 71);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    _injectStatus(HybridVehicleState::Transitioning, HybridVehicleState::TargetRover, 72, MAV_RESULT_IN_PROGRESS, 720);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::SupersededUnconfirmed);
    QVERIFY(!_controller->requestTransform(HybridVehicleState::TargetRover));

    _injectStatus(HybridVehicleState::Rover, HybridVehicleState::None, 72, MAV_RESULT_ACCEPTED, 720);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
}

void HybridTransitionControllerTest::_faultAndStaleStatusEndWithoutSuccess()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 81);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    _injectStatus(HybridVehicleState::TransitionFault, HybridVehicleState::TargetRover, 81, MAV_RESULT_FAILED, 810,
                  HYBRID_VEHICLE_FAULT_ACTUATOR_COMMUNICATION);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Faulted);

    _injectStatus(HybridVehicleState::Quad, HybridVehicleState::None, 81, MAV_RESULT_ACCEPTED, 810);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Idle);
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 82);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Unconfirmed);
}

void HybridTransitionControllerTest::_confirmedRebootCancelsQueuedCommand()
{
    _injectStatus(HybridVehicleState::Quad, HybridVehicleState::None, 7, MAV_RESULT_ACCEPTED, 70,
                  HYBRID_VEHICLE_FAULT_NONE, 900000);
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
    _injectStatus(HybridVehicleState::Quad, HybridVehicleState::None, 7, MAV_RESULT_ACCEPTED, 70,
                  HYBRID_VEHICLE_FAULT_NONE, 100, false);
    _injectSystemTime(5000);
    _injectSystemTime(20);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::VehicleRebootedUnconfirmed);
    QTRY_VERIFY(!vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
}

void HybridTransitionControllerTest::_detachedTransactionOutlivesGenericQueueWindow()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    _injectAck(MAV_RESULT_IN_PROGRESS, 91);
    QTRY_COMPARE(_controller->transactionState(), HybridTransitionController::Detached);

    for (int i = 0; i < 40; ++i) {
        _injectStatus(HybridVehicleState::Transitioning, HybridVehicleState::TargetRover, 91, MAV_RESULT_IN_PROGRESS,
                      910);
        QTest::qWait(40);
    }
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Detached);
    QVERIFY(!vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
}

UT_REGISTER_TEST(HybridTransitionControllerTest, TestLabel::Integration, TestLabel::Vehicle)
