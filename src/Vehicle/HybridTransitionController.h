#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "VehicleTypes.h"

class HybridVehicleState;
class Vehicle;

class HybridTransitionController final : public QObject, public VehicleTypes
{
    Q_OBJECT
    Q_PROPERTY(TransactionState transactionState READ transactionState NOTIFY transactionStateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(uint commandResult READ commandResult NOTIFY commandResultChanged)
    Q_PROPERTY(int requestedTarget READ requestedTarget NOTIFY requestedTargetChanged)
    Q_PROPERTY(bool hasExpectedSequence READ hasExpectedSequence NOTIFY expectedSequenceChanged)
    Q_PROPERTY(uint expectedSequence READ expectedSequence NOTIFY expectedSequenceChanged)

public:
    enum TransactionState
    {
        Idle,
        Queued,
        Detached,
        AwaitingStatus,
        NoMotionAccepted,
        Unconfirmed,
        SupersededUnconfirmed,
        VehicleRebootedUnconfirmed,
        Faulted,
    };
    Q_ENUM(TransactionState)

    explicit HybridTransitionController(Vehicle* vehicle, HybridVehicleState* state);

    TransactionState transactionState() const { return _transactionState; }

    bool busy() const;

    uint commandResult() const { return static_cast<uint>(_commandResult); }

    int requestedTarget() const { return _requestedTarget; }

    bool hasExpectedSequence() const { return _haveExpectedSequence; }

    uint32_t expectedSequence() const { return _expectedSequence; }

    Q_INVOKABLE bool requestTransform(int targetState);

    void handleDetachedAck(const mavlink_message_t& message, const mavlink_command_ack_t& ack);
    void handleVehicleReboot();

signals:
    void transactionStateChanged();
    void busyChanged();
    void commandResultChanged();
    void requestedTargetChanged();
    void expectedSequenceChanged();

private:
    static void _resultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack,
                               MavCmdResultFailureCode_t failureCode);
    static void _progressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);
    static bool _ackMatcher(void* matcherData, const mavlink_message_t& message, const mavlink_command_ack_t& ack);

    void _handleResult(const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);
    void _handleProgress(const mavlink_command_ack_t& ack);
    void _handleStatusAccepted();
    void _handleFreshnessChanged();
    void _evaluateActiveStatus();
    void _evaluateIndependentResync();
    void _setTransactionState(TransactionState state);
    void _setCommandResult(MAV_RESULT result);
    void _setExpectedSequence(uint32_t sequence);
    void _clearExpectedSequence();
    void _cancelQueuedCommand();
    void _finishWithoutPhysicalSuccess(MAV_RESULT result);
    void _finishAwaitingIndependentResync(TransactionState state, bool requireSequence, uint32_t sequence);
    bool _strictAckMatches(const mavlink_message_t& message, const mavlink_command_ack_t& ack) const;
    bool _statusIsRequestedStableShape() const;
    bool _statusIsHealthyStableAccepted() const;

    Vehicle* const _vehicle;
    HybridVehicleState* const _state;
    QTimer _acceptedStatusWatchdog;

    TransactionState _transactionState = Idle;
    MAV_RESULT _commandResult = MAV_RESULT_UNSUPPORTED;
    int _requestedTarget = 0;
    qint64 _preRequestStatusReceiptTime = -1;
    uint64_t _preRequestCommandTimestamp = 0;
    int _preRequestCurrentState = 0;
    bool _preRequestHadValidStatus = false;
    bool _havePostRequestStatus = false;
    bool _haveExpectedSequence = false;
    uint32_t _expectedSequence = 0;
    bool _haveAssociatedCommandTimestamp = false;
    uint64_t _associatedCommandTimestamp = 0;
    bool _resyncRequiresSequence = false;
    uint32_t _resyncSequence = 0;
};
