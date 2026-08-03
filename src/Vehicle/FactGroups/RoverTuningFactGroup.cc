#include "RoverTuningFactGroup.h"

#include <QtCore/QtMath>
#include <cmath>

#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RoverTuningFactGroupLog, "Vehicle.RoverTuningFactGroup")

namespace {

constexpr int kStaleTimeoutMSecs = 500;

bool flagIsSet(uint16_t flags, uint16_t flag)
{
    return (flags & flag) != 0;
}

bool validFiniteValue(uint16_t flags, uint16_t flag, float value)
{
    if (!flagIsSet(flags, flag)) {
        return false;
    }
    if (!std::isfinite(value)) {
        qCDebug(RoverTuningFactGroupLog) << "Rejected non-finite field with validity flag" << flag;
        return false;
    }
    return true;
}

double valueOrNaN(bool valid, float value)
{
    return valid ? static_cast<double>(value) : qQNaN();
}

double degreesOrNaN(bool valid, float radians)
{
    return valid ? qRadiansToDegrees(static_cast<double>(radians)) : qQNaN();
}

}  // namespace

RoverTuningFactGroup::RoverTuningFactGroup(Vehicle* vehicle, QObject* parent)
    : FactGroup(0, QStringLiteral(":/json/Vehicle/RoverTuningFactGroup.json"), parent), _vehicle(vehicle)
{
    Fact* const facts[] = {
        &_rateYawResponseFact,
        &_rateYawSetpointFact,
        &_rateIntegralFact,
        &_rateControlOutputFact,
        &_rateValidFlagsFact,
        &_rateDriveTypeFact,
        &_rateTimestampFact,
        &_attitudeYawResponseFact,
        &_attitudeYawSetpointFact,
        &_attitudeYawRateSetpointFact,
        &_attitudeValidFlagsFact,
        &_attitudeDriveTypeFact,
        &_attitudeTimestampFact,
        &_velocityBodyXResponseFact,
        &_velocityBodyXSetpointFact,
        &_velocityIntegralBodyXFact,
        &_velocityThrottleBodyXFact,
        &_velocityBodyYResponseFact,
        &_velocityBodyYSetpointFact,
        &_velocityIntegralBodyYFact,
        &_velocityThrottleBodyYFact,
        &_velocityValidFlagsFact,
        &_velocityDriveTypeFact,
        &_velocityTimestampFact,
        &_positionNorthFact,
        &_positionEastFact,
        &_targetNorthFact,
        &_targetEastFact,
        &_crosstrackErrorFact,
        &_lookaheadDistanceFact,
        &_targetBearingFact,
        &_distanceToWaypointFact,
        &_positionValidFlagsFact,
        &_positionDriveTypeFact,
        &_positionXYResetCounterFact,
        &_positionTimestampFact,
    };
    for (Fact* fact : facts) {
        _addFact(fact);
    }

    QTimer* const staleTimers[] = {
        &_rateStaleTimer,
        &_attitudeStaleTimer,
        &_velocityStaleTimer,
        &_positionStaleTimer,
    };
    for (QTimer* timer : staleTimers) {
        timer->setInterval(kStaleTimeoutMSecs);
        timer->setSingleShot(true);
    }

    connect(&_rateStaleTimer, &QTimer::timeout, this, [this]() { _handleTimeout(Stream::Rate); });
    connect(&_attitudeStaleTimer, &QTimer::timeout, this, [this]() { _handleTimeout(Stream::Attitude); });
    connect(&_velocityStaleTimer, &QTimer::timeout, this, [this]() { _handleTimeout(Stream::Velocity); });
    connect(&_positionStaleTimer, &QTimer::timeout, this, [this]() { _handleTimeout(Stream::Position); });

    _invalidateRate(true);
    _invalidateAttitude(true);
    _invalidateVelocity(true);
    _invalidatePosition(true);
}

