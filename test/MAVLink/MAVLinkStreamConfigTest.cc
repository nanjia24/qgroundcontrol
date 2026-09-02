#include "MAVLinkStreamConfigTest.h"

#include "MAVLinkLib.h"
#include "MAVLinkStreamConfig.h"

#include <QtCore/QPair>
#include <QtCore/QList>
#include <array>

namespace {

// 100 Hz in microseconds — must match the static constexpr in MAVLinkStreamConfig.cc
constexpr int kHighRate = static_cast<int>(1000000.0 / 100.0);

struct RoverStreamExpectation
{
    void (MAVLinkStreamConfig::*configure)();
    int messageId;
    int intervalUs;
};

constexpr std::array<RoverStreamExpectation, 4> kRoverStreams{{
    {&MAVLinkStreamConfig::setRoverRateTuning, MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 20000},
    {&MAVLinkStreamConfig::setRoverAttitudeTuning, MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS, 33333},
    {&MAVLinkStreamConfig::setRoverVelocityTuning, MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS, 40000},
    {&MAVLinkStreamConfig::setRoverPositionTuning, MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 100000},
}};

} // namespace

// Construction: no callback is invoked, object starts in Idle state (verified
// indirectly by the absence of any callback and by the behaviour of subsequent calls).
void MAVLinkStreamConfigTest::_testInitialState()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    // No callback should fire on construction.
    QVERIFY(calls.isEmpty());

    // Calling gotSetMessageIntervalAck() in Idle is a no-op.
    config.gotSetMessageIntervalAck();
    QVERIFY(calls.isEmpty());
}

// setHighRateRateAndAttitude() from Idle with no previously configured messages:
//   - No restore phase (changedIds is empty), transitions immediately to Configuring.
//   - First callback fires synchronously during the configure call itself.
//   - Each subsequent gotSetMessageIntervalAck() advances to the next message.
//   - Messages are emitted in last-in / first-out order from _desiredRates.
void MAVLinkStreamConfigTest::_testSetHighRateRateAndAttitude()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setHighRateRateAndAttitude();

    // First message fires synchronously.
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[0].second, kHighRate);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
    QCOMPARE(calls[1].second, kHighRate);

    // All messages consumed; further ack is a no-op.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// setHighRateVelAndPos() follows the same pattern with different message IDs.
void MAVLinkStreamConfigTest::_testSetHighRateVelAndPos()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setHighRateVelAndPos();

    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED);
    QCOMPARE(calls[0].second, kHighRate);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_LOCAL_POSITION_NED);
    QCOMPARE(calls[1].second, kHighRate);

    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// setHighRateAltAirspeed() follows the same pattern with different message IDs.
void MAVLinkStreamConfigTest::_testSetHighRateAltAirspeed()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setHighRateAltAirspeed();

    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_VFR_HUD);
    QCOMPARE(calls[0].second, kHighRate);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT);
    QCOMPARE(calls[1].second, kHighRate);

    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// After fully configuring two messages, restoreDefaults() should emit rate=0
