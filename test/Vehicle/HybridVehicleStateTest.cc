#include "HybridVehicleStateTest.h"

#include <QtTest/QSignalSpy>
#include <array>
#include <cmath>
#include <limits>

#include "HybridVehicleState.h"
#include "MAVLinkLib.h"

namespace {

constexpr uint8_t kAutopilotComponent = MAV_COMP_ID_AUTOPILOT1;
constexpr uint8_t kWrongComponent = MAV_COMP_ID_CAMERA;
constexpr int kLiveRebootEvidenceWaitMs = 600;

mavlink_hybrid_vehicle_status_t makeStatus(uint64_t timestamp)
{
    mavlink_hybrid_vehicle_status_t status{};
    status.timestamp = timestamp;
    status.current_state = HYBRID_VEHICLE_STATE_QUAD;
    status.target_state = HYBRID_VEHICLE_SHAPE_NONE;
    status.fault_reason = HYBRID_VEHICLE_FAULT_NONE;
    status.position_normalized = 0.25F;
    status.flags = HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID;
    return status;
}

}  // namespace

void HybridVehicleStateTest::_initialStateIsUnknownAndStale()
{
    HybridVehicleState state(kAutopilotComponent);

    QCOMPARE(state.currentState(), HybridVehicleState::Unknown);
    QCOMPARE(state.targetState(), HybridVehicleState::None);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(state.stale());
    QVERIFY(!state.resetCandidateActive());
    QVERIFY(!state.canRequestTransform());
}

void HybridVehicleStateTest::_decodesStatesAndScalarFields()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy stateSpy(&state, &HybridVehicleState::stateChanged);
    QSignalSpy acceptedSpy(&state, &HybridVehicleState::statusAccepted);
    QSignalSpy freshSpy(&state, &HybridVehicleState::freshStatusChanged);
    auto status = makeStatus(1000);
    status.current_state = HYBRID_VEHICLE_STATE_TRANSITIONING;
    status.target_state = HYBRID_VEHICLE_SHAPE_ROVER;
    status.transition_sequence = 17;
    status.transition_elapsed_ms = 431;
    status.position_normalized = 0.75F;
    status.fault_reason = HYBRID_VEHICLE_FAULT_STALL;
    status.command_result = MAV_RESULT_TEMPORARILY_REJECTED;
    status.sensor_source = HYBRID_VEHICLE_SENSOR_TMAG5273;
    status.actuator_backend = HYBRID_VEHICLE_ACTUATOR_HX8;
    status.actuator_protection_flags = 0xA5;
    status.flags = 0x5A | HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID;
    status.command_timestamp = 998;

    state.handleStatus(kAutopilotComponent, status);

    QCOMPARE(state.currentState(), HybridVehicleState::Transitioning);
    QCOMPARE(state.targetState(), HybridVehicleState::TargetRover);
    QCOMPARE(state.transitionSequence(), 17U);
    QCOMPARE(state.transitionElapsedMs(), 431U);
    QCOMPARE(state.positionNormalized(), 0.75);
    QVERIFY(state.positionNormalizedValid());
    QCOMPARE(state.faultReason(), uint8_t(HYBRID_VEHICLE_FAULT_STALL));
    QCOMPARE(state.commandResult(), uint8_t(MAV_RESULT_TEMPORARILY_REJECTED));
    QCOMPARE(state.sensorSource(), uint8_t(HYBRID_VEHICLE_SENSOR_TMAG5273));
    QCOMPARE(state.actuatorBackend(), uint8_t(HYBRID_VEHICLE_ACTUATOR_HX8));
    QCOMPARE(state.actuatorProtectionFlags(), uint8_t(0xA5));
    QCOMPARE(state.flags(), uint16_t(0x5A | HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID));
    QCOMPARE(state.commandTimestamp(), uint64_t(998));
    QVERIFY(state.localMonotonicStatusReceiptTime() >= 0);
    QVERIFY(state.hasValidStatus());
    QVERIFY(!state.stale());
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(acceptedSpy.count(), 1);
    QCOMPARE(freshSpy.count(), 1);

    status.timestamp = 2000;
    status.current_state = HYBRID_VEHICLE_STATE_ROVER;
    status.target_state = HYBRID_VEHICLE_SHAPE_QUAD;
    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(state.currentState(), HybridVehicleState::Rover);
    QCOMPARE(state.targetState(), HybridVehicleState::TargetQuad);

    status.timestamp = 3000;
    status.current_state = HYBRID_VEHICLE_STATE_UNKNOWN;
    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(state.currentState(), HybridVehicleState::Unknown);

    status.timestamp = 4000;
    status.current_state = HYBRID_VEHICLE_STATE_TRANSITION_FAULT;
    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(state.currentState(), HybridVehicleState::TransitionFault);
}