void RoverTuningFactGroup::handleMessage(Vehicle* vehicle, const mavlink_message_t& message)
{
    if (vehicle != _vehicle) {
        return;
    }

    switch (message.msgid) {
        case MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS:
            _handleRateMessage(message);
            break;
        case MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS:
            _handleAttitudeMessage(message);
            break;
        case MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS:
            _handleVelocityMessage(message);
            break;
        case MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS:
            _handlePositionMessage(message);
            break;
        default:
            break;
    }
}

void RoverTuningFactGroup::communicationLost()
{
    _clearAll(true);
}

bool RoverTuningFactGroup::_acceptTimestamp(Stream stream, quint64 timestamp)
{
    if (timestamp == 0) {
        return false;
    }

    quint64* lastTimestamp = nullptr;
    switch (stream) {
        case Stream::Rate:
            lastTimestamp = &_lastRateTimestamp;
            break;
        case Stream::Attitude:
            lastTimestamp = &_lastAttitudeTimestamp;
            break;
        case Stream::Velocity:
            lastTimestamp = &_lastVelocityTimestamp;
            break;
        case Stream::Position:
            lastTimestamp = &_lastPositionTimestamp;
            break;
    }

    if (*lastTimestamp != 0 && timestamp < *lastTimestamp) {
        qCDebug(RoverTuningFactGroupLog) << "Source timestamp moved backwards for stream" << static_cast<int>(stream)
                                         << *lastTimestamp << timestamp;
        _clearAll(true);
        return false;
    }
    if (timestamp == *lastTimestamp) {
        return false;
    }

    *lastTimestamp = timestamp;
    return true;
}

void RoverTuningFactGroup::_handleRateMessage(const mavlink_message_t& message)
{
    mavlink_rover_rate_tuning_status_t status{};
    mavlink_msg_rover_rate_tuning_status_decode(&message, &status);
    if (!_acceptTimestamp(Stream::Rate, status.time_usec)) {
        return;
    }

    _rateValidFlagsFact.setRawValue(status.valid_flags);
    _rateDriveTypeFact.setRawValue(status.drive_type);

    const bool controllerActive = flagIsSet(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE);
    if (controllerActive && status.drive_type == ROVER_DRIVE_TYPE_UNKNOWN) {
        qCDebug(RoverTuningFactGroupLog) << "Active rate sample has UNKNOWN drive type";
    }
    if (!controllerActive || status.drive_type != ROVER_DRIVE_TYPE_DIFFERENTIAL) {
        _invalidateRate(false);
        return;
    }

    const bool responseValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID,
                                                status.yaw_rate_response);
    const bool setpointValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID,
                                                status.yaw_rate_setpoint);
    const bool integralValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_INTEGRAL_PRIMARY_VALID, status.integral);
    const bool outputValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_OUTPUT_PRIMARY_VALID, status.control_output);

    _rateYawResponseFact.setRawValue(degreesOrNaN(responseValid, status.yaw_rate_response));
    _rateYawSetpointFact.setRawValue(degreesOrNaN(setpointValid, status.yaw_rate_setpoint));
    _rateIntegralFact.setRawValue(valueOrNaN(integralValid, status.integral));
    _rateControlOutputFact.setRawValue(valueOrNaN(outputValid, status.control_output));
    _setRateActive(true);
    if (!(responseValid && setpointValid)) {
        _setRateAvailable(false);
    }
    _rateTimestampFact.setRawValue(static_cast<qulonglong>(status.time_usec));
    _setRateAvailable(responseValid && setpointValid);
    _rateStaleTimer.start();
}

