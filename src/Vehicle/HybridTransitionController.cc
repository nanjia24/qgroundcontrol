#include "HybridTransitionController.h"

#include "HybridVehicleState.h"
#include "MAVLinkProtocol.h"
#include "MavCommandQueue.h"
#include "Vehicle.h"

namespace {

constexpr int kAcceptedStatusWatchdogMs = 3000;

bool isBusyState(HybridTransitionController::TransactionState state)
{
    return (state == HybridTransitionController::Queued) || (state == HybridTransitionController::Detached) ||
           (state == HybridTransitionController::AwaitingStatus);
}

bool isIndependentResyncState(HybridTransitionController::TransactionState state)
{
    return (state == HybridTransitionController::NoMotionAccepted) ||
           (state == HybridTransitionController::Unconfirmed) ||
           (state == HybridTransitionController::SupersededUnconfirmed) ||
           (state == HybridTransitionController::VehicleRebootedUnconfirmed) ||
           (state == HybridTransitionController::Faulted);
}

}  // namespace

HybridTransitionController::HybridTransitionController(Vehicle* vehicle, HybridVehicleState* state)
    : QObject(vehicle), _vehicle(vehicle), _state(state)
{
    _acceptedStatusWatchdog.setSingleShot(true);
    _acceptedStatusWatchdog.setInterval(kAcceptedStatusWatchdogMs);
    connect(&_acceptedStatusWatchdog, &QTimer::timeout, this, [this]() {
        if (_transactionState == AwaitingStatus) {
            _finishAwaitingIndependentResync(Unconfirmed, false, 0);
        }
    });
    connect(_state, &HybridVehicleState::statusAccepted, this, &HybridTransitionController::_handleStatusAccepted);
    connect(_state, &HybridVehicleState::freshnessChanged, this, &HybridTransitionController::_handleFreshnessChanged);
    connect(_state, &HybridVehicleState::rebootConfirmed, this, &HybridTransitionController::handleVehicleReboot);
}

bool HybridTransitionController::busy() const
{
    return isBusyState(_transactionState);
}

bool HybridTransitionController::requestTransform(int targetState)
{
    if (_transactionState != Idle || !_state->canRequestTransform() ||
        ((targetState != HybridVehicleState::TargetQuad) && (targetState != HybridVehicleState::TargetRover))) {
        return false;
    }

    _requestedTarget = targetState;
    emit requestedTargetChanged();
    _preRequestStatusReceiptTime = _state->localMonotonicStatusReceiptTime();
    _preRequestCommandTimestamp = _state->commandTimestamp();
    _preRequestCurrentState = _state->currentState();
    _preRequestHadValidStatus = _state->hasValidStatus();
    _havePostRequestStatus = false;
    _haveAssociatedCommandTimestamp = false;
    _associatedCommandTimestamp = 0;
    _resyncRequiresSequence = false;
    _clearExpectedSequence();
    _setCommandResult(MAV_RESULT_UNSUPPORTED);

    MavCmdAckHandlerInfo_t handlerInfo{};
    handlerInfo.resultHandler = _resultHandler;
    handlerInfo.resultHandlerData = this;
    handlerInfo.progressHandler = _progressHandler;
    handlerInfo.progressHandlerData = this;
    handlerInfo.ackMatcher = _ackMatcher;
    handlerInfo.ackMatcherData = this;
    handlerInfo.detachOnProgress = true;

    _vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION,
                                        static_cast<float>(targetState), 0.F, 0.F, 0.F, 0.F, 0.F, 0.F);
    _setTransactionState(Queued);
    return true;
}

void HybridTransitionController::handleDetachedAck(const mavlink_message_t& message, const mavlink_command_ack_t& ack)
{
    if ((_transactionState != Detached) || !_strictAckMatches(message, ack)) {
        return;
    }

    if (ack.result == MAV_RESULT_IN_PROGRESS) {
        return;
    }

    _handleResult(ack, MavCmdResultCommandResultOnly);
}