void HybridVehicleStateTest::_decodesIndependentFlags()
{
    using FlagGetter = bool (HybridVehicleState::*)() const;
    const std::array<FlagGetter, 8> flagGetters = {
        &HybridVehicleState::sensorsEnabled,  &HybridVehicleState::positionConfirmed,
        &HybridVehicleState::positionValid,   &HybridVehicleState::actuatorOnline,
        &HybridVehicleState::actuatorHealthy, &HybridVehicleState::actuatorConfigVerified,
        &HybridVehicleState::landed,          &HybridVehicleState::landDetectionSampleFresh,
    };

    HybridVehicleState state(kAutopilotComponent);
    auto status = makeStatus(1000);
    for (size_t setBit = 0; setBit < flagGetters.size(); ++setBit) {
        status.timestamp += 1000;
        status.flags = static_cast<uint16_t>(1U << setBit);
        state.handleStatus(kAutopilotComponent, status);

        for (size_t checkedBit = 0; checkedBit < flagGetters.size(); ++checkedBit) {
            QCOMPARE((state.*flagGetters[checkedBit])(), checkedBit == setBit);
        }
    }

    status.timestamp += 1000;
    status.flags = HYBRID_VEHICLE_STATUS_FLAGS_LANDED | HYBRID_VEHICLE_STATUS_FLAGS_LAND_DETECTION_FRESH;
    status.actuator_protection_flags = 0xFF;
    state.handleStatus(kAutopilotComponent, status);
    QVERIFY(state.canRequestTransform());

    status.timestamp = 3000;
    status.current_state = HYBRID_VEHICLE_STATE_TRANSITION_FAULT;
    state.handleStatus(kAutopilotComponent, status);
    QVERIFY(!state.canRequestTransform());
}

void HybridVehicleStateTest::_nanPositionIsInvalid()
{
    HybridVehicleState state(kAutopilotComponent);
    auto status = makeStatus(1000);
    status.position_normalized = std::numeric_limits<float>::quiet_NaN();

    state.handleStatus(kAutopilotComponent, status);

    QVERIFY(!state.positionNormalizedValid());
    QVERIFY(qIsNaN(state.positionNormalized()));
}

void HybridVehicleStateTest::_finitePositionRequiresValidFlag()
{
    HybridVehicleState state(kAutopilotComponent);
    auto status = makeStatus(1000);
    status.flags &= ~HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID;

    state.handleStatus(kAutopilotComponent, status);

    QVERIFY(std::isfinite(state.positionNormalized()));
    QVERIFY(!state.positionNormalizedValid());

    status.timestamp = 2000;
    status.flags |= HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID;
    state.handleStatus(kAutopilotComponent, status);
    QVERIFY(state.positionNormalizedValid());
}

void HybridVehicleStateTest::_statusBecomesStale()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy stateSpy(&state, &HybridVehicleState::stateChanged);
    QSignalSpy freshnessSpy(&state, &HybridVehicleState::freshnessChanged);
    QSignalSpy modePolicySpy(&state, &HybridVehicleState::modePolicyInputsChanged);
    state.handleStatus(kAutopilotComponent, makeStatus(1000));
    QVERIFY(state.positionNormalizedValid());
    QCOMPARE(modePolicySpy.count(), 1);

    QTRY_VERIFY_WITH_TIMEOUT(state.stale(), 4000);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.positionNormalizedValid());
    QCOMPARE(stateSpy.count(), 2);
    QVERIFY(freshnessSpy.count() >= 2);
    QCOMPARE(modePolicySpy.count(), 2);
}