void RoverTuningFactGroup::_handleAttitudeMessage(const mavlink_message_t& message)
{
    mavlink_rover_attitude_tuning_status_t status{};
    mavlink_msg_rover_attitude_tuning_status_decode(&message, &status);
    if (!_acceptTimestamp(Stream::Attitude, status.time_usec)) {
        return;
    }

    _attitudeValidFlagsFact.setRawValue(status.valid_flags);
    _attitudeDriveTypeFact.setRawValue(status.drive_type);

    const bool controllerActive = flagIsSet(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE);
    if (controllerActive && status.drive_type == ROVER_DRIVE_TYPE_UNKNOWN) {
        qCDebug(RoverTuningFactGroupLog) << "Active attitude sample has UNKNOWN drive type";
    }
    if (!controllerActive || status.drive_type != ROVER_DRIVE_TYPE_DIFFERENTIAL) {
        _invalidateAttitude(false);
        return;
    }

    const bool responseValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID, status.yaw_response);
    const bool setpointValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID, status.yaw_setpoint);
    const bool yawRateSetpointValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_OUTPUT_PRIMARY_VALID, status.yaw_rate_setpoint);

    double unwrappedResponseDeg = qQNaN();
    if (responseValid) {
        const double wrappedResponseDeg = qRadiansToDegrees(static_cast<double>(status.yaw_response));
        unwrappedResponseDeg =
            _hasYawReference
                ? _lastUnwrappedYawResponseDeg + std::remainder(wrappedResponseDeg - _lastUnwrappedYawResponseDeg, 360.)
                : wrappedResponseDeg;
        _lastUnwrappedYawResponseDeg = unwrappedResponseDeg;
        _hasYawReference = true;
    } else {
        _resetYawUnwrap();
    }

    double nearestSetpointDeg = qQNaN();
    if (responseValid && setpointValid) {
        const double wrappedSetpointDeg = qRadiansToDegrees(static_cast<double>(status.yaw_setpoint));
        nearestSetpointDeg = unwrappedResponseDeg + std::remainder(wrappedSetpointDeg - unwrappedResponseDeg, 360.);
    }

    _attitudeYawResponseFact.setRawValue(unwrappedResponseDeg);
    _attitudeYawSetpointFact.setRawValue(nearestSetpointDeg);
    _attitudeYawRateSetpointFact.setRawValue(degreesOrNaN(yawRateSetpointValid, status.yaw_rate_setpoint));
    _setAttitudeActive(true);
    if (!(responseValid && setpointValid)) {
        _setAttitudeAvailable(false);
    }
    _attitudeTimestampFact.setRawValue(static_cast<qulonglong>(status.time_usec));
    _setAttitudeAvailable(responseValid && setpointValid);
    _attitudeStaleTimer.start();
}

void RoverTuningFactGroup::_handleVelocityMessage(const mavlink_message_t& message)
{
    mavlink_rover_velocity_tuning_status_t status{};
    mavlink_msg_rover_velocity_tuning_status_decode(&message, &status);
    if (!_acceptTimestamp(Stream::Velocity, status.time_usec)) {
        return;
    }

    _velocityValidFlagsFact.setRawValue(status.valid_flags);
    _velocityDriveTypeFact.setRawValue(status.drive_type);

    const bool controllerActive = flagIsSet(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE);
    if (controllerActive && status.drive_type == ROVER_DRIVE_TYPE_UNKNOWN) {
        qCDebug(RoverTuningFactGroupLog) << "Active velocity sample has UNKNOWN drive type";
    }
    if (!controllerActive || status.drive_type != ROVER_DRIVE_TYPE_DIFFERENTIAL) {
        _invalidateVelocity(false);
        return;
    }

    const bool responseXValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID,
                                                 status.speed_body_x_response);
    const bool setpointXValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID,
                                                 status.speed_body_x_setpoint);
    const bool integralXValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_INTEGRAL_PRIMARY_VALID, status.integral_body_x);
    const bool outputXValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_OUTPUT_PRIMARY_VALID, status.throttle_body_x);
    const bool responseYValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_RESPONSE_SECONDARY_VALID,
                                                 status.speed_body_y_response);
    const bool setpointYValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_SETPOINT_SECONDARY_VALID,
                                                 status.speed_body_y_setpoint);
    const bool integralYValid = validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_INTEGRAL_SECONDARY_VALID,
                                                 status.integral_body_y);
    const bool outputYValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_OUTPUT_SECONDARY_VALID, status.throttle_body_y);

    _velocityBodyXResponseFact.setRawValue(valueOrNaN(responseXValid, status.speed_body_x_response));
    _velocityBodyXSetpointFact.setRawValue(valueOrNaN(setpointXValid, status.speed_body_x_setpoint));
    _velocityIntegralBodyXFact.setRawValue(valueOrNaN(integralXValid, status.integral_body_x));
    _velocityThrottleBodyXFact.setRawValue(valueOrNaN(outputXValid, status.throttle_body_x));
    _velocityBodyYResponseFact.setRawValue(valueOrNaN(responseYValid, status.speed_body_y_response));
    _velocityBodyYSetpointFact.setRawValue(valueOrNaN(setpointYValid, status.speed_body_y_setpoint));
    _velocityIntegralBodyYFact.setRawValue(valueOrNaN(integralYValid, status.integral_body_y));
    _velocityThrottleBodyYFact.setRawValue(valueOrNaN(outputYValid, status.throttle_body_y));
    _setVelocityActive(true);
    if (!(responseXValid && setpointXValid)) {
        _setVelocityAvailable(false);
    }
    _velocityTimestampFact.setRawValue(static_cast<qulonglong>(status.time_usec));
    _setVelocityAvailable(responseXValid && setpointXValid);
    _velocityStaleTimer.start();
}

