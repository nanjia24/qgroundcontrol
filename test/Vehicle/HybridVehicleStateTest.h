#pragma once

#include "UnitTest.h"

class HybridVehicleStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _initialStateIsUnknownAndStale();
    void _decodesStatesAndScalarFields();
    void _decodesIndependentFlags();
    void _nanPositionIsInvalid();
    void _statusBecomesStale();
    void _rejectsWrongComponentAndNonMonotonicStatus();
    void _hrtWrapIsForwardProgress();
    void _delayedPreWrapHrtDoesNotStartResetCandidate();
    void _bootTimeWrapIsForwardProgress();
    void _delayedPreWrapBootTimeDoesNotStartResetCandidate();
    void _rebootCandidatesRequireSameSelectedComponent();
    void _rebootStatusThenSystemTime();
    void _rebootSystemTimeThenStatus();
    void _candidateExpiryRestoresOrdinaryFreshness();
    void _forwardSamplesClearOnlyTheirCandidate();
    void _fallbackRequiresThreeIncreasingLowHrtSamples();
};
