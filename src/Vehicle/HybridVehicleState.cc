#include "HybridVehicleState.h"

#include <cmath>

namespace {

constexpr int kFreshnessTimeoutMs = 3000;
constexpr int kRebootPairingWindowMs = 3000;

HybridVehicleState::CurrentState decodeCurrentState(uint8_t state)
{
    switch (state) {
        case HYBRID_VEHICLE_STATE_QUAD:
            return HybridVehicleState::Quad;
        case HYBRID_VEHICLE_STATE_TRANSITIONING:
            return HybridVehicleState::Transitioning;
        case HYBRID_VEHICLE_STATE_ROVER:
            return HybridVehicleState::Rover;
        case HYBRID_VEHICLE_STATE_TRANSITION_FAULT:
            return HybridVehicleState::TransitionFault;
        default:
            return HybridVehicleState::Unknown;
    }
}

HybridVehicleState::TargetState decodeTargetState(uint8_t state)
{
    switch (state) {
        case HYBRID_VEHICLE_SHAPE_QUAD:
            return HybridVehicleState::TargetQuad;
        case HYBRID_VEHICLE_SHAPE_ROVER:
            return HybridVehicleState::TargetRover;
        default:
            return HybridVehicleState::None;
    }
}

}  // namespace

HybridVehicleState::HybridVehicleState(uint8_t selectedAutopilotComponentId, QObject* parent)
    : QObject(parent), _selectedAutopilotComponentId(selectedAutopilotComponentId)
{
    _monotonicClock.start();
    _freshnessTimer.setSingleShot(true);
    _freshnessTimer.setInterval(kFreshnessTimeoutMs);
    connect(&_freshnessTimer, &QTimer::timeout, this, [this]() {
        if (_stale) {
            return;
        }
        const bool wasValid = hasValidStatus();
        _stale = true;
        emit stateChanged();
        emit freshnessChanged();
        if (wasValid != hasValidStatus()) {
            emit statusValidityChanged();
        }
    });

    _candidateTimer.setSingleShot(true);
    _candidateTimer.setInterval(kRebootPairingWindowMs);
    connect(&_candidateTimer, &QTimer::timeout, this, &HybridVehicleState::_clearExpiredCandidates);
}

bool HybridVehicleState::canRequestTransform() const
{
    const bool stableShape = (_currentState == Quad) || (_currentState == Rover);
    return hasValidStatus() && stableShape && (_faultReason == HYBRID_VEHICLE_FAULT_NONE) && landed() &&
           landDetectionSampleFresh();
}

bool HybridVehicleState::_serialIsNewer(uint32_t candidate, uint32_t baseline)
{
    const uint32_t delta = candidate - baseline;
    return (delta != 0U) && (delta < 0x80000000U);
}

bool HybridVehicleState::_serialIsNewer(uint64_t candidate, uint64_t baseline)
{
    const uint64_t delta = candidate - baseline;
    return (delta != 0ULL) && (delta < 0x8000000000000000ULL);
}

bool HybridVehicleState::_candidateWindowExpired() const
{
    return (_candidateWindowStartMs >= 0) &&
           ((_monotonicClock.elapsed() - _candidateWindowStartMs) >= _candidateTimer.interval());
}

void HybridVehicleState::_beginCandidateWindow()
{
    if (!resetCandidateActive()) {
        _candidateWindowStartMs = _monotonicClock.elapsed();
        _candidateTimer.start();
    }
}

void HybridVehicleState::_emitCandidateValidityChanges(bool wasCandidateActive, bool wasValid)
{
    if (wasCandidateActive != resetCandidateActive()) {
        emit resetCandidateActiveChanged();
        emit stateChanged();
    }
    if (wasValid != hasValidStatus()) {
        emit statusValidityChanged();
    }
}

void HybridVehicleState::_clearExpiredCandidates()
{
    if (!resetCandidateActive()) {
        return;
    }
    const bool wasValid = hasValidStatus();
    _statusResetCandidate = false;
    _timeResetCandidate = false;
    _lowHrtSampleCount = 0;
    _candidateWindowStartMs = -1;
    emit resetCandidateActiveChanged();
    emit stateChanged();
    if (wasValid != hasValidStatus()) {
        emit statusValidityChanged();
    }
}

void HybridVehicleState::_setStatusCandidate(uint64_t timestamp)
{
    if (_candidateWindowExpired()) {
        _clearExpiredCandidates();
    }
    const bool wasCandidateActive = resetCandidateActive();
    const bool wasValid = hasValidStatus();
    _beginCandidateWindow();
    _statusResetCandidate = true;
    _statusCandidateTimestamp = timestamp;
    _lowHrtSampleCount = 1;
    emit requestQGCTimeSync();
    emit requestQGCTimeSync();
    _emitCandidateValidityChanges(wasCandidateActive, wasValid);
}

void HybridVehicleState::_setTimeCandidate(uint32_t timeBootMs)
{
    if (_candidateWindowExpired()) {
        _clearExpiredCandidates();
    }
    const bool wasCandidateActive = resetCandidateActive();
    const bool wasValid = hasValidStatus();
    _beginCandidateWindow();
    _timeResetCandidate = true;
    _timeCandidateBootMs = timeBootMs;
    _emitCandidateValidityChanges(wasCandidateActive, wasValid);
}

void HybridVehicleState::_confirmReboot()
{
    const uint64_t newStatusTimestamp = _statusCandidateTimestamp;
    const uint32_t newBootTime = _timeCandidateBootMs;
    const bool haveNewBootTime = _timeResetCandidate;

    resetForNewVehicleSession();
    _haveStatusTimestamp = true;
    _lastStatusTimestamp = newStatusTimestamp;
    if (haveNewBootTime) {
        _haveBootTime = true;
        _lastBootTimeMs = newBootTime;
    }
    emit rebootConfirmed();
}

