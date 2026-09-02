#pragma once

#include <QtCore/QList>

/// Allows to configure a set of mavlink streams to a specific rate,
/// and restore back to default.
/// Note that only one set is active at a time.
class MAVLinkStreamConfig
{
public:
    using SetMessageIntervalCb = std::function<void(int messageId, int rate)>;

    MAVLinkStreamConfig(const SetMessageIntervalCb& messageIntervalCb);
    ~MAVLinkStreamConfig();

    void setHighRateRateAndAttitude();
    void setHighRateVelAndPos();
    void setHighRateAltAirspeed();
    void setRoverRateTuning();
    void setRoverAttitudeTuning();
    void setRoverVelocityTuning();
    void setRoverPositionTuning();

    void restoreDefaults();
    void abort();
    void abortAndRestoreDefaults();

    void gotSetMessageIntervalAck();
    void gotSetMessageIntervalFailure();

    bool hasChangedStreams() const { return !_changedIds.empty(); }
    QList<int> takeChangedStreams();
    void adoptChangedStreams(const QList<int>& messageIds);

private:
    enum class State
    {
        Idle,
        RestoringDefaults,
        Configuring
    };

    struct DesiredStreamRate
    {
        const int messageId = 0;
        const int rate = 0;
    };

    void restoreNextDefault();
    void nextDesiredRate();
    void setSingleStream(int messageId, int rate);
    void setNextState(State state);

    State _state{State::Idle};
    QList<DesiredStreamRate> _desiredRates;
    QList<int> _changedIds;

    State _nextState{State::Idle};
    QList<DesiredStreamRate> _nextDesiredRates;

    const SetMessageIntervalCb _messageIntervalCb;
};