void HybridTransitionController::handleVehicleReboot()
{
    if (!busy()) {
        return;
    }

    _cancelQueuedCommand();
    _acceptedStatusWatchdog.stop();
    _finishAwaitingIndependentResync(VehicleRebootedUnconfirmed, false, 0);
}

void HybridTransitionController::_resultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack,
                                                MavCmdResultFailureCode_t failureCode)
{
    Q_UNUSED(compId);
    static_cast<HybridTransitionController*>(resultHandlerData)->_handleResult(ack, failureCode);
}

void HybridTransitionController::_progressHandler(void* progressHandlerData, int compId,
                                                  const mavlink_command_ack_t& ack)
{
    Q_UNUSED(compId);
    static_cast<HybridTransitionController*>(progressHandlerData)->_handleProgress(ack);
}

bool HybridTransitionController::_ackMatcher(void* matcherData, const mavlink_message_t& message,
                                             const mavlink_command_ack_t& ack)
{
    return static_cast<HybridTransitionController*>(matcherData)->_strictAckMatches(message, ack);
}

void HybridTransitionController::_handleResult(const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    _setCommandResult(static_cast<MAV_RESULT>(ack.result));

    if ((failureCode != MavCmdResultCommandResultOnly) || (ack.result != MAV_RESULT_ACCEPTED)) {
        _finishWithoutPhysicalSuccess(static_cast<MAV_RESULT>(ack.result));
        return;
    }

    const bool directResult = _transactionState == Queued;
    const bool requestedShapeWasAlreadyStable =
        _preRequestHadValidStatus && (((_requestedTarget == HybridVehicleState::TargetQuad) &&
                                       (_preRequestCurrentState == HybridVehicleState::Quad)) ||
                                      ((_requestedTarget == HybridVehicleState::TargetRover) &&
                                       (_preRequestCurrentState == HybridVehicleState::Rover)));
    if (directResult && requestedShapeWasAlreadyStable) {
        _finishAwaitingIndependentResync(NoMotionAccepted, false, 0);
        return;
    }

    _setTransactionState(AwaitingStatus);
    _acceptedStatusWatchdog.start();
    _evaluateActiveStatus();
}

void HybridTransitionController::_handleProgress(const mavlink_command_ack_t& ack)
{
    if (_transactionState != Queued) {
        return;
    }

    _setCommandResult(MAV_RESULT_IN_PROGRESS);
    _setExpectedSequence(static_cast<uint32_t>(ack.result_param2));
    _setTransactionState(Detached);
    _evaluateActiveStatus();
}

void HybridTransitionController::_handleStatusAccepted()
{
    _havePostRequestStatus = _state->localMonotonicStatusReceiptTime() >= _preRequestStatusReceiptTime;
    if (isIndependentResyncState(_transactionState)) {
        _evaluateIndependentResync();
    } else if (busy()) {
        _evaluateActiveStatus();
    }
}

void HybridTransitionController::_handleFreshnessChanged()
{
    if (busy() && _state->stale()) {
        _acceptedStatusWatchdog.stop();
        _cancelQueuedCommand();
        _finishAwaitingIndependentResync(Unconfirmed, false, 0);
    }
}

void HybridTransitionController::_evaluateActiveStatus()
{
    if (!_state->hasValidStatus() || !_havePostRequestStatus) {
        return;
    }

    if ((_state->currentState() == HybridVehicleState::TransitionFault) ||
        (_state->faultReason() != HYBRID_VEHICLE_FAULT_NONE)) {
        _acceptedStatusWatchdog.stop();
        _cancelQueuedCommand();
        _finishAwaitingIndependentResync(Faulted, false, 0);
        return;
    }

    if (_haveExpectedSequence && (_state->transitionSequence() != _expectedSequence)) {
        _acceptedStatusWatchdog.stop();
        _finishAwaitingIndependentResync(SupersededUnconfirmed, true, _state->transitionSequence());
        return;
    }

    if (!_haveExpectedSequence) {
        return;
    }

    if (_state->commandResult() == MAV_RESULT_IN_PROGRESS) {
        if (!_haveAssociatedCommandTimestamp) {
            _associatedCommandTimestamp = _state->commandTimestamp();
            _haveAssociatedCommandTimestamp = true;
        }
        return;
    }

    const bool commandTimestampMatches = _haveAssociatedCommandTimestamp
                                             ? (_state->commandTimestamp() == _associatedCommandTimestamp)
                                             : (_state->commandTimestamp() != _preRequestCommandTimestamp);
    if (_statusIsRequestedStableShape() && (_state->commandResult() == MAV_RESULT_ACCEPTED) &&
        commandTimestampMatches) {
        _acceptedStatusWatchdog.stop();
        _setCommandResult(MAV_RESULT_ACCEPTED);
        _setTransactionState(Idle);
    }
}

