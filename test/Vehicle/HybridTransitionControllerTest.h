#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"
#include "HybridVehicleState.h"

class HybridTransitionController;

class HybridTransitionControllerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void init() override;

    void _invalidTargetIsRejected();
    void _requestUsesTypedCommandContract();
    void _wrongAddressDoesNotDetach();
    void _directTerminalAcceptedIsNoMotionOnlyForCurrentShape();
    void _terminalFailuresAllowExplicitRetry();
    void _lateBaselineFailureRequiresStatusCorroboration();
    void _firstAckRequiresBaselineOrNextSequence();
    void _noFirstAckEndsWithoutSuccess();
    void _duplicateQueueEntryDoesNotLeaveControllerBusy();
    void _duplicateRequestsAreRejected();
    void _progressSequenceMatrix_data();
    void _progressSequenceMatrix();
    void _terminalAcceptedWaitsForPhysicalStatus();
    void _terminalFailedEndsDetachedTransaction();
    void _lateAckFromCompletedTransactionIsIgnored();
    void _statusOnlyCompletion();
    void _mismatchingStatusDoesNotComplete();
    void _firstProgressStatusOwnsTimestampAssociation();
    void _supersedingSequenceRequiresIndependentResync();
    void _faultAndStaleStatusEndWithoutSuccess();
    void _confirmedRebootCancelsQueuedCommand();
    void _detachedTransactionOutlivesGenericQueueWindow();

private:
    void _connectQuadRover();
    void _injectAck(MAV_RESULT result, uint32_t sequence, uint8_t senderComponent = MAV_COMP_ID_AUTOPILOT1,
                    uint8_t targetSystem = 0, uint8_t targetComponent = 0);
    void _injectStatus(HybridVehicleState::CurrentState currentState, HybridVehicleState::TargetState targetState,
                       uint32_t sequence, MAV_RESULT commandResult, uint64_t commandTimestamp,
                       uint8_t faultReason = HYBRID_VEHICLE_FAULT_NONE, uint64_t statusTimestamp = 0,
                       bool waitForAcceptance = true);
    void _injectSystemTime(uint32_t timeBootMs);
    void _primeStableState(HybridVehicleState::CurrentState currentState);

    HybridTransitionController* _controller = nullptr;
    uint64_t _statusTimestamp = 100;
};