void RoverTuningFactGroup::_handlePositionMessage(const mavlink_message_t& message)
{
    mavlink_rover_position_tuning_status_t status{};
    mavlink_msg_rover_position_tuning_status_decode(&message, &status);
    if (!_acceptTimestamp(Stream::Position, status.time_usec)) {
        return;
    }

    if (!_hasXYResetCounter) {
        _hasXYResetCounter = true;
        _lastXYResetCounter = status.xy_reset_counter;
    } else if (status.xy_reset_counter != _lastXYResetCounter) {
        _lastXYResetCounter = status.xy_reset_counter;
        _incrementResetCounter(Stream::Position);
    }

    _positionValidFlagsFact.setRawValue(status.valid_flags);
    _positionDriveTypeFact.setRawValue(status.drive_type);
    _positionXYResetCounterFact.setRawValue(status.xy_reset_counter);

    const bool controllerActive = flagIsSet(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE);
    if (controllerActive && status.drive_type == ROVER_DRIVE_TYPE_UNKNOWN) {
        qCDebug(RoverTuningFactGroupLog) << "Active position sample has UNKNOWN drive type";
    }
    if (!controllerActive || status.drive_type != ROVER_DRIVE_TYPE_DIFFERENTIAL) {
        _invalidatePosition(false);
        return;
    }

    const bool positionNorthValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_POSITION_VALID, status.position_north);
    const bool positionEastValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_POSITION_VALID, status.position_east);
    const bool targetNorthValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_TARGET_VALID, status.target_north);
    const bool targetEastValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_TARGET_VALID, status.target_east);
    const bool positionValid = positionNorthValid && positionEastValid;
    const bool targetValid = targetNorthValid && targetEastValid;
    const bool crosstrackValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_PATH_VALID, status.crosstrack_error);
    const bool lookaheadValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_PATH_VALID, status.lookahead_distance);
    const bool bearingValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_PATH_VALID, status.target_bearing);
    const bool distanceValid =
        validFiniteValue(status.valid_flags, ROVER_TUNING_STATUS_FLAGS_PATH_VALID, status.distance_to_waypoint);
    const bool pathAvailable = crosstrackValid && lookaheadValid && bearingValid && distanceValid;

    _positionNorthFact.setRawValue(valueOrNaN(positionValid, status.position_north));
    _positionEastFact.setRawValue(valueOrNaN(positionValid, status.position_east));
    _targetNorthFact.setRawValue(valueOrNaN(targetValid, status.target_north));
    _targetEastFact.setRawValue(valueOrNaN(targetValid, status.target_east));
    _crosstrackErrorFact.setRawValue(valueOrNaN(crosstrackValid, status.crosstrack_error));
    _lookaheadDistanceFact.setRawValue(valueOrNaN(lookaheadValid, status.lookahead_distance));
    _targetBearingFact.setRawValue(degreesOrNaN(bearingValid, status.target_bearing));
    _distanceToWaypointFact.setRawValue(valueOrNaN(distanceValid, status.distance_to_waypoint));
    _setPositionActive(true);
    if (_positionPathAvailable && !pathAvailable) {
        _incrementResetCounter(Stream::Position);
    }
    _setPositionPathAvailable(pathAvailable);
    if (!(positionValid && targetValid)) {
        _setPositionAvailable(false);
    }
    _positionTimestampFact.setRawValue(static_cast<qulonglong>(status.time_usec));
    _setPositionAvailable(positionValid && targetValid);
    _positionStaleTimer.start();
}

