#include "RoverTuningFactGroupTest.h"

#include <QtCore/QtMath>
#include <cmath>
#include <limits>

#include "MAVLinkLib.h"
#include "RoverTuningFactGroup.h"

static_assert(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS == 60100);
static_assert(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS_LEN == 27);
static_assert(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS_CRC == 147);
static_assert(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS == 60101);
static_assert(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS_LEN == 23);
static_assert(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS_CRC == 85);
static_assert(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS == 60102);
static_assert(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS_LEN == 43);
static_assert(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS_CRC == 217);
static_assert(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS == 60103);
static_assert(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS_LEN == 44);
static_assert(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS_CRC == 90);

namespace {

constexpr uint16_t kRateFlags = static_cast<uint16_t>(
    ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE | ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID | ROVER_TUNING_STATUS_FLAGS_INTEGRAL_PRIMARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_OUTPUT_PRIMARY_VALID);

constexpr uint16_t kAttitudeFlags = static_cast<uint16_t>(
    ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE | ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID | ROVER_TUNING_STATUS_FLAGS_OUTPUT_PRIMARY_VALID);

constexpr uint16_t kVelocityFlags = static_cast<uint16_t>(
    ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE | ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID | ROVER_TUNING_STATUS_FLAGS_RESPONSE_SECONDARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_SETPOINT_SECONDARY_VALID | ROVER_TUNING_STATUS_FLAGS_INTEGRAL_PRIMARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_INTEGRAL_SECONDARY_VALID | ROVER_TUNING_STATUS_FLAGS_OUTPUT_PRIMARY_VALID |
    ROVER_TUNING_STATUS_FLAGS_OUTPUT_SECONDARY_VALID);

constexpr uint16_t kPositionFlags =
    static_cast<uint16_t>(ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE | ROVER_TUNING_STATUS_FLAGS_POSITION_VALID |
                          ROVER_TUNING_STATUS_FLAGS_TARGET_VALID | ROVER_TUNING_STATUS_FLAGS_PATH_VALID);

Vehicle* owner(quintptr value)
{
    return reinterpret_cast<Vehicle*>(value);
}

mavlink_message_t rateMessage(quint64 timestamp, uint16_t flags = kRateFlags,
                              uint8_t driveType = ROVER_DRIVE_TYPE_DIFFERENTIAL, float response = 1.F,
                              float setpoint = 0.5F, float integral = 0.25F, float output = 0.75F)
{
    mavlink_message_t message{};
    (void)mavlink_msg_rover_rate_tuning_status_pack(1, 1, &message, timestamp, response, setpoint, integral, output,
                                                    flags, driveType);
    return message;
}

mavlink_message_t attitudeMessage(quint64 timestamp, float response, float setpoint, float yawRateSetpoint = 0.25F,
                                  uint16_t flags = kAttitudeFlags, uint8_t driveType = ROVER_DRIVE_TYPE_DIFFERENTIAL)
{
    mavlink_message_t message{};
    (void)mavlink_msg_rover_attitude_tuning_status_pack(1, 1, &message, timestamp, response, setpoint, yawRateSetpoint,
                                                        flags, driveType);
    return message;
}

mavlink_message_t velocityMessage(quint64 timestamp, uint16_t flags = kVelocityFlags,
                                  uint8_t driveType = ROVER_DRIVE_TYPE_DIFFERENTIAL)
{
    mavlink_message_t message{};
    (void)mavlink_msg_rover_velocity_tuning_status_pack(1, 1, &message, timestamp, 1.F, 2.F, 3.F, 4.F, 5.F, 6.F, 7.F,
                                                        8.F, flags, driveType);
    return message;
}

mavlink_message_t positionMessage(quint64 timestamp, uint8_t xyResetCounter, uint16_t flags = kPositionFlags,
                                  uint8_t driveType = ROVER_DRIVE_TYPE_DIFFERENTIAL)
{
    mavlink_message_t message{};
    (void)mavlink_msg_rover_position_tuning_status_pack(1, 1, &message, timestamp, 1.F, 2.F, 3.F, 4.F, 5.F, 6.F, 0.5F,
                                                        8.F, flags, driveType, xyResetCounter);
    return message;
}

bool fuzzyCompare(double actual, double expected, double tolerance = 1e-4)
{
    return std::abs(actual - expected) <= tolerance;
}

bool factIsNaN(Fact* fact)
{
    return std::isnan(fact->rawValue().toDouble());
}

}  // namespace