void HybridVehicleState::_acceptStatus(const mavlink_hybrid_vehicle_status_t& status)
{
    const bool wasValid = hasValidStatus();
    const bool wasStale = _stale;
    _currentState = decodeCurrentState(status.current_state);
    _targetState = decodeTargetState(status.target_state);
    _transitionSequence = status.transition_sequence;
    _transitionElapsedMs = status.transition_elapsed_ms;
    _positionNormalized = status.position_normalized;
    _positionNormalizedValid = std::isfinite(status.position_normalized);
    _faultReason = status.fault_reason;
    _commandResult = status.command_result;
    _sensorSource = status.sensor_source;
    _actuatorBackend = status.actuator_backend;
    _actuatorProtectionFlags = status.actuator_protection_flags;
    _flags = status.flags;
    _commandTimestamp = status.command_timestamp;
    _localMonotonicStatusReceiptTime = _monotonicClock.elapsed();
    _haveStatus = true;
    _stale = false;
    _freshnessTimer.start();

    emit stateChanged();
    emit statusAccepted();
    emit freshStatusChanged();
    if (wasStale != _stale) {
        emit freshnessChanged();
    }
    if (wasValid != hasValidStatus()) {
        emit statusValidityChanged();
    }
}

void HybridVehicleState::handleStatus(uint8_t componentId, const mavlink_hybrid_vehicle_status_t& status)
{
    if (componentId != _selectedAutopilotComponentId) {
        return;
    }
    if (!_haveStatusTimestamp) {
        _haveStatusTimestamp = true;
        _lastStatusTimestamp = status.timestamp;
        _acceptStatus(status);
        return;
    }
    if (status.timestamp == _lastStatusTimestamp) {
        return;
    }
    if (_serialIsNewer(status.timestamp, _lastStatusTimestamp)) {
        const bool wasCandidateActive = resetCandidateActive();
        const bool wasValid = hasValidStatus();
        _statusResetCandidate = false;
        _lowHrtSampleCount = 0;
        _lastStatusTimestamp = status.timestamp;
        _emitCandidateValidityChanges(wasCandidateActive, wasValid);
        _acceptStatus(status);
        return;
    }

    if (status.timestamp < _lastStatusTimestamp) {
        if (!_statusResetCandidate) {
            _setStatusCandidate(status.timestamp);
        } else if (!_candidateWindowExpired() && (status.timestamp > _statusCandidateTimestamp)) {
            _statusCandidateTimestamp = status.timestamp;
            ++_lowHrtSampleCount;
        }
    }

    if (_statusResetCandidate && _timeResetCandidate) {
        _confirmReboot();
    } else if (_statusResetCandidate && (_lowHrtSampleCount == 3)) {
        _confirmReboot();
    }
}

void HybridVehicleState::handleSystemTime(uint8_t componentId, uint32_t timeBootMs)
{
    if (componentId != _selectedAutopilotComponentId) {
        return;
    }
    if (!_haveBootTime) {
        _haveBootTime = true;
        _lastBootTimeMs = timeBootMs;
        return;
    }
    if (timeBootMs == _lastBootTimeMs) {
        return;
    }
    if (_serialIsNewer(timeBootMs, _lastBootTimeMs)) {
        const bool wasCandidateActive = resetCandidateActive();
        const bool wasValid = hasValidStatus();
        _lastBootTimeMs = timeBootMs;
        _timeResetCandidate = false;
        _emitCandidateValidityChanges(wasCandidateActive, wasValid);
        return;
    }

    if (timeBootMs < _lastBootTimeMs) {
        _setTimeCandidate(timeBootMs);
        if (_statusResetCandidate && _timeResetCandidate) {
            _confirmReboot();
        }
    }
}

void HybridVehicleState::resetForNewVehicleSession()
{
    const bool wasValid = hasValidStatus();
    const bool wasStale = _stale;
    const bool wasCandidateActive = resetCandidateActive();
    _freshnessTimer.stop();
    _candidateTimer.stop();

    _currentState = Unknown;
    _targetState = None;
    _transitionSequence = 0;
    _transitionElapsedMs = 0;
    _positionNormalized = qQNaN();
    _positionNormalizedValid = false;
    _faultReason = HYBRID_VEHICLE_FAULT_NONE;
    _commandResult = MAV_RESULT_UNSUPPORTED;
    _sensorSource = HYBRID_VEHICLE_SENSOR_NONE;
    _actuatorBackend = HYBRID_VEHICLE_ACTUATOR_PWM;
    _actuatorProtectionFlags = 0;
    _flags = 0;
    _commandTimestamp = 0;
    _localMonotonicStatusReceiptTime = -1;
    _haveStatus = false;
    _stale = true;
    _haveStatusTimestamp = false;
    _lastStatusTimestamp = 0;
    _haveBootTime = false;
    _lastBootTimeMs = 0;
    _statusResetCandidate = false;
    _statusCandidateTimestamp = 0;
    _lowHrtSampleCount = 0;
    _timeResetCandidate = false;
    _timeCandidateBootMs = 0;
    _candidateWindowStartMs = -1;

    emit stateChanged();
    if (wasStale != _stale) {
        emit freshnessChanged();
    }
    if (wasCandidateActive) {
        emit resetCandidateActiveChanged();
    }
    if (wasValid != hasValidStatus()) {
        emit statusValidityChanged();
    }
}
