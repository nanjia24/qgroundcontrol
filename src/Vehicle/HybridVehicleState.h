#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "MAVLinkLib.h"

class HybridVehicleState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CurrentState currentState READ currentState NOTIFY stateChanged)
    Q_PROPERTY(TargetState targetState READ targetState NOTIFY stateChanged)
    Q_PROPERTY(uint transitionSequence READ transitionSequence NOTIFY stateChanged)
    Q_PROPERTY(uint transitionElapsedMs READ transitionElapsedMs NOTIFY stateChanged)
    Q_PROPERTY(double positionNormalized READ positionNormalized NOTIFY stateChanged)
    Q_PROPERTY(bool positionNormalizedValid READ positionNormalizedValid NOTIFY stateChanged)
    Q_PROPERTY(uint faultReason READ faultReason NOTIFY stateChanged)
    Q_PROPERTY(uint commandResult READ commandResult NOTIFY stateChanged)
    Q_PROPERTY(uint sensorSource READ sensorSource NOTIFY stateChanged)
    Q_PROPERTY(uint actuatorBackend READ actuatorBackend NOTIFY stateChanged)
    Q_PROPERTY(uint actuatorProtectionFlags READ actuatorProtectionFlags NOTIFY stateChanged)
    Q_PROPERTY(uint flags READ flags NOTIFY stateChanged)
    Q_PROPERTY(bool sensorsEnabled READ sensorsEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool positionConfirmed READ positionConfirmed NOTIFY stateChanged)
    Q_PROPERTY(bool positionValid READ positionValid NOTIFY stateChanged)
    Q_PROPERTY(bool actuatorOnline READ actuatorOnline NOTIFY stateChanged)
    Q_PROPERTY(bool actuatorHealthy READ actuatorHealthy NOTIFY stateChanged)
    Q_PROPERTY(bool actuatorConfigVerified READ actuatorConfigVerified NOTIFY stateChanged)
    Q_PROPERTY(bool landed READ landed NOTIFY stateChanged)
    Q_PROPERTY(bool landDetectionSampleFresh READ landDetectionSampleFresh NOTIFY stateChanged)
    Q_PROPERTY(qulonglong commandTimestamp READ commandTimestamp NOTIFY stateChanged)
    Q_PROPERTY(qint64 localMonotonicStatusReceiptTime READ localMonotonicStatusReceiptTime NOTIFY stateChanged)
    Q_PROPERTY(bool hasValidStatus READ hasValidStatus NOTIFY statusValidityChanged)
    Q_PROPERTY(bool stale READ stale NOTIFY freshnessChanged)
    Q_PROPERTY(bool resetCandidateActive READ resetCandidateActive NOTIFY resetCandidateActiveChanged)
    Q_PROPERTY(bool canRequestTransform READ canRequestTransform NOTIFY stateChanged)

public:
    enum CurrentState
    {
        Quad = 0,
        Transitioning = 1,
        Rover = 2,
        Unknown = 3,
        TransitionFault = 4,
    };
    Q_ENUM(CurrentState)

    enum TargetState
    {
        None = 0,
        TargetQuad = 1,
        TargetRover = 2,
    };
    Q_ENUM(TargetState)

    explicit HybridVehicleState(uint8_t selectedAutopilotComponentId, QObject* parent = nullptr);

    CurrentState currentState() const { return _currentState; }

    TargetState targetState() const { return _targetState; }

    uint32_t transitionSequence() const { return _transitionSequence; }

    uint32_t transitionElapsedMs() const { return _transitionElapsedMs; }

    double positionNormalized() const { return _positionNormalized; }

    bool positionNormalizedValid() const { return _positionNormalizedWireValid && hasValidStatus(); }

    uint8_t faultReason() const { return _faultReason; }

    uint8_t commandResult() const { return _commandResult; }

    uint8_t sensorSource() const { return _sensorSource; }

    uint8_t actuatorBackend() const { return _actuatorBackend; }

    uint8_t actuatorProtectionFlags() const { return _actuatorProtectionFlags; }

    uint16_t flags() const { return _flags; }

    bool sensorsEnabled() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_SENSORS_ENABLED); }

    bool positionConfirmed() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_POSITION_CONFIRMED); }

    bool positionValid() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_POSITION_VALID); }

    bool actuatorOnline() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_ACTUATOR_ONLINE); }

    bool actuatorHealthy() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_ACTUATOR_HEALTHY); }

    bool actuatorConfigVerified() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_ACTUATOR_CONFIG_VERIFIED); }

    bool landed() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_LANDED); }

    bool landDetectionSampleFresh() const { return _flag(HYBRID_VEHICLE_STATUS_FLAGS_LAND_DETECTION_FRESH); }

    uint64_t commandTimestamp() const { return _commandTimestamp; }

    qint64 localMonotonicStatusReceiptTime() const { return _localMonotonicStatusReceiptTime; }

    bool hasValidStatus() const { return _haveStatus && !_stale && !resetCandidateActive(); }

    bool stale() const { return _stale; }

    bool resetCandidateActive() const { return _statusResetCandidate || _timeResetCandidate; }

    bool canRequestTransform() const;

    void handleStatus(uint8_t componentId, const mavlink_hybrid_vehicle_status_t& status);
    void handleSystemTime(uint8_t componentId, uint32_t timeBootMs);
    void resetForNewVehicleSession();