void RoverTuningFactGroup::_handleTimeout(Stream stream)
{
    switch (stream) {
        case Stream::Rate:
            _invalidateRate(true);
            break;
        case Stream::Attitude:
            _invalidateAttitude(true);
            break;
        case Stream::Velocity:
            _invalidateVelocity(true);
            break;
        case Stream::Position:
            _invalidatePosition(true);
            break;
    }
    _incrementResetCounter(stream);
}

void RoverTuningFactGroup::_invalidateRate(bool clearDiagnostics)
{
    _rateStaleTimer.stop();
    _rateYawResponseFact.setRawValue(qQNaN());
    _rateYawSetpointFact.setRawValue(qQNaN());
    _rateIntegralFact.setRawValue(qQNaN());
    _rateControlOutputFact.setRawValue(qQNaN());
    if (clearDiagnostics) {
        _rateValidFlagsFact.setRawValue(0);
        _rateDriveTypeFact.setRawValue(ROVER_DRIVE_TYPE_UNKNOWN);
    }
    _setRateActive(false);
    _setRateAvailable(false);
    _rateTimestampFact.setRawValue(static_cast<qulonglong>(0));
}

void RoverTuningFactGroup::_invalidateAttitude(bool clearDiagnostics)
{
    _attitudeStaleTimer.stop();
    _attitudeYawResponseFact.setRawValue(qQNaN());
    _attitudeYawSetpointFact.setRawValue(qQNaN());
    _attitudeYawRateSetpointFact.setRawValue(qQNaN());
    if (clearDiagnostics) {
        _attitudeValidFlagsFact.setRawValue(0);
        _attitudeDriveTypeFact.setRawValue(ROVER_DRIVE_TYPE_UNKNOWN);
    }
    _resetYawUnwrap();
    _setAttitudeActive(false);
    _setAttitudeAvailable(false);
    _attitudeTimestampFact.setRawValue(static_cast<qulonglong>(0));
}

void RoverTuningFactGroup::_invalidateVelocity(bool clearDiagnostics)
{
    _velocityStaleTimer.stop();
    _velocityBodyXResponseFact.setRawValue(qQNaN());
    _velocityBodyXSetpointFact.setRawValue(qQNaN());
    _velocityIntegralBodyXFact.setRawValue(qQNaN());
    _velocityThrottleBodyXFact.setRawValue(qQNaN());
    _velocityBodyYResponseFact.setRawValue(qQNaN());
    _velocityBodyYSetpointFact.setRawValue(qQNaN());
    _velocityIntegralBodyYFact.setRawValue(qQNaN());
    _velocityThrottleBodyYFact.setRawValue(qQNaN());
    if (clearDiagnostics) {
        _velocityValidFlagsFact.setRawValue(0);
        _velocityDriveTypeFact.setRawValue(ROVER_DRIVE_TYPE_UNKNOWN);
    }
    _setVelocityActive(false);
    _setVelocityAvailable(false);
    _velocityTimestampFact.setRawValue(static_cast<qulonglong>(0));
}

void RoverTuningFactGroup::_invalidatePosition(bool clearDiagnostics)
{
    _positionStaleTimer.stop();
    _positionNorthFact.setRawValue(qQNaN());
    _positionEastFact.setRawValue(qQNaN());
    _targetNorthFact.setRawValue(qQNaN());
    _targetEastFact.setRawValue(qQNaN());
    _crosstrackErrorFact.setRawValue(qQNaN());
    _lookaheadDistanceFact.setRawValue(qQNaN());
    _targetBearingFact.setRawValue(qQNaN());
    _distanceToWaypointFact.setRawValue(qQNaN());
    if (clearDiagnostics) {
        _positionValidFlagsFact.setRawValue(0);
        _positionDriveTypeFact.setRawValue(ROVER_DRIVE_TYPE_UNKNOWN);
        _positionXYResetCounterFact.setRawValue(0);
    }
    _setPositionActive(false);
    _setPositionAvailable(false);
    _setPositionPathAvailable(false);
    _positionTimestampFact.setRawValue(static_cast<qulonglong>(0));
}

