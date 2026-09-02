#include "MAVLinkStreamConfig.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MAVLinkStreamConfigLog, "MAVLink.MAVLinkStreamConfig")

MAVLinkStreamConfig::MAVLinkStreamConfig(const SetMessageIntervalCb &messageIntervalCb)
    : _messageIntervalCb(messageIntervalCb)
{
    // qCDebug(MAVLinkStreamConfigLog) << Q_FUNC_INFO << this;
}

MAVLinkStreamConfig::~MAVLinkStreamConfig()
{
    // qCDebug(MAVLinkStreamConfigLog) << Q_FUNC_INFO << this;
}

void MAVLinkStreamConfig::setHighRateRateAndAttitude()
{
    static constexpr int requestedRate = static_cast<int>(1000000.0 / 100.0); // 100 Hz in usecs (better set this a bit higher than actually needed,
    // to give it more priority in case of exceeding link bandwidth)

    _nextDesiredRates = QList<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_ATTITUDE_QUATERNION, requestedRate},
        {MAVLINK_MSG_ID_ATTITUDE_TARGET, requestedRate},
    }};
    // we could check if we're already configured as requested

    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setHighRateVelAndPos()
{
    static constexpr int requestedRate = static_cast<int>(1000000.0 / 100.0);
    _nextDesiredRates = QList<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_LOCAL_POSITION_NED, requestedRate},
        {MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED, requestedRate},
    }};
    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setHighRateAltAirspeed()
{
    static constexpr int requestedRate = static_cast<int>(1000000.0 / 100.0);
    _nextDesiredRates = QList<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT, requestedRate},
        {MAVLINK_MSG_ID_VFR_HUD, requestedRate},
    }};
    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setRoverRateTuning()
{
    setSingleStream(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 20000);
}

void MAVLinkStreamConfig::setRoverAttitudeTuning()
{
    setSingleStream(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS, 33333);
}

void MAVLinkStreamConfig::setRoverVelocityTuning()
{
    setSingleStream(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS, 40000);
}

void MAVLinkStreamConfig::setRoverPositionTuning()
{
    setSingleStream(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 100000);
}

void MAVLinkStreamConfig::gotSetMessageIntervalAck()
{
    switch (_state) {
    case State::Configuring:
        nextDesiredRate();
        break;
    case State::RestoringDefaults:
        if (!_changedIds.empty()) {
            _changedIds.pop_back();
        }
        restoreNextDefault();
        break;
    default:
        break;
    }
}

void MAVLinkStreamConfig::gotSetMessageIntervalFailure()
{
    switch (_state) {
    case State::Configuring:
        abort();
        restoreDefaults();
        break;
    case State::RestoringDefaults:
        abort();
        break;
    case State::Idle:
        break;
    }
}

void MAVLinkStreamConfig::setNextState(State state)
{
    _nextState = state;
    if (_state == State::Idle) {
        // first restore defaults no matter what the next state is
        _state = State::RestoringDefaults;
        restoreNextDefault();
    }
}

void MAVLinkStreamConfig::nextDesiredRate()
{
    if (_nextState != State::Idle) { // allow state to be interrupted by a new request
        _desiredRates.clear();
        _state = State::RestoringDefaults;
        restoreNextDefault();
        return;
    }

    if (_desiredRates.empty()) {
        _state = State::Idle;
        return;
    }

    const DesiredStreamRate rate = _desiredRates.takeLast();
    _changedIds.push_back(rate.messageId);
    _messageIntervalCb(rate.messageId, rate.rate);
}

void MAVLinkStreamConfig::setSingleStream(int messageId, int rate)
{
    _nextDesiredRates = QList<DesiredStreamRate>{{messageId, rate}};
    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::restoreDefaults()
{
    setNextState(State::RestoringDefaults);
}

void MAVLinkStreamConfig::abort()
{
    _state = State::Idle;
    _desiredRates.clear();
    _nextState = State::Idle;
    _nextDesiredRates.clear();
}

void MAVLinkStreamConfig::abortAndRestoreDefaults()
{
    abort();
    restoreDefaults();
}

QList<int> MAVLinkStreamConfig::takeChangedStreams()
{
    abort();
    QList<int> messageIds;
    _changedIds.swap(messageIds);
    return messageIds;
}

void MAVLinkStreamConfig::adoptChangedStreams(const QList<int>& messageIds)
{
    abort();
    _changedIds = messageIds;
}

void MAVLinkStreamConfig::restoreNextDefault()
{
    if (_changedIds.empty()) {
        // do we have a pending request?
        switch (_nextState) {
        case State::Configuring:
            _state = _nextState;
            _desiredRates = _nextDesiredRates;
            _nextDesiredRates.clear();
            _nextState = State::Idle;
            nextDesiredRate();
            break;
        case State::RestoringDefaults:
        case State::Idle:
            _nextState = State::Idle; // nothing to do, we just finished restoring
            _state = State::Idle;
            break;
        }
        return;
    }

    const int id = _changedIds.last();
    _messageIntervalCb(id, 0); // restore the default rate
}
