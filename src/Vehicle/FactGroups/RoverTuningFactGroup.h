#pragma once

#include <QtCore/QTimer>

#include "FactGroup.h"

class Vehicle;

class RoverTuningFactGroup : public FactGroup
{
    Q_OBJECT

    Q_PROPERTY(Fact* rateYawResponse READ rateYawResponse CONSTANT)
    Q_PROPERTY(Fact* rateYawSetpoint READ rateYawSetpoint CONSTANT)
    Q_PROPERTY(Fact* rateIntegral READ rateIntegral CONSTANT)
    Q_PROPERTY(Fact* rateControlOutput READ rateControlOutput CONSTANT)
    Q_PROPERTY(Fact* rateValidFlags READ rateValidFlags CONSTANT)
    Q_PROPERTY(Fact* rateDriveType READ rateDriveType CONSTANT)
    Q_PROPERTY(Fact* rateTimestamp READ rateTimestamp CONSTANT)
    Q_PROPERTY(bool rateActive READ rateActive NOTIFY rateActiveChanged)
    Q_PROPERTY(bool rateAvailable READ rateAvailable NOTIFY rateAvailableChanged)
    Q_PROPERTY(quint32 rateResetCounter READ rateResetCounter NOTIFY rateResetCounterChanged)

    Q_PROPERTY(Fact* attitudeYawResponse READ attitudeYawResponse CONSTANT)
    Q_PROPERTY(Fact* attitudeYawSetpoint READ attitudeYawSetpoint CONSTANT)
    Q_PROPERTY(Fact* attitudeYawRateSetpoint READ attitudeYawRateSetpoint CONSTANT)
    Q_PROPERTY(Fact* attitudeValidFlags READ attitudeValidFlags CONSTANT)
    Q_PROPERTY(Fact* attitudeDriveType READ attitudeDriveType CONSTANT)
    Q_PROPERTY(Fact* attitudeTimestamp READ attitudeTimestamp CONSTANT)
    Q_PROPERTY(bool attitudeActive READ attitudeActive NOTIFY attitudeActiveChanged)
    Q_PROPERTY(bool attitudeAvailable READ attitudeAvailable NOTIFY attitudeAvailableChanged)
    Q_PROPERTY(quint32 attitudeResetCounter READ attitudeResetCounter NOTIFY attitudeResetCounterChanged)

    Q_PROPERTY(Fact* velocityBodyXResponse READ velocityBodyXResponse CONSTANT)
    Q_PROPERTY(Fact* velocityBodyXSetpoint READ velocityBodyXSetpoint CONSTANT)
    Q_PROPERTY(Fact* velocityIntegralBodyX READ velocityIntegralBodyX CONSTANT)
    Q_PROPERTY(Fact* velocityThrottleBodyX READ velocityThrottleBodyX CONSTANT)
    Q_PROPERTY(Fact* velocityBodyYResponse READ velocityBodyYResponse CONSTANT)
    Q_PROPERTY(Fact* velocityBodyYSetpoint READ velocityBodyYSetpoint CONSTANT)
    Q_PROPERTY(Fact* velocityIntegralBodyY READ velocityIntegralBodyY CONSTANT)
    Q_PROPERTY(Fact* velocityThrottleBodyY READ velocityThrottleBodyY CONSTANT)
    Q_PROPERTY(Fact* velocityValidFlags READ velocityValidFlags CONSTANT)
    Q_PROPERTY(Fact* velocityDriveType READ velocityDriveType CONSTANT)
    Q_PROPERTY(Fact* velocityTimestamp READ velocityTimestamp CONSTANT)
    Q_PROPERTY(bool velocityActive READ velocityActive NOTIFY velocityActiveChanged)
    Q_PROPERTY(bool velocityAvailable READ velocityAvailable NOTIFY velocityAvailableChanged)
    Q_PROPERTY(quint32 velocityResetCounter READ velocityResetCounter NOTIFY velocityResetCounterChanged)