void RoverTuningFactGroupTest::_protocolContract_test()
{
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS), 60100);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS_LEN), 27);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS_CRC), 147);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS), 60101);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS_LEN), 23);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS_CRC), 85);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS), 60102);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS_LEN), 43);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS_CRC), 217);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS), 60103);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS_LEN), 44);
    QCOMPARE(static_cast<int>(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS_CRC), 90);

    const mavlink_message_t rate = rateMessage(101);
    QCOMPARE(static_cast<int>(rate.len), static_cast<int>(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS_LEN));
    mavlink_rover_rate_tuning_status_t decodedRate{};
    mavlink_msg_rover_rate_tuning_status_decode(&rate, &decodedRate);
    QCOMPARE(decodedRate.time_usec, UINT64_C(101));
    QCOMPARE(decodedRate.control_output, 0.75F);

    const mavlink_message_t attitude = attitudeMessage(102, 0.1F, 0.2F);
    QCOMPARE(static_cast<int>(attitude.len), static_cast<int>(MAVLINK_MSG_ID_ROVER_ATTITUDE_TUNING_STATUS_LEN));
    mavlink_rover_attitude_tuning_status_t decodedAttitude{};
    mavlink_msg_rover_attitude_tuning_status_decode(&attitude, &decodedAttitude);
    QCOMPARE(decodedAttitude.time_usec, UINT64_C(102));
    QCOMPARE(decodedAttitude.yaw_setpoint, 0.2F);

    const mavlink_message_t velocity = velocityMessage(103);
    QCOMPARE(static_cast<int>(velocity.len), static_cast<int>(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS_LEN));
    mavlink_rover_velocity_tuning_status_t decodedVelocity{};
    mavlink_msg_rover_velocity_tuning_status_decode(&velocity, &decodedVelocity);
    QCOMPARE(decodedVelocity.time_usec, UINT64_C(103));
    QCOMPARE(decodedVelocity.throttle_body_y, 8.F);

    const mavlink_message_t position = positionMessage(104, 9);
    QCOMPARE(static_cast<int>(position.len), static_cast<int>(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS_LEN));
    mavlink_rover_position_tuning_status_t decodedPosition{};
    mavlink_msg_rover_position_tuning_status_decode(&position, &decodedPosition);
    QCOMPARE(decodedPosition.time_usec, UINT64_C(104));
    QCOMPARE(static_cast<int>(decodedPosition.xy_reset_counter), 9);
}