// for each previously configured message ID (in reverse push order).
void MAVLinkStreamConfigTest::_testRestoreDefaultsAfterConfigure()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    // Configure attitude messages fully.
    config.setHighRateRateAndAttitude();     // callback[0]: ATTITUDE_TARGET, kHighRate
    config.gotSetMessageIntervalAck();       // callback[1]: ATTITUDE_QUATERNION, kHighRate
    config.gotSetMessageIntervalAck();       // → Idle, _changedIds = [ATTITUDE_TARGET, ATTITUDE_QUATERNION]
    QCOMPARE(calls.size(), 2);

    calls.clear();
    config.restoreDefaults();

    // Restore fires synchronously for the last pushed ID (ATTITUDE_QUATERNION was pushed second).
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
    QCOMPARE(calls[0].second, 0);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[1].second, 0);

    // Both restored; further ack is a no-op.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);

    // Each Rover page configures one stream and restores that same stream through
    // the normal ACK-driven path.
    for (const RoverStreamExpectation& stream : kRoverStreams) {
        QList<QPair<int, int>> roverCalls;
        MAVLinkStreamConfig roverConfig([&roverCalls](int id, int rate) { roverCalls.append({id, rate}); });

        (roverConfig.*stream.configure)();
        QCOMPARE(roverCalls.size(), 1);
        QCOMPARE(roverCalls[0].first, stream.messageId);
        QCOMPARE(roverCalls[0].second, stream.intervalUs);

        roverConfig.gotSetMessageIntervalAck();
        QCOMPARE(roverCalls.size(), 1);

        roverConfig.restoreDefaults();
        QCOMPARE(roverCalls.size(), 2);
        QCOMPARE(roverCalls[1].first, stream.messageId);
        QCOMPARE(roverCalls[1].second, 0);

        roverConfig.gotSetMessageIntervalAck();
        roverConfig.gotSetMessageIntervalAck();
        QCOMPARE(roverCalls.size(), 2);
    }
}

// restoreDefaults() from Idle (no previously configured messages) is a no-op.
void MAVLinkStreamConfigTest::_testRestoreDefaultsFromIdle()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.restoreDefaults();
    QVERIFY(calls.isEmpty());

    config.gotSetMessageIntervalAck();
    QVERIFY(calls.isEmpty());

    config.abortAndRestoreDefaults();
    QVERIFY(calls.isEmpty());

    // Abort restores every stream which was already requested, one ACK at a time.
    config.setHighRateRateAndAttitude();
    config.gotSetMessageIntervalAck();
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);

    calls.clear();
    config.abortAndRestoreDefaults();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first, MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
    QCOMPARE(calls[0].second, 0);

    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first, MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[1].second, 0);
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

void MAVLinkStreamConfigTest::_testRestoreFailureRetainsDebt()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setRoverRateTuning();
    config.gotSetMessageIntervalAck();
    config.restoreDefaults();
    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls.last(), qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 0));

    config.gotSetMessageIntervalFailure();
    config.restoreDefaults();
    QCOMPARE(calls.size(), 3);
    QCOMPARE(calls.last(), qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 0));

    config.gotSetMessageIntervalAck();
    config.restoreDefaults();
    QCOMPARE(calls.size(), 3);
}

