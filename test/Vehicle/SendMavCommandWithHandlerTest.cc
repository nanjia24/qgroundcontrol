#include "SendMavCommandWithHandlerTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QScopeGuard>

#include "MAVLinkProtocol.h"
#include "MavCommandQueue.h"
#include "MultiVehicleManager.h"

namespace {
constexpr MAV_CMD kHybridTransitionCommand = MAV_CMD_DO_HYBRID_TRANSITION;
}

SendMavCommandWithHandlerTest::TestCase_t SendMavCommandWithHandlerTest::_rgTestCases[] = {
    {MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED, false,
     Vehicle::MavCmdResultCommandResultOnly, 1},
    {MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED, MAV_RESULT_FAILED, false, Vehicle::MavCmdResultCommandResultOnly,
     1},
    {MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED, false,
     Vehicle::MavCmdResultCommandResultOnly, 2},
    {MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED, MAV_RESULT_FAILED, false,
     Vehicle::MavCmdResultCommandResultOnly, 2},
    {MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, false, Vehicle::MavCmdResultFailureNoResponseToCommand,
     Vehicle::_mavCommandMaxRetryCount},
    {MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY, MAV_RESULT_FAILED, false,
     Vehicle::MavCmdResultFailureNoResponseToCommand, 1},
    {MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED, MAV_RESULT_ACCEPTED, true,
     Vehicle::MavCmdResultCommandResultOnly, 1},
    {MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED, MAV_RESULT_FAILED, true,
     Vehicle::MavCmdResultCommandResultOnly, 1},
    {MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK, MAV_RESULT_FAILED, true,
     Vehicle::MavCmdResultFailureNoResponseToCommand, 1},
};
bool SendMavCommandWithHandlerTest::_resultHandlerCalled = false;
bool SendMavCommandWithHandlerTest::_progressHandlerCalled = false;

void SendMavCommandWithHandlerTest::_mavCmdResultHandler(void* resultHandlerData, int compId,
                                                         const mavlink_command_ack_t& ack,
                                                         Vehicle::MavCmdResultFailureCode_t failureCode)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);
    _resultHandlerCalled = true;
    QCOMPARE(MAV_COMP_ID_AUTOPILOT1, compId);
    QCOMPARE(testCase->expectedCommandResult, ack.result);
    QCOMPARE(testCase->expectedFailureCode, failureCode);
}

void SendMavCommandWithHandlerTest::_mavCmdProgressHandler(void* progressHandlerData, int compId,
                                                           const mavlink_command_ack_t& ack)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(progressHandlerData);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    _progressHandlerCalled = true;
    QCOMPARE(MAV_COMP_ID_AUTOPILOT1, compId);
    QCOMPARE(MAV_RESULT_IN_PROGRESS, ack.result);
    QCOMPARE(1, ack.progress);
    // Command should still be in list
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase->command) != -1);
}

void SendMavCommandWithHandlerTest::_testCaseWorker(TestCase_t& testCase)
{
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _mavCmdResultHandler;
    handlerInfo.resultHandlerData = &testCase;
    handlerInfo.progressHandler = _mavCmdProgressHandler;
    handlerInfo.progressHandlerData = &testCase;
    _resultHandlerCalled = false;
    _progressHandlerCalled = false;
    _mockLink->clearReceivedMavCommandCounts();
    const Vehicle::MavCmdQueueEntryToken token =
        vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, testCase.command);
    QVERIFY(token != Vehicle::InvalidMavCmdQueueEntryToken);
    if (testCase.expectInProgressResult) {
        QVERIFY_TRUE_WAIT(_progressHandlerCalled, TestTimeout::longMs());
    }
    QVERIFY_TRUE_WAIT(_resultHandlerCalled, TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command), -1);
}

void SendMavCommandWithHandlerTest::_performTestCases()
{
    for (TestCase_t& testCase : _rgTestCases) {
        _testCaseWorker(testCase);
    }
}

void SendMavCommandWithHandlerTest::_duplicateCommand()
{
    SendMavCommandWithHandlerTest::TestCase_t testCase = {MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0,
                                                          Vehicle::MavCmdResultFailureDuplicateCommand, 1};
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _mavCmdResultHandler;
    handlerInfo.resultHandlerData = &testCase;
    handlerInfo.progressHandler = _mavCmdProgressHandler;
    handlerInfo.progressHandlerData = &testCase;
    _resultHandlerCalled = false;
    _progressHandlerCalled = false;
    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(testCase.command) == 1, TestTimeout::shortMs());
    QVERIFY(!_resultHandlerCalled);
    QVERIFY(!_progressHandlerCalled);
    const Vehicle::MavCmdQueueEntryToken token =
        vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, testCase.command);
    QCOMPARE(token, Vehicle::InvalidMavCmdQueueEntryToken);
    // Duplicate command response should happen immediately
    QVERIFY(_resultHandlerCalled);
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command) != -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command), 1);
}