void HybridVehicleStateTest::_modePolicyInputsSignalIsSelective()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy modePolicySpy(&state, &HybridVehicleState::modePolicyInputsChanged);
    auto status = makeStatus(1000);

    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(modePolicySpy.count(), 1);

    status.timestamp = 2000;
    status.transition_elapsed_ms = 100;
    status.position_normalized = 0.5F;
    status.target_state = HYBRID_VEHICLE_SHAPE_ROVER;
    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(modePolicySpy.count(), 1);

    status.timestamp = 3000;
    status.fault_reason = HYBRID_VEHICLE_FAULT_STALL;
    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(modePolicySpy.count(), 2);

    status.timestamp = 4000;
    status.current_state = HYBRID_VEHICLE_STATE_ROVER;
    state.handleStatus(kAutopilotComponent, status);
    QCOMPARE(modePolicySpy.count(), 3);

    state.resetForNewVehicleSession();
    QCOMPARE(modePolicySpy.count(), 4);

    state.resetForNewVehicleSession();
    QCOMPARE(modePolicySpy.count(), 4);
}

void HybridVehicleStateTest::_rejectsWrongComponentAndNonMonotonicStatus()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy acceptedSpy(&state, &HybridVehicleState::statusAccepted);
    state.handleStatus(kWrongComponent, makeStatus(1000));
    QVERIFY(!state.hasValidStatus());

    state.handleStatus(kAutopilotComponent, makeStatus(1000));
    state.handleStatus(kAutopilotComponent, makeStatus(1000));
    QCOMPARE(acceptedSpy.count(), 1);
    QCOMPARE(state.commandTimestamp(), uint64_t(0));
}

void HybridVehicleStateTest::_bootTimeWrapIsForwardProgress()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 0xFFFFFFF0U);
    state.handleSystemTime(kAutopilotComponent, 0x00000010U);
    state.handleStatus(kAutopilotComponent, makeStatus(1000));

    QVERIFY(!state.resetCandidateActive());
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_hrtWrapIsForwardProgress()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleStatus(kAutopilotComponent, makeStatus(std::numeric_limits<uint64_t>::max() - 10));
    state.handleStatus(kAutopilotComponent, makeStatus(20));

    QVERIFY(!state.resetCandidateActive());
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_delayedPreWrapHrtDoesNotStartResetCandidate()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleStatus(kAutopilotComponent, makeStatus(std::numeric_limits<uint64_t>::max() - 10));
    state.handleStatus(kAutopilotComponent, makeStatus(20));
    state.handleStatus(kAutopilotComponent, makeStatus(std::numeric_limits<uint64_t>::max() - 9));

    QVERIFY(!state.resetCandidateActive());
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_delayedPreWrapBootTimeDoesNotStartResetCandidate()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 0xFFFFFFF0U);
    state.handleSystemTime(kAutopilotComponent, 0x00000010U);
    state.handleSystemTime(kAutopilotComponent, 0xFFFFFFF1U);
    state.handleStatus(kAutopilotComponent, makeStatus(1000));

    QVERIFY(!state.resetCandidateActive());
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_rebootCandidatesRequireSameSelectedComponent()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 5000);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleStatus(kAutopilotComponent, makeStatus(100));
    QVERIFY(state.resetCandidateActive());

    state.handleSystemTime(kWrongComponent, 20);

    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());
}

void HybridVehicleStateTest::_rebootStatusThenSystemTime()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    QSignalSpy syncSpy(&state, &HybridVehicleState::requestQGCTimeSync);
    state.handleSystemTime(kAutopilotComponent, 9000);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));
    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());
    QCOMPARE(syncSpy.count(), 2);

    state.handleSystemTime(kAutopilotComponent, 100);
    QCOMPARE(rebootSpy.count(), 0);
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleStatus(kAutopilotComponent, makeStatus(1100000));
    QCOMPARE(rebootSpy.count(), 0);
    state.handleSystemTime(kAutopilotComponent, 1100);

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.resetCandidateActive());
}

