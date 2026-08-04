#include "HybridVehicleState.h"

#include <cmath>

namespace {

constexpr int kFreshnessTimeoutMs = 3000;
constexpr int kRebootPairingWindowMs = 3000;
constexpr int kMinimumRebootEvidenceSpanMs = 250;
constexpr int kPairedRebootSamples = 2;
constexpr int kFallbackHrtSamples = 3;
constexpr uint32_t kCandidateClockAgreementMs = 1500;

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
        _notifyModePolicyInputsChanged();
    });

    _candidateTimer.setSingleShot(true);
    _candidateTimer.setInterval(kRebootPairingWindowMs);
    connect(&_candidateTimer, &QTimer::timeout, this, [this]() {
        _clearExpiredCandidates();
        _notifyModePolicyInputsChanged();
    });
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

uint32_t HybridVehicleState::_serialDistance(uint32_t first, uint32_t second)
{
    const uint32_t forwardDistance = first - second;
    const uint32_t reverseDistance = second - first;
    return (forwardDistance < reverseDistance) ? forwardDistance : reverseDistance;
}

bool HybridVehicleState::_clocksConsistent(uint64_t statusTimestamp, uint32_t timeBootMs) const
{
    const uint32_t statusBootMs = static_cast<uint32_t>(statusTimestamp / 1000ULL);
    return _serialDistance(statusBootMs, timeBootMs) <= kCandidateClockAgreementMs;
}

bool HybridVehicleState::_candidateWindowExpired() const
{
    return (_candidateWindowStartMs >= 0) &&
           ((_monotonicClock.elapsed() - _candidateWindowStartMs) >= _candidateTimer.interval());
}

bool HybridVehicleState::_candidateEvidenceSpanSufficient(qint64 firstReceiptMs, qint64 lastReceiptMs) const
{
    return (firstReceiptMs >= 0) && ((lastReceiptMs - firstReceiptMs) >= kMinimumRebootEvidenceSpanMs);
}

bool HybridVehicleState::_candidateClocksConsistent() const
{
    return _clocksConsistent(_statusCandidateTimestamp, _timeCandidateBootMs);
}

void HybridVehicleState::_beginCandidateWindow()
{
    if (!resetCandidateActive()) {
        _bootObservedInCandidateWindow = false;
        _candidateWindowStartMs = _monotonicClock.elapsed();
        _candidateTimer.start();
    }
}

void HybridVehicleState::_finishCandidateWindowIfInactive()
{
    if (!resetCandidateActive()) {
        _candidateTimer.stop();
        _candidateWindowStartMs = -1;
        _bootObservedInCandidateWindow = false;
    }
}

void HybridVehicleState::_clearStatusCandidateTracking()
{
    _statusResetCandidate = false;
    _statusCandidateTimestamp = 0;
    _lowHrtSampleCount = 0;
    _statusCandidateFirstReceiptMs = -1;
    _statusCandidateLastReceiptMs = -1;
    _finishCandidateWindowIfInactive();
}

void HybridVehicleState::_clearTimeCandidateTracking()
{
    _timeResetCandidate = false;
    _timeCandidateBootMs = 0;
    _lowBootSampleCount = 0;
    _timeCandidateFirstReceiptMs = -1;
    _timeCandidateLastReceiptMs = -1;
    _finishCandidateWindowIfInactive();
}

void HybridVehicleState::_clearCandidateTracking()
{
    _candidateTimer.stop();
    _statusResetCandidate = false;
    _statusCandidateTimestamp = 0;
    _lowHrtSampleCount = 0;
    _statusCandidateFirstReceiptMs = -1;
    _statusCandidateLastReceiptMs = -1;
    _timeResetCandidate = false;
    _timeCandidateBootMs = 0;
    _lowBootSampleCount = 0;
    _timeCandidateFirstReceiptMs = -1;
    _timeCandidateLastReceiptMs = -1;
    _candidateWindowStartMs = -1;
    _bootObservedInCandidateWindow = false;
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
    _clearCandidateTracking();
    emit resetCandidateActiveChanged();
    emit stateChanged();
    if (wasValid != hasValidStatus()) {
        emit statusValidityChanged();
    }
}