void SendMavCommandWithHandlerTest::_compIdAllFailureMavCmdResultHandler(void* /*resultHandlerData*/, int compId,
                                                                         const mavlink_command_ack_t& ack,
                                                                         Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _resultHandlerCalled = true;
    QCOMPARE(compId, MAV_COMP_ID_ALL);
    QCOMPARE(ack.result, MAV_RESULT_FAILED);
    QCOMPARE(failureCode, Vehicle::MavCmdResultCommandResultOnly);
}

void SendMavCommandWithHandlerTest::_compIdAllFailure()
{
    SendMavCommandWithHandlerTest::TestCase_t testCase = {MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0,
                                                          Vehicle::MavCmdResultFailureDuplicateCommand, 0};
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _compIdAllFailureMavCmdResultHandler;
    _resultHandlerCalled = false;
    _mockLink->clearReceivedMavCommandCounts();
    const Vehicle::MavCmdQueueEntryToken token =
        vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_ALL, testCase.command);
    QCOMPARE(token, Vehicle::InvalidMavCmdQueueEntryToken);
    QCOMPARE(_resultHandlerCalled, true);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, testCase.command), -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);
}

bool SendMavCommandWithHandlerTest::_strictAckMatcher(void* matcherData, const mavlink_message_t& message,
                                                      const mavlink_command_ack_t& ack)
{
    const AckMatcherData_t* data = static_cast<const AckMatcherData_t*>(matcherData);
    return message.sysid == data->senderSystemId && message.compid == data->senderComponentId &&
           ack.target_system == data->targetSystemId && ack.target_component == data->targetComponentId &&
           ack.command == data->command;
}

void SendMavCommandWithHandlerTest::_countResultHandler(void* resultHandlerData, int /*compId*/,
                                                        const mavlink_command_ack_t& /*ack*/,
                                                        Vehicle::MavCmdResultFailureCode_t /*failureCode*/)
{
    static_cast<AckHandlerData_t*>(resultHandlerData)->resultHandlerCallCount++;
}

void SendMavCommandWithHandlerTest::_countProgressHandler(void* progressHandlerData, int /*compId*/,
                                                          const mavlink_command_ack_t& /*ack*/)
{
    static_cast<AckHandlerData_t*>(progressHandlerData)->progressHandlerCallCount++;
}

bool SendMavCommandWithHandlerTest::_injectAck(MAV_CMD command, MAV_RESULT result, uint8_t senderSystemId,
                                               uint8_t senderComponentId, uint8_t targetSystemId,
                                               uint8_t targetComponentId)
{
    mavlink_message_t message{};
    (void)mavlink_msg_command_ack_pack_chan(senderSystemId, senderComponentId, MAVLINK_COMM_0, &message, command,
                                            result, 0, 0, targetSystemId, targetComponentId);
    mavlink_command_ack_t ack{};
    mavlink_msg_command_ack_decode(&message, &ack);
    return MultiVehicleManager::instance()->activeVehicle()->_mavCmdQueue->handleCommandAck(message, ack);
}