void HybridVehicleStateTest::_rebootSystemTimeThenStatus()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 9000);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));
    state.handleSystemTime(kAutopilotComponent, 100);
    QVERIFY(state.resetCandidateActive());

    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    QCOMPARE(rebootSpy.count(), 0);
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleSystemTime(kAutopilotComponent, 1100);
    QCOMPARE(rebootSpy.count(), 0);
    state.handleStatus(kAutopilotComponent, makeStatus(1100000));

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
}

void HybridVehicleStateTest::_rebootStatusThenFirstSystemTime()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));

    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    state.handleSystemTime(kAutopilotComponent, 100);
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleStatus(kAutopilotComponent, makeStatus(1100000));
    state.handleSystemTime(kAutopilotComponent, 1100);

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.resetCandidateActive());
}

void HybridVehicleStateTest::_rebootFirstSystemTimeThenStatus()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));

    state.handleSystemTime(kAutopilotComponent, 100);
    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleSystemTime(kAutopilotComponent, 1100);
    state.handleStatus(kAutopilotComponent, makeStatus(1100000));

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.resetCandidateActive());
}

void HybridVehicleStateTest::_rebootBootTimeCanAdvancePastStaleBaseline()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    state.handleSystemTime(kAutopilotComponent, 100);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));

    state.handleStatus(kAutopilotComponent, makeStatus(1100000));
    state.handleSystemTime(kAutopilotComponent, 1100);
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleStatus(kAutopilotComponent, makeStatus(2100000));
    state.handleSystemTime(kAutopilotComponent, 2100);

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.resetCandidateActive());
}

void HybridVehicleStateTest::_candidateExpiryRestoresOrdinaryFreshness()
{
    HybridVehicleState state(kAutopilotComponent);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));
    state.handleSystemTime(kAutopilotComponent, 9000);
    QVERIFY(state.positionNormalizedValid());
    QSignalSpy validitySpy(&state, &HybridVehicleState::statusValidityChanged);
    QSignalSpy modePolicySpy(&state, &HybridVehicleState::modePolicyInputsChanged);

    state.handleSystemTime(kAutopilotComponent, 100);
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.positionNormalizedValid());

    for (uint64_t timestamp = 10000000; timestamp <= 14000000; timestamp += 1000000) {
        QTest::qWait(700);
        state.handleStatus(kAutopilotComponent, makeStatus(timestamp));
    }
    QTRY_VERIFY_WITH_TIMEOUT(!state.resetCandidateActive(), 4000);
    QVERIFY(!state.stale());
    QVERIFY(state.hasValidStatus());
    QVERIFY(state.positionNormalizedValid());
    QCOMPARE(state.currentState(), HybridVehicleState::Quad);
    QCOMPARE(validitySpy.count(), 2);
    QCOMPARE(modePolicySpy.count(), 2);
}

void HybridVehicleStateTest::_forwardStatusRestoresValidityAfterPayloadCommit()
{
    HybridVehicleState state(kAutopilotComponent);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleStatus(kAutopilotComponent, makeStatus(100));
    QVERIFY(!state.hasValidStatus());

    QList<HybridVehicleState::CurrentState> validStateNotifications;
    connect(&state, &HybridVehicleState::stateChanged, &state, [&state, &validStateNotifications]() {
        if (state.hasValidStatus()) {
            validStateNotifications.append(state.currentState());
        }
    });

    mavlink_hybrid_vehicle_status_t forwardStatus = makeStatus(1000000);
    forwardStatus.current_state = HYBRID_VEHICLE_STATE_ROVER;
    state.handleStatus(kAutopilotComponent, forwardStatus);

    QVERIFY(state.hasValidStatus());
    QVERIFY(!validStateNotifications.isEmpty());
    for (const HybridVehicleState::CurrentState observedState : validStateNotifications) {
        QCOMPARE(observedState, HybridVehicleState::Rover);
    }
}

void HybridVehicleStateTest::_staleCandidateRecoveryEmitsValidityOnce()
{
    HybridVehicleState state(kAutopilotComponent);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));
    QTRY_VERIFY_WITH_TIMEOUT(state.stale(), 4000);

    QSignalSpy validitySpy(&state, &HybridVehicleState::statusValidityChanged);
    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    QVERIFY(state.resetCandidateActive());

    auto forwardStatus = makeStatus(10000000);
    forwardStatus.current_state = HYBRID_VEHICLE_STATE_ROVER;
    state.handleStatus(kAutopilotComponent, forwardStatus);

    QCOMPARE(validitySpy.count(), 1);
    QVERIFY(state.hasValidStatus());
    QCOMPARE(state.currentState(), HybridVehicleState::Rover);
}