// Calling setHighRateVelAndPos() while setHighRateRateAndAttitude() is mid-flight
// (one message already sent, one still pending) should:
//   1. Abandon the pending ATTITUDE_QUATERNION message.
//   2. Restore the already-sent ATTITUDE_TARGET (rate=0).
//   3. Then configure the vel+pos messages.
void MAVLinkStreamConfigTest::_testInterruptConfigureWithNewRequest()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    // Start attitude configure — first message fires synchronously.
    config.setHighRateRateAndAttitude();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[0].second, kHighRate);

    // Interrupt before sending the second message.
    config.setHighRateVelAndPos();

    // The interrupt is staged; no extra callbacks yet.
    QCOMPARE(calls.size(), 1);

    // Next ack detects a pending _nextState and begins restoring.
    // ATTITUDE_TARGET (the only changedId) gets rate=0 synchronously inside nextDesiredRate.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[1].second, 0);

    // Ack for the restore → changedIds empty → switch to Configuring vel+pos.
    // POSITION_TARGET_LOCAL_NED fires synchronously.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 3);
    QCOMPARE(calls[2].first,  MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED);
    QCOMPARE(calls[2].second, kHighRate);

    // Ack → LOCAL_POSITION_NED.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 4);
    QCOMPARE(calls[3].first,  MAVLINK_MSG_ID_LOCAL_POSITION_NED);
    QCOMPARE(calls[3].second, kHighRate);

    // Ack → Idle, no more calls.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 4);

    // Switching Rover pages first restores the old single stream, then starts
    // the newly requested stream after the restore ACK.
    QList<QPair<int, int>> roverCalls;
    MAVLinkStreamConfig roverConfig([&roverCalls](int id, int rate) { roverCalls.append({id, rate}); });

    roverConfig.setRoverRateTuning();
    roverConfig.setRoverAttitudeTuning();
    QCOMPARE(roverCalls.size(), 1);
    QCOMPARE(roverCalls[0], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 20000));

    roverConfig.gotSetMessageIntervalAck();
    QCOMPARE(roverCalls.size(), 2);
    QCOMPARE(roverCalls[1], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 0));

    roverConfig.gotSetMessageIntervalAck();
    QCOMPARE(roverCalls.size(), 3);
    QCOMPARE(roverCalls[2], qMakePair(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS, 33333));

    roverConfig.gotSetMessageIntervalAck();
    QCOMPARE(roverCalls.size(), 3);

    // Abort discards the pending page, restores the stream already requested,
    // and ignores ACKs from the abandoned operation.
    QList<QPair<int, int>> abortCalls;
    MAVLinkStreamConfig abortConfig([&abortCalls](int id, int rate) { abortCalls.append({id, rate}); });

    abortConfig.setRoverRateTuning();
    abortConfig.setRoverAttitudeTuning();
    abortConfig.abortAndRestoreDefaults();
    QCOMPARE(abortCalls.size(), 2);
    QCOMPARE(abortCalls[0], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 20000));
    QCOMPARE(abortCalls[1], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 0));

    abortConfig.gotSetMessageIntervalAck();
    abortConfig.gotSetMessageIntervalAck();
    QCOMPARE(abortCalls.size(), 2);

    // A fresh configuration starts immediately after abort. Aborting it after
    // its ACK restores it once and still leaves the state reusable.
    abortConfig.setRoverVelocityTuning();
    QCOMPARE(abortCalls.size(), 3);
    QCOMPARE(abortCalls[2], qMakePair(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS, 40000));
    abortConfig.gotSetMessageIntervalAck();
    abortConfig.abortAndRestoreDefaults();
    QCOMPARE(abortCalls.size(), 4);
    QCOMPARE(abortCalls[3], qMakePair(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS, 0));
    abortConfig.gotSetMessageIntervalAck();
    QCOMPARE(abortCalls.size(), 4);

    abortConfig.setRoverPositionTuning();
    QCOMPARE(abortCalls.size(), 5);
    QCOMPARE(abortCalls[4], qMakePair(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 100000));
}

void MAVLinkStreamConfigTest::_testSynchronousCallbackReentrancy()
{
    QList<QPair<int, int>> calls;
    MAVLinkStreamConfig* configPtr = nullptr;
    bool failSynchronously = true;
    MAVLinkStreamConfig config([&](int id, int rate) {
        calls.append({id, rate});
        if (failSynchronously) {
            configPtr->gotSetMessageIntervalFailure();
        } else {
            configPtr->gotSetMessageIntervalAck();
        }
    });
    configPtr = &config;

    // A queue-level rejection can invoke the result handler before the configure
    // callback returns. The failed configure immediately attempts one restore,
    // which also fails synchronously, without invalidating nextDesiredRate state.
    config.setRoverRateTuning();
    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[0], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 20000));
    QCOMPARE(calls[1], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 0));
    QVERIFY(config.hasChangedStreams());

    failSynchronously = false;
    config.restoreDefaults();
    QCOMPARE(calls.size(), 3);
    QCOMPARE(calls[2], qMakePair(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 0));
    QVERIFY(!config.hasChangedStreams());

    config.setRoverAttitudeTuning();
    QCOMPARE(calls.size(), 4);
    QCOMPARE(calls[3], qMakePair(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS, 33333));
    QVERIFY(config.hasChangedStreams());
}

UT_REGISTER_TEST(MAVLinkStreamConfigTest, TestLabel::Unit)