void SendMavCommandWithHandlerTest::_strictAckMatching()
{
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    const uint8_t qgcSystemId = MAVLinkProtocol::instance()->getSystemId();
    const uint8_t qgcComponentId = MAVLinkProtocol::getComponentId();
    AckMatcherData_t matcherData{static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, qgcSystemId,
                                 qgcComponentId, kHybridTransitionCommand};
    AckHandlerData_t handlerData;
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _countResultHandler;
    handlerInfo.resultHandlerData = &handlerData;
    handlerInfo.progressHandler = _countProgressHandler;
    handlerInfo.progressHandlerData = &handlerData;
    handlerInfo.ackMatcher = _strictAckMatcher;
    handlerInfo.ackMatcherData = &matcherData;
    handlerInfo.detachOnProgress = true;

    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand);
    QVERIFY(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));

    QVERIFY(!_injectAck(kHybridTransitionCommand, MAV_RESULT_ACCEPTED, static_cast<uint8_t>(vehicle->id()),
                        MAV_COMP_ID_AUTOPILOT1, qgcSystemId, qgcComponentId + 1));
    QVERIFY(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QCOMPARE(handlerData.resultHandlerCallCount, 0);
    QCOMPARE(handlerData.progressHandlerCallCount, 0);

    QVERIFY(!_injectAck(kHybridTransitionCommand, MAV_RESULT_IN_PROGRESS, static_cast<uint8_t>(vehicle->id()) + 1,
                        MAV_COMP_ID_AUTOPILOT1, qgcSystemId, qgcComponentId));
    QVERIFY(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QCOMPARE(handlerData.progressHandlerCallCount, 0);

    QVERIFY(_injectAck(kHybridTransitionCommand, MAV_RESULT_IN_PROGRESS, static_cast<uint8_t>(vehicle->id()),
                       MAV_COMP_ID_AUTOPILOT1, qgcSystemId, qgcComponentId));
    QVERIFY(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QCOMPARE(handlerData.progressHandlerCallCount, 1);
    QCOMPARE(handlerData.resultHandlerCallCount, 0);

    handlerInfo.detachOnProgress = false;
    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand);
    QVERIFY(_injectAck(kHybridTransitionCommand, MAV_RESULT_ACCEPTED, static_cast<uint8_t>(vehicle->id()),
                       MAV_COMP_ID_AUTOPILOT1, qgcSystemId, qgcComponentId));
    QVERIFY(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QCOMPARE(handlerData.resultHandlerCallCount, 1);
}

void SendMavCommandWithHandlerTest::_hybridTransitionRetriesBeforeFirstMatchingAck()
{
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    AckMatcherData_t matcherData{static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1,
                                 static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
                                 static_cast<uint8_t>(MAVLinkProtocol::getComponentId()), kHybridTransitionCommand};
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.ackMatcher = _strictAckMatcher;
    handlerInfo.ackMatcherData = &matcherData;

    _mockLink->setHoldHybridTransitionAcks(true);
    const auto restoreHybridTransitionAcks = qScopeGuard([this]() { _mockLink->setHoldHybridTransitionAcks(false); });
    _mockLink->clearReceivedMavCommandCounts();
    const Vehicle::MavCmdQueueEntryToken token =
        vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand, 17.0f);
    QVERIFY(token != Vehicle::InvalidMavCmdQueueEntryToken);
    QTRY_COMPARE(_mockLink->receivedMavCommandCount(kHybridTransitionCommand), 1);
    QCOMPARE(_mockLink->lastHybridTransitionRequest().confirmation, 0);
    QTRY_COMPARE(_mockLink->receivedMavCommandCount(kHybridTransitionCommand), 2);
    QCOMPARE(_mockLink->lastHybridTransitionRequest().confirmation, 1);
    QVERIFY(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));

    QElapsedTimer secondAckWindow;
    secondAckWindow.start();
    QTRY_COMPARE_WITH_TIMEOUT(_mockLink->receivedMavCommandCount(kHybridTransitionCommand), 3,
                              MavCommandQueue::kTestMaxWaitMs);
    QVERIFY(secondAckWindow.elapsed() >= (MavCommandQueue::kTestAckTimeoutMs - 100));
    QCOMPARE(_mockLink->lastHybridTransitionRequest().confirmation, 2);

    const int sendCountBeforeDuplicate = _mockLink->receivedMavCommandCount(kHybridTransitionCommand);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand, false, 42.0f);
    QCOMPARE(_mockLink->receivedMavCommandCount(kHybridTransitionCommand), sendCountBeforeDuplicate);

    QVERIFY(vehicle->_mavCmdQueue->cancelCommand(token));
    QVERIFY(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
}

void SendMavCommandWithHandlerTest::_cancelCommand()
{
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    AckMatcherData_t matcherData{static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1,
                                 static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
                                 static_cast<uint8_t>(MAVLinkProtocol::getComponentId()), kHybridTransitionCommand};
    AckHandlerData_t handlerData;
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _countResultHandler;
    handlerInfo.resultHandlerData = &handlerData;
    handlerInfo.ackMatcher = _strictAckMatcher;
    handlerInfo.ackMatcherData = &matcherData;

    const Vehicle::MavCmdQueueEntryToken token =
        vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand);
    QVERIFY(token != Vehicle::InvalidMavCmdQueueEntryToken);
    QVERIFY(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QVERIFY(vehicle->_mavCmdQueue->isPending(token));

    QVERIFY(!vehicle->_mavCmdQueue->cancelCommand(Vehicle::InvalidMavCmdQueueEntryToken));
    QVERIFY(vehicle->_mavCmdQueue->cancelCommand(token));
    QVERIFY(!vehicle->_mavCmdQueue->cancelCommand(token));
    QVERIFY(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QCOMPARE(handlerData.resultHandlerCallCount, 0);
}

void SendMavCommandWithHandlerTest::_stoppedQueueRejectsSend()
{
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    AckHandlerData_t handlerData;
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _countResultHandler;
    handlerInfo.resultHandlerData = &handlerData;

    vehicle->_mavCmdQueue->stop();
    const Vehicle::MavCmdQueueEntryToken token =
        vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand);

    QCOMPARE(token, Vehicle::InvalidMavCmdQueueEntryToken);
    QVERIFY(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, kHybridTransitionCommand));
    QCOMPARE(handlerData.resultHandlerCallCount, 0);
}

UT_REGISTER_TEST(SendMavCommandWithHandlerTest, TestLabel::Integration, TestLabel::Vehicle)