    Q_PROPERTY(Fact* positionNorth READ positionNorth CONSTANT)
    Q_PROPERTY(Fact* positionEast READ positionEast CONSTANT)
    Q_PROPERTY(Fact* targetNorth READ targetNorth CONSTANT)
    Q_PROPERTY(Fact* targetEast READ targetEast CONSTANT)
    Q_PROPERTY(Fact* crosstrackError READ crosstrackError CONSTANT)
    Q_PROPERTY(Fact* lookaheadDistance READ lookaheadDistance CONSTANT)
    Q_PROPERTY(Fact* targetBearing READ targetBearing CONSTANT)
    Q_PROPERTY(Fact* distanceToWaypoint READ distanceToWaypoint CONSTANT)
    Q_PROPERTY(Fact* positionValidFlags READ positionValidFlags CONSTANT)
    Q_PROPERTY(Fact* positionDriveType READ positionDriveType CONSTANT)
    Q_PROPERTY(Fact* positionXYResetCounter READ positionXYResetCounter CONSTANT)
    Q_PROPERTY(Fact* positionTimestamp READ positionTimestamp CONSTANT)
    Q_PROPERTY(bool positionActive READ positionActive NOTIFY positionActiveChanged)
    Q_PROPERTY(bool positionAvailable READ positionAvailable NOTIFY positionAvailableChanged)
    Q_PROPERTY(bool positionPathAvailable READ positionPathAvailable NOTIFY positionPathAvailableChanged)
    Q_PROPERTY(quint32 positionResetCounter READ positionResetCounter NOTIFY positionResetCounterChanged)

public:
    explicit RoverTuningFactGroup(Vehicle* vehicle, QObject* parent = nullptr);

    Fact* rateYawResponse() { return &_rateYawResponseFact; }

    Fact* rateYawSetpoint() { return &_rateYawSetpointFact; }

    Fact* rateIntegral() { return &_rateIntegralFact; }

    Fact* rateControlOutput() { return &_rateControlOutputFact; }

    Fact* rateValidFlags() { return &_rateValidFlagsFact; }

    Fact* rateDriveType() { return &_rateDriveTypeFact; }

    Fact* rateTimestamp() { return &_rateTimestampFact; }

    bool rateActive() const { return _rateActive; }

    bool rateAvailable() const { return _rateAvailable; }

    quint32 rateResetCounter() const { return _rateResetCounter; }

    Fact* attitudeYawResponse() { return &_attitudeYawResponseFact; }

    Fact* attitudeYawSetpoint() { return &_attitudeYawSetpointFact; }

    Fact* attitudeYawRateSetpoint() { return &_attitudeYawRateSetpointFact; }

    Fact* attitudeValidFlags() { return &_attitudeValidFlagsFact; }

    Fact* attitudeDriveType() { return &_attitudeDriveTypeFact; }

    Fact* attitudeTimestamp() { return &_attitudeTimestampFact; }

    bool attitudeActive() const { return _attitudeActive; }

    bool attitudeAvailable() const { return _attitudeAvailable; }

    quint32 attitudeResetCounter() const { return _attitudeResetCounter; }

    Fact* velocityBodyXResponse() { return &_velocityBodyXResponseFact; }

    Fact* velocityBodyXSetpoint() { return &_velocityBodyXSetpointFact; }

    Fact* velocityIntegralBodyX() { return &_velocityIntegralBodyXFact; }

    Fact* velocityThrottleBodyX() { return &_velocityThrottleBodyXFact; }

    Fact* velocityBodyYResponse() { return &_velocityBodyYResponseFact; }

    Fact* velocityBodyYSetpoint() { return &_velocityBodyYSetpointFact; }

    Fact* velocityIntegralBodyY() { return &_velocityIntegralBodyYFact; }

    Fact* velocityThrottleBodyY() { return &_velocityThrottleBodyYFact; }

    Fact* velocityValidFlags() { return &_velocityValidFlagsFact; }

    Fact* velocityDriveType() { return &_velocityDriveTypeFact; }

    Fact* velocityTimestamp() { return &_velocityTimestampFact; }

    bool velocityActive() const { return _velocityActive; }

    bool velocityAvailable() const { return _velocityAvailable; }

    quint32 velocityResetCounter() const { return _velocityResetCounter; }

    Fact* positionNorth() { return &_positionNorthFact; }

    Fact* positionEast() { return &_positionEastFact; }

    Fact* targetNorth() { return &_targetNorthFact; }

    Fact* targetEast() { return &_targetEastFact; }

    Fact* crosstrackError() { return &_crosstrackErrorFact; }

    Fact* lookaheadDistance() { return &_lookaheadDistanceFact; }

    Fact* targetBearing() { return &_targetBearingFact; }

    Fact* distanceToWaypoint() { return &_distanceToWaypointFact; }

    Fact* positionValidFlags() { return &_positionValidFlagsFact; }