void RoverTuningFactGroup::_clearAll(bool resetGraphs)
{
    _invalidateRate(true);
    _invalidateAttitude(true);
    _invalidateVelocity(true);
    _invalidatePosition(true);
    _lastRateTimestamp = 0;
    _lastAttitudeTimestamp = 0;
    _lastVelocityTimestamp = 0;
    _lastPositionTimestamp = 0;
    _hasXYResetCounter = false;
    _lastXYResetCounter = 0;

    if (resetGraphs) {
        _incrementResetCounter(Stream::Rate);
        _incrementResetCounter(Stream::Attitude);
        _incrementResetCounter(Stream::Velocity);
        _incrementResetCounter(Stream::Position);
    }
}

void RoverTuningFactGroup::_resetYawUnwrap()
{
    _hasYawReference = false;
    _lastUnwrappedYawResponseDeg = 0.;
}

void RoverTuningFactGroup::_setRateActive(bool active)
{
    if (_rateActive != active) {
        _rateActive = active;
        emit rateActiveChanged(active);
    }
}

void RoverTuningFactGroup::_setRateAvailable(bool available)
{
    if (_rateAvailable != available) {
        _rateAvailable = available;
        emit rateAvailableChanged(available);
        _updateTelemetryAvailable();
    }
}

void RoverTuningFactGroup::_setAttitudeActive(bool active)
{
    if (_attitudeActive != active) {
        _attitudeActive = active;
        emit attitudeActiveChanged(active);
    }
}

void RoverTuningFactGroup::_setAttitudeAvailable(bool available)
{
    if (_attitudeAvailable != available) {
        _attitudeAvailable = available;
        emit attitudeAvailableChanged(available);
        _updateTelemetryAvailable();
    }
}

void RoverTuningFactGroup::_setVelocityActive(bool active)
{
    if (_velocityActive != active) {
        _velocityActive = active;
        emit velocityActiveChanged(active);
    }
}

void RoverTuningFactGroup::_setVelocityAvailable(bool available)
{
    if (_velocityAvailable != available) {
        _velocityAvailable = available;
        emit velocityAvailableChanged(available);
        _updateTelemetryAvailable();
    }
}

void RoverTuningFactGroup::_setPositionActive(bool active)
{
    if (_positionActive != active) {
        _positionActive = active;
        emit positionActiveChanged(active);
    }
}

void RoverTuningFactGroup::_setPositionAvailable(bool available)
{
    if (_positionAvailable != available) {
        _positionAvailable = available;
        emit positionAvailableChanged(available);
        _updateTelemetryAvailable();
    }
}

void RoverTuningFactGroup::_setPositionPathAvailable(bool available)
{
    if (_positionPathAvailable != available) {
        _positionPathAvailable = available;
        emit positionPathAvailableChanged(available);
    }
}

void RoverTuningFactGroup::_updateTelemetryAvailable()
{
    _setTelemetryAvailable(_rateAvailable || _attitudeAvailable || _velocityAvailable || _positionAvailable);
}

void RoverTuningFactGroup::_incrementResetCounter(Stream stream)
{
    switch (stream) {
        case Stream::Rate:
            emit rateResetCounterChanged(++_rateResetCounter);
            break;
        case Stream::Attitude:
            emit attitudeResetCounterChanged(++_attitudeResetCounter);
            break;
        case Stream::Velocity:
            emit velocityResetCounterChanged(++_velocityResetCounter);
            break;
        case Stream::Position:
            emit positionResetCounterChanged(++_positionResetCounter);
            break;
    }
}