void RoverTuningFactGroupTest::_rateValidation_test()
{
    Vehicle* const vehicle = owner(1);
    RoverTuningFactGroup group(vehicle);

    group.handleMessage(vehicle, rateMessage(100, kRateFlags, ROVER_DRIVE_TYPE_DIFFERENTIAL, qDegreesToRadians(90.F),
                                             qDegreesToRadians(45.F), 0.3F, 0.4F));
    QVERIFY(group.rateActive());
    QVERIFY(group.rateAvailable());
    QVERIFY(fuzzyCompare(group.rateYawResponse()->rawValue().toDouble(), 90.));
    QVERIFY(fuzzyCompare(group.rateYawSetpoint()->rawValue().toDouble(), 45.));
    QCOMPARE(group.rateTimestamp()->rawValue().toULongLong(), 100ULL);

    group.handleMessage(vehicle, rateMessage(100, kRateFlags, ROVER_DRIVE_TYPE_DIFFERENTIAL, 2.F, 2.F));
    QVERIFY(fuzzyCompare(group.rateYawResponse()->rawValue().toDouble(), 90.));

    const float nan = std::numeric_limits<float>::quiet_NaN();
    group.handleMessage(vehicle, rateMessage(101, kRateFlags, ROVER_DRIVE_TYPE_DIFFERENTIAL, nan, 0.5F));
    QVERIFY(group.rateActive());
    QVERIFY(!group.rateAvailable());
    QVERIFY(factIsNaN(group.rateYawResponse()));
    QVERIFY(!factIsNaN(group.rateYawSetpoint()));

    qulonglong timestampWhenAvailable = 0;
    connect(&group, &RoverTuningFactGroup::rateAvailableChanged, this, [&](bool available) {
        if (available) {
            timestampWhenAvailable = group.rateTimestamp()->rawValue().toULongLong();
        }
    });
    group.handleMessage(vehicle, rateMessage(102));
    QCOMPARE(timestampWhenAvailable, 102ULL);

    const quint32 resetCounter = group.rateResetCounter();
    group.handleMessage(vehicle, rateMessage(103, kRateFlags, ROVER_DRIVE_TYPE_ACKERMANN));
    QVERIFY(!group.rateActive());
    QVERIFY(!group.rateAvailable());
    QCOMPARE(group.rateDriveType()->rawValue().toUInt(), static_cast<uint>(ROVER_DRIVE_TYPE_ACKERMANN));
    QCOMPARE(group.rateResetCounter(), resetCounter);

    group.handleMessage(vehicle, rateMessage(104, kRateFlags, ROVER_DRIVE_TYPE_MECANUM));
    QVERIFY(!group.rateAvailable());
    QCOMPARE(group.rateDriveType()->rawValue().toUInt(), static_cast<uint>(ROVER_DRIVE_TYPE_MECANUM));

    group.handleMessage(vehicle, rateMessage(105, 0, ROVER_DRIVE_TYPE_UNKNOWN, nan, nan, nan, nan));
    QVERIFY(!group.rateActive());
    QCOMPARE(group.rateValidFlags()->rawValue().toUInt(), 0U);
    QCOMPARE(group.rateResetCounter(), resetCounter);
}

void RoverTuningFactGroupTest::_attitudeYawUnwrap_test()
{
    Vehicle* const vehicle = owner(1);
    RoverTuningFactGroup group(vehicle);

    group.handleMessage(
        vehicle, attitudeMessage(200, qDegreesToRadians(179.F), qDegreesToRadians(-179.F), qDegreesToRadians(180.F)));
    QVERIFY(group.attitudeAvailable());
    QVERIFY(fuzzyCompare(group.attitudeYawResponse()->rawValue().toDouble(), 179.));
    QVERIFY(fuzzyCompare(group.attitudeYawSetpoint()->rawValue().toDouble(), 181.));
    QVERIFY(fuzzyCompare(group.attitudeYawRateSetpoint()->rawValue().toDouble(), 180.));

    group.handleMessage(vehicle, attitudeMessage(201, qDegreesToRadians(-179.F), qDegreesToRadians(179.F)));
    QVERIFY(fuzzyCompare(group.attitudeYawResponse()->rawValue().toDouble(), 181.));
    QVERIFY(fuzzyCompare(group.attitudeYawSetpoint()->rawValue().toDouble(), 179.));

    group.handleMessage(vehicle, attitudeMessage(202, 0.F, 0.F, 0.F, 0, ROVER_DRIVE_TYPE_UNKNOWN));
    QVERIFY(!group.attitudeActive());
    group.handleMessage(vehicle, attitudeMessage(203, qDegreesToRadians(-170.F), qDegreesToRadians(-160.F)));
    QVERIFY(fuzzyCompare(group.attitudeYawResponse()->rawValue().toDouble(), -170.));
    QVERIFY(fuzzyCompare(group.attitudeYawSetpoint()->rawValue().toDouble(), -160.));
}