    Fact* positionDriveType() { return &_positionDriveTypeFact; }

    Fact* positionXYResetCounter() { return &_positionXYResetCounterFact; }

    Fact* positionTimestamp() { return &_positionTimestampFact; }

    bool positionActive() const { return _positionActive; }

    bool positionAvailable() const { return _positionAvailable; }

    bool positionPathAvailable() const { return _positionPathAvailable; }

    quint32 positionResetCounter() const { return _positionResetCounter; }

    void handleMessage(Vehicle* vehicle, const mavlink_message_t& message) final;

public slots:
    void communicationLost();

signals:
    void rateActiveChanged(bool active);
    void rateAvailableChanged(bool available);
    void rateResetCounterChanged(quint32 resetCounter);
    void attitudeActiveChanged(bool active);
    void attitudeAvailableChanged(bool available);
    void attitudeResetCounterChanged(quint32 resetCounter);
    void velocityActiveChanged(bool active);
    void velocityAvailableChanged(bool available);
    void velocityResetCounterChanged(quint32 resetCounter);
    void positionActiveChanged(bool active);
    void positionAvailableChanged(bool available);
    void positionPathAvailableChanged(bool available);
    void positionResetCounterChanged(quint32 resetCounter);

private:
    enum class Stream
    {
        Rate,
        Attitude,
        Velocity,
        Position,
    };

    bool _acceptTimestamp(Stream stream, quint64 timestamp);
    void _handleRateMessage(const mavlink_message_t& message);
    void _handleAttitudeMessage(const mavlink_message_t& message);
    void _handleVelocityMessage(const mavlink_message_t& message);
    void _handlePositionMessage(const mavlink_message_t& message);
    void _handleTimeout(Stream stream);

    void _invalidateRate(bool clearDiagnostics);
    void _invalidateAttitude(bool clearDiagnostics);
    void _invalidateVelocity(bool clearDiagnostics);
    void _invalidatePosition(bool clearDiagnostics);
    void _clearAll(bool resetGraphs);
    void _resetYawUnwrap();

    void _setRateActive(bool active);
    void _setRateAvailable(bool available);
    void _setAttitudeActive(bool active);
    void _setAttitudeAvailable(bool available);
    void _setVelocityActive(bool active);
    void _setVelocityAvailable(bool available);
    void _setPositionActive(bool active);
    void _setPositionAvailable(bool available);
    void _setPositionPathAvailable(bool available);
    void _updateTelemetryAvailable();
    void _incrementResetCounter(Stream stream);

    Vehicle* const _vehicle = nullptr;

    Fact _rateYawResponseFact = Fact(0, QStringLiteral("rateYawResponse"), FactMetaData::valueTypeDouble);
    Fact _rateYawSetpointFact = Fact(0, QStringLiteral("rateYawSetpoint"), FactMetaData::valueTypeDouble);
    Fact _rateIntegralFact = Fact(0, QStringLiteral("rateIntegral"), FactMetaData::valueTypeDouble);
    Fact _rateControlOutputFact = Fact(0, QStringLiteral("rateControlOutput"), FactMetaData::valueTypeDouble);
    Fact _rateValidFlagsFact = Fact(0, QStringLiteral("rateValidFlags"), FactMetaData::valueTypeUint16);
    Fact _rateDriveTypeFact = Fact(0, QStringLiteral("rateDriveType"), FactMetaData::valueTypeUint8);
    Fact _rateTimestampFact = Fact(0, QStringLiteral("rateTimestamp"), FactMetaData::valueTypeUint64);

    Fact _attitudeYawResponseFact = Fact(0, QStringLiteral("attitudeYawResponse"), FactMetaData::valueTypeDouble);
    Fact _attitudeYawSetpointFact = Fact(0, QStringLiteral("attitudeYawSetpoint"), FactMetaData::valueTypeDouble);
    Fact _attitudeYawRateSetpointFact =
        Fact(0, QStringLiteral("attitudeYawRateSetpoint"), FactMetaData::valueTypeDouble);
    Fact _attitudeValidFlagsFact = Fact(0, QStringLiteral("attitudeValidFlags"), FactMetaData::valueTypeUint16);
    Fact _attitudeDriveTypeFact = Fact(0, QStringLiteral("attitudeDriveType"), FactMetaData::valueTypeUint8);
    Fact _attitudeTimestampFact = Fact(0, QStringLiteral("attitudeTimestamp"), FactMetaData::valueTypeUint64);