void HybridVehicleState::_setStatusCandidate(uint64_t timestamp)
{
    const bool wasCandidateActive = resetCandidateActive();
    const bool wasValid = hasValidStatus();
    if (_candidateWindowExpired()) {
        _clearCandidateTracking();
    }
    _beginCandidateWindow();
    const qint64 receiptTimeMs = _monotonicClock.elapsed();
    _statusResetCandidate = true;
    _statusCandidateTimestamp = timestamp;
    _lowHrtSampleCount = 1;
    _statusCandidateFirstReceiptMs = receiptTimeMs;
    _statusCandidateLastReceiptMs = receiptTimeMs;
    emit requestQGCTimeSync();
    emit requestQGCTimeSync();

    if (_haveBootTime && _clocksConsistent(_statusCandidateTimestamp, _lastBootTimeMs) &&
        !_clocksConsistent(_lastStatusTimestamp, _lastBootTimeMs)) {
        _setTimeCandidate(_lastBootTimeMs);
    } else if (_haveBootTime && (_lastBootTimeReceiptMs >= 0) &&
               ((receiptTimeMs - _lastBootTimeReceiptMs) <= kRebootPairingWindowMs)) {
        _bootObservedInCandidateWindow = true;
    }

    _emitCandidateValidityChanges(wasCandidateActive, wasValid);
}

void HybridVehicleState::_setTimeCandidate(uint32_t timeBootMs)
{
    const bool wasCandidateActive = resetCandidateActive();
    const bool wasValid = hasValidStatus();
    if (_candidateWindowExpired()) {
        _clearCandidateTracking();
    }
    _beginCandidateWindow();
    const qint64 receiptTimeMs = _monotonicClock.elapsed();
    _timeResetCandidate = true;
    _timeCandidateBootMs = timeBootMs;
    _lowBootSampleCount = 1;
    _timeCandidateFirstReceiptMs = receiptTimeMs;
    _timeCandidateLastReceiptMs = receiptTimeMs;
    _bootObservedInCandidateWindow = true;
    _emitCandidateValidityChanges(wasCandidateActive, wasValid);
}

void HybridVehicleState::_trackTimeCandidate(uint32_t timeBootMs)
{
    if (!_timeResetCandidate || _candidateWindowExpired()) {
        _setTimeCandidate(timeBootMs);
    } else if (_serialIsNewer(timeBootMs, _timeCandidateBootMs)) {
        _timeCandidateBootMs = timeBootMs;
        if (_lowBootSampleCount < kPairedRebootSamples) {
            ++_lowBootSampleCount;
        }
        _timeCandidateLastReceiptMs = _monotonicClock.elapsed();
    } else if (timeBootMs < _timeCandidateBootMs) {
        const qint64 receiptTimeMs = _monotonicClock.elapsed();
        _timeCandidateBootMs = timeBootMs;
        _lowBootSampleCount = 1;
        _timeCandidateFirstReceiptMs = receiptTimeMs;
        _timeCandidateLastReceiptMs = receiptTimeMs;
    }
}

void HybridVehicleState::_evaluateRebootCandidate()
{
    const bool statusEvidenceReady =
        _statusResetCandidate && (_lowHrtSampleCount >= kPairedRebootSamples) &&
        _candidateEvidenceSpanSufficient(_statusCandidateFirstReceiptMs, _statusCandidateLastReceiptMs);
    const bool timeEvidenceReady =
        _timeResetCandidate && (_lowBootSampleCount >= kPairedRebootSamples) &&
        _candidateEvidenceSpanSufficient(_timeCandidateFirstReceiptMs, _timeCandidateLastReceiptMs);
    const bool pairedEvidenceReady = statusEvidenceReady && timeEvidenceReady && _candidateClocksConsistent();
    const bool fallbackEvidenceReady =
        _statusResetCandidate && !_timeResetCandidate && !_bootObservedInCandidateWindow &&
        (_lowHrtSampleCount >= kFallbackHrtSamples) &&
        _candidateEvidenceSpanSufficient(_statusCandidateFirstReceiptMs, _statusCandidateLastReceiptMs);

    if (pairedEvidenceReady || fallbackEvidenceReady) {
        _confirmReboot();
    }
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
        _lastBootTimeReceiptMs = _monotonicClock.elapsed();
    }
    emit rebootConfirmed();
}