void RoverTuningFactGroupTest::_velocityAndPosition_test()
{
    Vehicle* const vehicle = owner(1);
    RoverTuningFactGroup group(vehicle);

    group.handleMessage(vehicle, velocityMessage(300));
    QVERIFY(group.velocityAvailable());
    QCOMPARE(group.velocityBodyXResponse()->rawValue().toDouble(), 1.);
    QCOMPARE(group.velocityBodyXSetpoint()->rawValue().toDouble(), 2.);
    QCOMPARE(group.velocityIntegralBodyX()->rawValue().toDouble(), 3.);
    QCOMPARE(group.velocityThrottleBodyX()->rawValue().toDouble(), 4.);
    QCOMPARE(group.velocityBodyYResponse()->rawValue().toDouble(), 5.);
    QCOMPARE(group.velocityBodyYSetpoint()->rawValue().toDouble(), 6.);
    QCOMPARE(group.velocityIntegralBodyY()->rawValue().toDouble(), 7.);
    QCOMPARE(group.velocityThrottleBodyY()->rawValue().toDouble(), 8.);

    constexpr uint16_t primaryOnlyFlags = static_cast<uint16_t>(ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE |
                                                                ROVER_TUNING_STATUS_FLAGS_RESPONSE_PRIMARY_VALID |
                                                                ROVER_TUNING_STATUS_FLAGS_SETPOINT_PRIMARY_VALID);
    group.handleMessage(vehicle, velocityMessage(301, primaryOnlyFlags));
    QVERIFY(group.velocityAvailable());
    QVERIFY(factIsNaN(group.velocityBodyYResponse()));
    QVERIFY(factIsNaN(group.velocityBodyYSetpoint()));

    group.handleMessage(vehicle, positionMessage(400, 7));
    QVERIFY(group.positionAvailable());
    QVERIFY(group.positionPathAvailable());
    QCOMPARE(group.positionNorth()->rawValue().toDouble(), 1.);
    QCOMPARE(group.positionEast()->rawValue().toDouble(), 2.);
    QCOMPARE(group.targetNorth()->rawValue().toDouble(), 3.);
    QCOMPARE(group.targetEast()->rawValue().toDouble(), 4.);
    QCOMPARE(group.crosstrackError()->rawValue().toDouble(), 5.);
    QCOMPARE(group.lookaheadDistance()->rawValue().toDouble(), 6.);
    QVERIFY(fuzzyCompare(group.targetBearing()->rawValue().toDouble(), qRadiansToDegrees(0.5)));
    QCOMPARE(group.distanceToWaypoint()->rawValue().toDouble(), 8.);
    QCOMPARE(group.positionResetCounter(), 0U);

    group.handleMessage(vehicle, positionMessage(401, 7));
    QCOMPARE(group.positionResetCounter(), 0U);
    group.handleMessage(vehicle, positionMessage(402, 8));
    QCOMPARE(group.positionResetCounter(), 1U);

    constexpr uint16_t noPathFlags =
        static_cast<uint16_t>(ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE | ROVER_TUNING_STATUS_FLAGS_POSITION_VALID |
                              ROVER_TUNING_STATUS_FLAGS_TARGET_VALID);
    group.handleMessage(vehicle, positionMessage(403, 8, noPathFlags));
    QVERIFY(group.positionAvailable());
    QVERIFY(!group.positionPathAvailable());
    QCOMPARE(group.positionResetCounter(), 2U);
    QVERIFY(factIsNaN(group.crosstrackError()));
    QVERIFY(factIsNaN(group.targetBearing()));

    constexpr uint16_t noTargetFlags =
        static_cast<uint16_t>(ROVER_TUNING_STATUS_FLAGS_CONTROLLER_ACTIVE | ROVER_TUNING_STATUS_FLAGS_POSITION_VALID);
    group.handleMessage(vehicle, positionMessage(404, 8, noTargetFlags));
    QVERIFY(group.positionActive());
    QVERIFY(!group.positionAvailable());
    QVERIFY(factIsNaN(group.targetNorth()));
    QVERIFY(factIsNaN(group.targetEast()));
}

