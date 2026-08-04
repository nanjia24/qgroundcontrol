#pragma once

#include "UnitTest.h"

class HybridVehicleStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _initialStateIsUnknownAndStale();
    void _decodesStatesAndScalarFields();
    void _faultStateWithoutReasonHasFaultText();
    void _decodesIndependentFlags();
    void _finitePositionRequiresValidFlag();
    void _nanPositionIsInvalid();
    void _statusBecomesStale();
    void _modePolicyInputsSignalIsSelective();
    void _rejectsWrongComponentAndNonMonotonicStatus();
    void _hrtWrapIsForwardProgress();
    void _delayedPreWrapHrtDoesNotStartResetCandidate();
    void _bootTimeWrapIsForwardProgress();
    void _delayedPreWrapBootTimeDoesNotStartResetCandidate();
    void _rebootCandidatesRequireSameSelectedComponent();
    void _rebootStatusThenSystemTime();
    void _rebootSystemTimeThenStatus();
    void _rebootStatusThenFirstSystemTime();
    void _rebootFirstSystemTimeThenStatus();
    void _rebootBootTimeCanAdvancePastStaleBaseline();
    void _candidateExpiryRestoresOrdinaryFreshness();
    void _forwardStatusRestoresValidityAfterPayloadCommit();
    void _staleCandidateRecoveryEmitsValidityOnce();
    void _forwardSamplesClearOnlyTheirCandidate();
    void _sameBootOutOfOrderRollbacksDoNotConfirm();
    void _cachedRollbackBurstDoesNotConfirm();
    void _mismatchedBootEvidenceBlocksHrtFallback();
    void _fallbackRequiresThreeIncreasingLowHrtSamples();
};