void HybridVehicleState::_acceptStatus(const mavlink_hybrid_vehicle_status_t& status, bool deferStatusValidityChange)
{
    const bool wasValid = hasValidStatus();
    const bool wasStale = _stale;
    _currentState = decodeCurrentState(status.current_state);
    _targetState = decodeTargetState(status.target_state);
    _transitionSequence = status.transition_sequence;
    _transitionElapsedMs = status.transition_elapsed_ms;
    _positionNormalized = status.position_normalized;
    _positionNormalizedWireValid =
        std::isfinite(status.position_normalized) && ((status.flags & HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID) != 0);
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
    if (!deferStatusValidityChange && (wasValid != hasValidStatus())) {
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
        _notifyModePolicyInputsChanged();
        return;
    }
    if (status.timestamp == _lastStatusTimestamp) {
        if (resetCandidateActive()) {
            const bool wasValid = hasValidStatus();
            _clearStatusCandidateTracking();
            _emitCandidateValidityChanges(true, wasValid);
            _notifyModePolicyInputsChanged();
        }
        return;
    }
    if (_serialIsNewer(status.timestamp, _lastStatusTimestamp)) {
        const bool wasCandidateActive = resetCandidateActive();
        const bool wasValid = hasValidStatus();
        _clearStatusCandidateTracking();
        _lastStatusTimestamp = status.timestamp;
        _acceptStatus(status, true);
        _emitCandidateValidityChanges(wasCandidateActive, wasValid);
        _notifyModePolicyInputsChanged();
        return;
    }

    if (status.timestamp < _lastStatusTimestamp) {
        if (!_statusResetCandidate || _candidateWindowExpired()) {
            _setStatusCandidate(status.timestamp);
        } else if (_serialIsNewer(status.timestamp, _statusCandidateTimestamp)) {
            _statusCandidateTimestamp = status.timestamp;
            if (_lowHrtSampleCount < kFallbackHrtSamples) {
                ++_lowHrtSampleCount;
            }
            _statusCandidateLastReceiptMs = _monotonicClock.elapsed();
        } else if (status.timestamp < _statusCandidateTimestamp) {
            const qint64 receiptTimeMs = _monotonicClock.elapsed();
            _statusCandidateTimestamp = status.timestamp;
            _lowHrtSampleCount = 1;
            _statusCandidateFirstReceiptMs = receiptTimeMs;
            _statusCandidateLastReceiptMs = receiptTimeMs;
        }
    }

    _evaluateRebootCandidate();
    _notifyModePolicyInputsChanged();
}

void HybridVehicleState::handleSystemTime(uint8_t componentId, uint32_t timeBootMs)
{
    if (componentId != _selectedAutopilotComponentId) {
        return;
    }
    if (_statusResetCandidate && _clocksConsistent(_statusCandidateTimestamp, timeBootMs) &&
        !_clocksConsistent(_lastStatusTimestamp, timeBootMs)) {
        _trackTimeCandidate(timeBootMs);
        _evaluateRebootCandidate();
        _notifyModePolicyInputsChanged();
        return;
    }
    if (resetCandidateActive()) {
        _bootObservedInCandidateWindow = true;
    }
    if (!_haveBootTime) {
        _haveBootTime = true;
        _lastBootTimeMs = timeBootMs;
        _lastBootTimeReceiptMs = _monotonicClock.elapsed();
        return;
    }
    if (timeBootMs == _lastBootTimeMs) {
        _lastBootTimeReceiptMs = _monotonicClock.elapsed();
        if (resetCandidateActive()) {
            const bool wasValid = hasValidStatus();
            _clearTimeCandidateTracking();
            _emitCandidateValidityChanges(true, wasValid);
            _notifyModePolicyInputsChanged();
        }
        return;
    }
    if (_serialIsNewer(timeBootMs, _lastBootTimeMs)) {
        const bool wasCandidateActive = resetCandidateActive();
        const bool wasValid = hasValidStatus();
        _clearTimeCandidateTracking();
        _lastBootTimeMs = timeBootMs;
        _lastBootTimeReceiptMs = _monotonicClock.elapsed();
        _emitCandidateValidityChanges(wasCandidateActive, wasValid);
        _notifyModePolicyInputsChanged();
        return;
    }

    if (timeBootMs < _lastBootTimeMs) {
        _trackTimeCandidate(timeBootMs);
    }

    _evaluateRebootCandidate();
    _notifyModePolicyInputsChanged();
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
    _positionNormalizedWireValid = false;
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
    _lastBootTimeReceiptMs = -1;
    _clearCandidateTracking();

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
    _notifyModePolicyInputsChanged();
}

void HybridVehicleState::_notifyModePolicyInputsChanged()
{
    const bool statusValid = hasValidStatus();
    if ((_lastModePolicyStatusValid == statusValid) && (_lastModePolicyCurrentState == _currentState) &&
        (_lastModePolicyFaultReason == _faultReason)) {
        return;
    }

    _lastModePolicyStatusValid = statusValid;
    _lastModePolicyCurrentState = _currentState;
    _lastModePolicyFaultReason = _faultReason;
    emit modePolicyInputsChanged();
}