    Fact _velocityBodyXResponseFact = Fact(0, QStringLiteral("velocityBodyXResponse"), FactMetaData::valueTypeDouble);
    Fact _velocityBodyXSetpointFact = Fact(0, QStringLiteral("velocityBodyXSetpoint"), FactMetaData::valueTypeDouble);
    Fact _velocityIntegralBodyXFact = Fact(0, QStringLiteral("velocityIntegralBodyX"), FactMetaData::valueTypeDouble);
    Fact _velocityThrottleBodyXFact = Fact(0, QStringLiteral("velocityThrottleBodyX"), FactMetaData::valueTypeDouble);
    Fact _velocityBodyYResponseFact = Fact(0, QStringLiteral("velocityBodyYResponse"), FactMetaData::valueTypeDouble);
    Fact _velocityBodyYSetpointFact = Fact(0, QStringLiteral("velocityBodyYSetpoint"), FactMetaData::valueTypeDouble);
    Fact _velocityIntegralBodyYFact = Fact(0, QStringLiteral("velocityIntegralBodyY"), FactMetaData::valueTypeDouble);
    Fact _velocityThrottleBodyYFact = Fact(0, QStringLiteral("velocityThrottleBodyY"), FactMetaData::valueTypeDouble);
    Fact _velocityValidFlagsFact = Fact(0, QStringLiteral("velocityValidFlags"), FactMetaData::valueTypeUint16);
    Fact _velocityDriveTypeFact = Fact(0, QStringLiteral("velocityDriveType"), FactMetaData::valueTypeUint8);
    Fact _velocityTimestampFact = Fact(0, QStringLiteral("velocityTimestamp"), FactMetaData::valueTypeUint64);

    Fact _positionNorthFact = Fact(0, QStringLiteral("positionNorth"), FactMetaData::valueTypeDouble);
    Fact _positionEastFact = Fact(0, QStringLiteral("positionEast"), FactMetaData::valueTypeDouble);
    Fact _targetNorthFact = Fact(0, QStringLiteral("targetNorth"), FactMetaData::valueTypeDouble);
    Fact _targetEastFact = Fact(0, QStringLiteral("targetEast"), FactMetaData::valueTypeDouble);
    Fact _crosstrackErrorFact = Fact(0, QStringLiteral("crosstrackError"), FactMetaData::valueTypeDouble);
    Fact _lookaheadDistanceFact = Fact(0, QStringLiteral("lookaheadDistance"), FactMetaData::valueTypeDouble);
    Fact _targetBearingFact = Fact(0, QStringLiteral("targetBearing"), FactMetaData::valueTypeDouble);
    Fact _distanceToWaypointFact = Fact(0, QStringLiteral("distanceToWaypoint"), FactMetaData::valueTypeDouble);
    Fact _positionValidFlagsFact = Fact(0, QStringLiteral("positionValidFlags"), FactMetaData::valueTypeUint16);
    Fact _positionDriveTypeFact = Fact(0, QStringLiteral("positionDriveType"), FactMetaData::valueTypeUint8);
    Fact _positionXYResetCounterFact = Fact(0, QStringLiteral("positionXYResetCounter"), FactMetaData::valueTypeUint8);
    Fact _positionTimestampFact = Fact(0, QStringLiteral("positionTimestamp"), FactMetaData::valueTypeUint64);

    QTimer _rateStaleTimer;
    QTimer _attitudeStaleTimer;
    QTimer _velocityStaleTimer;
    QTimer _positionStaleTimer;

    quint64 _lastRateTimestamp = 0;
    quint64 _lastAttitudeTimestamp = 0;
    quint64 _lastVelocityTimestamp = 0;
    quint64 _lastPositionTimestamp = 0;

    bool _rateActive = false;
    bool _rateAvailable = false;
    bool _attitudeActive = false;
    bool _attitudeAvailable = false;
    bool _velocityActive = false;
    bool _velocityAvailable = false;
    bool _positionActive = false;
    bool _positionAvailable = false;
    bool _positionPathAvailable = false;

    quint32 _rateResetCounter = 0;
    quint32 _attitudeResetCounter = 0;
    quint32 _velocityResetCounter = 0;
    quint32 _positionResetCounter = 0;

    bool _hasYawReference = false;
    double _lastUnwrappedYawResponseDeg = 0.;
    bool _hasXYResetCounter = false;
    quint8 _lastXYResetCounter = 0;
};