signals:
    void stateChanged();
    void statusValidityChanged();
    void freshnessChanged();
    void resetCandidateActiveChanged();
    void freshStatusChanged();
    void statusAccepted();
    void requestQGCTimeSync();
    void rebootConfirmed();
    void modePolicyInputsChanged();

private:
    static bool _serialIsNewer(uint32_t candidate, uint32_t baseline);
    static bool _serialIsNewer(uint64_t candidate, uint64_t baseline);
    static uint32_t _serialDistance(uint32_t first, uint32_t second);

    bool _flag(uint16_t mask) const { return (_flags & mask) != 0; }

    bool _clocksConsistent(uint64_t statusTimestamp, uint32_t timeBootMs) const;
    bool _candidateWindowExpired() const;
    bool _candidateEvidenceSpanSufficient(qint64 firstReceiptMs, qint64 lastReceiptMs) const;
    bool _candidateClocksConsistent() const;
    void _beginCandidateWindow();
    void _finishCandidateWindowIfInactive();
    void _clearStatusCandidateTracking();
    void _clearTimeCandidateTracking();
    void _clearCandidateTracking();
    void _clearExpiredCandidates();
    void _setStatusCandidate(uint64_t timestamp);
    void _setTimeCandidate(uint32_t timeBootMs);
    void _trackTimeCandidate(uint32_t timeBootMs);
    void _evaluateRebootCandidate();
    void _confirmReboot();
    void _acceptStatus(const mavlink_hybrid_vehicle_status_t& status, bool deferStatusValidityChange = false);
    void _emitCandidateValidityChanges(bool wasCandidateActive, bool wasValid);
    void _notifyModePolicyInputsChanged();

    const uint8_t _selectedAutopilotComponentId;
    QElapsedTimer _monotonicClock;
    QTimer _freshnessTimer;
    QTimer _candidateTimer;

    CurrentState _currentState = Unknown;
    TargetState _targetState = None;
    uint32_t _transitionSequence = 0;
    uint32_t _transitionElapsedMs = 0;
    double _positionNormalized = qQNaN();
    bool _positionNormalizedWireValid = false;
    uint8_t _faultReason = HYBRID_VEHICLE_FAULT_NONE;
    uint8_t _commandResult = MAV_RESULT_UNSUPPORTED;
    uint8_t _sensorSource = HYBRID_VEHICLE_SENSOR_NONE;
    uint8_t _actuatorBackend = HYBRID_VEHICLE_ACTUATOR_PWM;
    uint8_t _actuatorProtectionFlags = 0;
    uint16_t _flags = 0;
    uint64_t _commandTimestamp = 0;
    qint64 _localMonotonicStatusReceiptTime = -1;
    bool _haveStatus = false;
    bool _stale = true;

    bool _haveStatusTimestamp = false;
    uint64_t _lastStatusTimestamp = 0;
    bool _haveBootTime = false;
    uint32_t _lastBootTimeMs = 0;
    qint64 _lastBootTimeReceiptMs = -1;
    bool _statusResetCandidate = false;
    uint64_t _statusCandidateTimestamp = 0;
    int _lowHrtSampleCount = 0;
    qint64 _statusCandidateFirstReceiptMs = -1;
    qint64 _statusCandidateLastReceiptMs = -1;
    bool _timeResetCandidate = false;
    uint32_t _timeCandidateBootMs = 0;
    int _lowBootSampleCount = 0;
    qint64 _timeCandidateFirstReceiptMs = -1;
    qint64 _timeCandidateLastReceiptMs = -1;
    qint64 _candidateWindowStartMs = -1;
    bool _bootObservedInCandidateWindow = false;

    bool _lastModePolicyStatusValid = false;
    CurrentState _lastModePolicyCurrentState = Unknown;
    uint8_t _lastModePolicyFaultReason = HYBRID_VEHICLE_FAULT_NONE;
};
