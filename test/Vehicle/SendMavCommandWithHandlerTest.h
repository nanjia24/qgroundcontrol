#pragma once

#include "BaseClasses/VehicleTest.h"
#include "Vehicle.h"

class SendMavCommandWithHandlerTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit SendMavCommandWithHandlerTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }

private slots:
    void _performTestCases();
    void _compIdAllFailure();
    void _duplicateCommand();
    void _strictAckMatching();
    void _detachOnProgress();
    void _hybridTransitionRetriesBeforeFirstMatchingAck();

private:
    struct TestCase_t
    {
        MAV_CMD command;
        MAV_RESULT expectedCommandResult;
        bool expectInProgressResult;
        Vehicle::MavCmdResultFailureCode_t expectedFailureCode;
        int expectedSendCount;
    };

    struct AckMatcherData_t {
        uint8_t senderSystemId;
        uint8_t senderComponentId;
        uint8_t targetSystemId;
        uint8_t targetComponentId;
        MAV_CMD command;
    };

    struct AckHandlerData_t {
        int resultHandlerCallCount = 0;
        int progressHandlerCallCount = 0;
    };

    void _testCaseWorker(TestCase_t& testCase);

    static void _mavCmdResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack,
                                     Vehicle::MavCmdResultFailureCode_t failureCode);
    static void _mavCmdProgressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);
    static void _compIdAllFailureMavCmdResultHandler(void* resultHandlerData, int compId,
                                                     const mavlink_command_ack_t& ack,
                                                     Vehicle::MavCmdResultFailureCode_t failureCode);
    static bool _strictAckMatcher(void* matcherData, const mavlink_message_t& message, const mavlink_command_ack_t& ack);
    static void _countResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack,
                                    Vehicle::MavCmdResultFailureCode_t failureCode);
    static void _countProgressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);

    void _injectAck(MAV_CMD command, MAV_RESULT result, uint8_t senderSystemId, uint8_t senderComponentId,
                    uint8_t targetSystemId, uint8_t targetComponentId);

    static bool _resultHandlerCalled;
    static bool _progressHandlerCalled;

    static TestCase_t _rgTestCases[];
};
