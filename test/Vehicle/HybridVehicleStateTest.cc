#include "HybridVehicleStateTest.h"

#include <QtTest/QSignalSpy>
#include <array>
#include <limits>

#include "HybridVehicleState.h"
#include "MAVLinkLib.h"

namespace {

constexpr uint8_t kAutopilotComponent = MAV_COMP_ID_AUTOPILOT1;
constexpr uint8_t kWrongComponent = MAV_COMP_ID_CAMERA;

mavlink_hybrid_vehicle_status_t makeStatus(uint64_t timestamp)
{
    mavlink_hybrid_vehicle_status_t status{};
    status.timestamp = timestamp;
    status.current_state = HYBRID_VEHICLE_STATE_QUAD;
    status.target_state = HYBRID_VEHICLE_SHAPE_NONE;
    status.fault_reason = HYBRID_VEHICLE_FAULT_NONE;
    status.position_normalized = 0.25F;
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
    status.flags = 0x5A;
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
    QCOMPARE(state.flags(), uint16_t(0x5A));
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

void HybridVehicleStateTest::_statusBecomesStale()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy stateSpy(&state, &HybridVehicleState::stateChanged);
    QSignalSpy freshnessSpy(&state, &HybridVehicleState::freshnessChanged);
    state.handleStatus(kAutopilotComponent, makeStatus(1000));

    QTRY_VERIFY_WITH_TIMEOUT(state.stale(), 4000);
    QVERIFY(!state.hasValidStatus());
    QCOMPARE(stateSpy.count(), 2);
    QVERIFY(freshnessSpy.count() >= 2);
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
    state.handleSystemTime(kAutopilotComponent, 5000);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleStatus(kAutopilotComponent, makeStatus(100));
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());
    QCOMPARE(syncSpy.count(), 2);

    state.handleSystemTime(kAutopilotComponent, 20);

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
    QVERIFY(!state.resetCandidateActive());
}

void HybridVehicleStateTest::_rebootSystemTimeThenStatus()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    state.handleSystemTime(kAutopilotComponent, 5000);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleSystemTime(kAutopilotComponent, 20);
    QVERIFY(state.resetCandidateActive());

    state.handleStatus(kAutopilotComponent, makeStatus(100));

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
}

void HybridVehicleStateTest::_candidateExpiryRestoresOrdinaryFreshness()
{
    HybridVehicleState state(kAutopilotComponent);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleStatus(kAutopilotComponent, makeStatus(100));
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());

    QTRY_VERIFY_WITH_TIMEOUT(!state.resetCandidateActive(), 4000);
    QVERIFY(state.stale());
    QCOMPARE(state.currentState(), HybridVehicleState::Quad);
}

void HybridVehicleStateTest::_forwardSamplesClearOnlyTheirCandidate()
{
    HybridVehicleState state(kAutopilotComponent);
    state.handleSystemTime(kAutopilotComponent, 5000);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleSystemTime(kAutopilotComponent, 20);
    QVERIFY(state.resetCandidateActive());

    state.handleStatus(kAutopilotComponent, makeStatus(901000));
    QVERIFY(state.resetCandidateActive());
    QVERIFY(!state.hasValidStatus());

    state.handleSystemTime(kAutopilotComponent, 6000);
    QVERIFY(!state.resetCandidateActive());
    QVERIFY(state.hasValidStatus());
}

void HybridVehicleStateTest::_fallbackRequiresThreeIncreasingLowHrtSamples()
{
    HybridVehicleState state(kAutopilotComponent);
    QSignalSpy rebootSpy(&state, &HybridVehicleState::rebootConfirmed);
    QSignalSpy syncSpy(&state, &HybridVehicleState::requestQGCTimeSync);
    state.handleStatus(kAutopilotComponent, makeStatus(900000));
    state.handleStatus(kAutopilotComponent, makeStatus(100));
    QCOMPARE(syncSpy.count(), 2);
    QCOMPARE(rebootSpy.count(), 0);
    state.handleStatus(kAutopilotComponent, makeStatus(90));
    QCOMPARE(rebootSpy.count(), 0);
    state.handleStatus(kAutopilotComponent, makeStatus(200));
    QCOMPARE(rebootSpy.count(), 0);
    state.handleStatus(kAutopilotComponent, makeStatus(300));

    QCOMPARE(rebootSpy.count(), 1);
    QVERIFY(!state.hasValidStatus());
}

UT_REGISTER_TEST(HybridVehicleStateTest, TestLabel::Unit, TestLabel::Vehicle)