void HybridVehicleStateTest::_forwardSamplesClearOnlyTheirCandidate()
{
    HybridVehicleState state(kAutopilotComponent);
    state.handleSystemTime(kAutopilotComponent, 9000);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));
    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    state.handleSystemTime(kAutopilotComponent, 100);
    QVERIFY(state.resetCandidateActive());

    state.handleStatus(kAutopilotComponent, makeStatus(10000000));
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());

    state.handleSystemTime(kAutopilotComponent, 10000);
    QVERIFY(!state.resetCandidateActive());
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_sameBootOutOfOrderRollbacksDoNotConfirm()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 9000);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));

    state.handleStatus(kAutopilotComponent, makeStatus(7000000));
    state.handleSystemTime(kAutopilotComponent, 7000);
    state.handleStatus(kAutopilotComponent, makeStatus(6000000));
    state.handleSystemTime(kAutopilotComponent, 6000);

    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());

    state.handleStatus(kAutopilotComponent, makeStatus(10000000));
    state.handleSystemTime(kAutopilotComponent, 10000);
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(!state.resetCandidateActive());
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_cachedRollbackBurstDoesNotConfirm()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 9000);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));

    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    state.handleStatus(kAutopilotComponent, makeStatus(1100000));
    state.handleStatus(kAutopilotComponent, makeStatus(2100000));
    state.handleSystemTime(kAutopilotComponent, 100);
    state.handleSystemTime(kAutopilotComponent, 1100);
    state.handleSystemTime(kAutopilotComponent, 2100);

    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());

    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleStatus(kAutopilotComponent, makeStatus(2100000));
    state.handleSystemTime(kAutopilotComponent, 2100);
    QCOMPARE(rebootSpy.count(), 0);

    state.handleStatus(kAutopilotComponent, makeStatus(10000000));
    state.handleSystemTime(kAutopilotComponent, 10000);
    QVERIFY(!state.resetCandidateActive());
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_mismatchedBootEvidenceBlocksHrtFallback()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 20000);
    state.handleStatus(kAutopilotComponent, makeStatus(20000000));

    state.handleStatus(kAutopilotComponent, makeStatus(1000000));
    state.handleSystemTime(kAutopilotComponent, 8000);
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleStatus(kAutopilotComponent, makeStatus(2000000));
    state.handleSystemTime(kAutopilotComponent, 9000);
    state.handleStatus(kAutopilotComponent, makeStatus(3000000));

    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());

    state.handleSystemTime(kAutopilotComponent, 21000);
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    state.handleStatus(kAutopilotComponent, makeStatus(4000000));
    QCOMPARE(rebootSpy.count(), 0);
    QVERIFY(state.resetCandidateActive());
    state.handleStatus(kAutopilotComponent, makeStatus(21000000));
    QVERIFY(!state.resetCandidateActive());
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_fallbackRequiresThreeIncreasingLowHrtSamples()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    QSignalSpy syncSpy(&state, &HybridVehicleState::requestQGCTimeSync);
    state.handleStatus(kAutopilotComponent, makeStatus(9000000));
    state.handleStatus(kAutopilotComponent, makeStatus(100000));
    QCOMPARE(syncSpy.count(), 2);
    QCOMPARE(rebootSpy.count(), 0);
    state.handleStatus(kAutopilotComponent, makeStatus(90000));
    QCOMPARE(rebootSpy.count(), 0);
    state.handleStatus(kAutopilotComponent, makeStatus(1090000));
    QCOMPARE(rebootSpy.count(), 0);
    QTest::qWait(kLiveRebootEvidenceWaitMs);
    state.handleStatus(kAutopilotComponent, makeStatus(2090000));

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
}

UT_REGISTER_TEST(HybridVehicleStateTest, TestLabel::Unit, TestLabel::Vehicle)