void HybridTransitionController::_evaluateIndependentResync()
{
    if (!_statusIsHealthyStableAccepted()) {
        return;
    }
    if (_resyncRequiresSequence && (_state->transitionSequence() != _resyncSequence)) {
        return;
    }
    _setTransactionState(Idle);
}

void HybridTransitionController::_setTransactionState(TransactionState state)
{
    if (_transactionState == state) {
        return;
    }
    const bool wasBusy = busy();
    _transactionState = state;
    emit transactionStateChanged();
    if (wasBusy != busy()) {
        emit busyChanged();
    }
}

void HybridTransitionController::_setCommandResult(MAV_RESULT result)
{
    if (_commandResult == result) {
        return;
    }
    _commandResult = result;
    emit commandResultChanged();
}

void HybridTransitionController::_setExpectedSequence(uint32_t sequence)
{
    const bool changed = !_haveExpectedSequence || (_expectedSequence != sequence);
    _haveExpectedSequence = true;
    _expectedSequence = sequence;
    if (changed) {
        emit expectedSequenceChanged();
    }
}

void HybridTransitionController::_clearExpectedSequence()
{
    if (!_haveExpectedSequence) {
        return;
    }
    _haveExpectedSequence = false;
    _expectedSequence = 0;
    emit expectedSequenceChanged();
}

void HybridTransitionController::_cancelQueuedCommand()
{
    if (_transactionState == Queued) {
        _vehicle->_mavCmdQueue->cancelCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION);
    }
}

void HybridTransitionController::_finishWithoutPhysicalSuccess(MAV_RESULT result)
{
    _acceptedStatusWatchdog.stop();
    _setCommandResult(result);
    _setTransactionState(Idle);
}

void HybridTransitionController::_finishAwaitingIndependentResync(TransactionState state, bool requireSequence,
                                                                  uint32_t sequence)
{
    _resyncRequiresSequence = requireSequence;
    _resyncSequence = sequence;
    _setTransactionState(state);
}

bool HybridTransitionController::_strictAckMatches(const mavlink_message_t& message,
                                                   const mavlink_command_ack_t& ack) const
{
    return (ack.command == MAV_CMD_DO_HYBRID_TRANSITION) && (message.sysid == _vehicle->id()) &&
           (message.compid == MAV_COMP_ID_AUTOPILOT1) &&
           (ack.target_system == MAVLinkProtocol::instance()->getSystemId()) &&
           (ack.target_component == MAVLinkProtocol::getComponentId());
}

bool HybridTransitionController::_statusIsRequestedStableShape() const
{
    return ((_requestedTarget == HybridVehicleState::TargetQuad) &&
            (_state->currentState() == HybridVehicleState::Quad)) ||
           ((_requestedTarget == HybridVehicleState::TargetRover) &&
            (_state->currentState() == HybridVehicleState::Rover));
}

bool HybridTransitionController::_statusIsHealthyStableAccepted() const
{
    const bool stableShape =
        (_state->currentState() == HybridVehicleState::Quad) || (_state->currentState() == HybridVehicleState::Rover);
    return _state->hasValidStatus() && stableShape && (_state->faultReason() == HYBRID_VEHICLE_FAULT_NONE) &&
           (_state->commandResult() == MAV_RESULT_ACCEPTED);
}