void RoverTuningFactGroupTest::_timestampLifecycle_test()
{
    Vehicle* const vehicle = owner(1);
    RoverTuningFactGroup group(vehicle);

    group.handleMessage(vehicle, rateMessage(500));
    group.handleMessage(vehicle, attitudeMessage(500, 0.F, 0.F));
    group.handleMessage(vehicle, velocityMessage(500));
    group.handleMessage(vehicle, positionMessage(500, 1));
    QVERIFY(group.rateAvailable());
    QVERIFY(group.attitudeAvailable());
    QVERIFY(group.velocityAvailable());
    QVERIFY(group.positionAvailable());
    QVERIFY(group.positionPathAvailable());

    group.handleMessage(vehicle, rateMessage(499));
    QVERIFY(group.rateAvailable());
    QVERIFY(group.attitudeAvailable());
    QVERIFY(group.velocityAvailable());
    QVERIFY(group.positionAvailable());
    QVERIFY(group.positionPathAvailable());
    QCOMPARE(group.rateTimestamp()->rawValue().toULongLong(), 500ULL);
    QCOMPARE(group.attitudeTimestamp()->rawValue().toULongLong(), 500ULL);
    QCOMPARE(group.velocityTimestamp()->rawValue().toULongLong(), 500ULL);
    QCOMPARE(group.positionTimestamp()->rawValue().toULongLong(), 500ULL);
    QCOMPARE(group.rateResetCounter(), 0U);

    group.communicationLost();
    QVERIFY(!group.rateAvailable());
    QCOMPARE(group.rateResetCounter(), 1U);
    QCOMPARE(group.attitudeResetCounter(), 1U);
    QCOMPARE(group.velocityResetCounter(), 1U);
    QCOMPARE(group.positionResetCounter(), 1U);

    group.handleMessage(vehicle, rateMessage(499));
    QVERIFY(group.rateAvailable());
    QCOMPARE(group.rateTimestamp()->rawValue().toULongLong(), 499ULL);
}

void RoverTuningFactGroupTest::_timeout_test()
{
    Vehicle* const vehicle = owner(1);
    RoverTuningFactGroup group(vehicle);

    group.handleMessage(vehicle, rateMessage(600));
    QVERIFY(group.rateAvailable());
    QTRY_VERIFY_WITH_TIMEOUT(!group.rateAvailable(), TestTimeout::shortMs());
    QCOMPARE(group.rateResetCounter(), 1U);
    QCOMPARE(group.rateTimestamp()->rawValue().toULongLong(), 0ULL);

    group.handleMessage(vehicle, rateMessage(600));
    QVERIFY(!group.rateAvailable());
    group.handleMessage(vehicle, rateMessage(601));
    QVERIFY(group.rateAvailable());
}

void RoverTuningFactGroupTest::_ownerIsolation_test()
{
    Vehicle* const vehicle1 = owner(1);
    Vehicle* const vehicle2 = owner(2);
    RoverTuningFactGroup group1(vehicle1);
    RoverTuningFactGroup group2(vehicle2);

    group1.handleMessage(vehicle2, rateMessage(700, kRateFlags, ROVER_DRIVE_TYPE_DIFFERENTIAL, 0.25F, 0.5F));
    QVERIFY(!group1.rateAvailable());
    QCOMPARE(group1.rateTimestamp()->rawValue().toULongLong(), 0ULL);

    group2.handleMessage(vehicle2, rateMessage(700, kRateFlags, ROVER_DRIVE_TYPE_DIFFERENTIAL, 0.25F, 0.5F));
    QVERIFY(group2.rateAvailable());
    QVERIFY(!group1.rateAvailable());

    group1.handleMessage(vehicle1, rateMessage(701, kRateFlags, ROVER_DRIVE_TYPE_DIFFERENTIAL, 1.F, 1.5F));
    QVERIFY(group1.rateAvailable());
    QVERIFY(!fuzzyCompare(group1.rateYawResponse()->rawValue().toDouble(),
                          group2.rateYawResponse()->rawValue().toDouble()));
}

UT_REGISTER_TEST(RoverTuningFactGroupTest, TestLabel::Unit, TestLabel::Vehicle)
