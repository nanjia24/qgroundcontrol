#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtTest/QTest>
#include <array>
#include <cmath>

#include "BaseClasses/VehicleTest.h"
#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

class VehiclePIDTuningTelemetryTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _mavlink1RejectsRoverMode();
    void _quadRoverVehicleTypeGate();
    void _mavlink2NegotiationAppliesRequestedRoverMode();
    void _communicationLossRecoveryReappliesRequestedMode();
    void _staleAckDoesNotAdvanceCurrentGeneration();
    void _rejectedAckAbortsCurrentGeneration();
    void _messageIntervalRequestsAreSerializedAcrossCallers();

private:
    struct IntervalRequest
    {
        int messageId = 0;
        int intervalUsec = 0;
    };

    void _recordOutgoingBytes(const QByteArray& bytes);
    bool _setPrimaryLinkMavlink1(bool mavlink1);
    int _matchingRequestCount(int messageId, int intervalUsec) const;
    void _resetRequests();

    QList<IntervalRequest> _requests;
    mavlink_message_t _parserMessage{};
    mavlink_status_t _parserStatus{};
    QMetaObject::Connection _writeConnection;
};

void VehiclePIDTuningTelemetryTest::init()
{
    VehicleTest::init();

    QVERIFY(vehicle());
    QVERIFY(mockLink());
    mockLink()->setMessageIntervalAccepted(true);
    _writeConnection = connect(mockLink(), &MockLink::writeBytesQueuedSignal, this,
                               &VehiclePIDTuningTelemetryTest::_recordOutgoingBytes, Qt::DirectConnection);
    _resetRequests();
}

void VehiclePIDTuningTelemetryTest::cleanup()
{
    if (mockLink()) {
        mockLink()->setMessageIntervalResponseEnabled(true);
    }
    if (vehicle() && vehicle()->vehicleLinkManager()) {
        const SharedLinkInterfacePtr link = vehicle()->vehicleLinkManager()->primaryLink().lock();
        if (link && link->mavlinkChannelIsSet()) {
            mavlink_get_channel_status(static_cast<mavlink_channel_t>(link->mavlinkChannel()))->flags &=
                ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
        }
    }

    disconnect(_writeConnection);
    VehicleTest::cleanup();
}

void VehiclePIDTuningTelemetryTest::_mavlink1RejectsRoverMode()
{
    QVERIFY(_setPrimaryLinkMavlink1(true));
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle()->roverTuningMavlink2Supported(), TestTimeout::shortMs());
    _resetRequests();

    static constexpr std::array<Vehicle::PIDTuningTelemetryMode, 4> roverModes{
        Vehicle::ModeRoverRate,
        Vehicle::ModeRoverAttitude,
        Vehicle::ModeRoverVelocity,
        Vehicle::ModeRoverPosition,
    };
    for (const Vehicle::PIDTuningTelemetryMode mode : roverModes) {
        vehicle()->setPIDTuningTelemetryMode(mode);
        QTest::qWait(100);
    }

    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 0);
    QCOMPARE(_requests.size(), 0);
}

void VehiclePIDTuningTelemetryTest::_quadRoverVehicleTypeGate()
{
    {
        Vehicle quadRoverVehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUAD_ROVER);
        QCOMPARE(static_cast<int>(MAV_TYPE_QUAD_ROVER), 200);
        QVERIFY(quadRoverVehicle.quadRover());
    }

    static constexpr std::array<MAV_TYPE, 5> otherVehicleTypes{
        MAV_TYPE_VTOL_FIXEDROTOR, MAV_TYPE_VTOL_RESERVED5, MAV_TYPE_VTOL_TILTROTOR,
        MAV_TYPE_QUADROTOR,       MAV_TYPE_GROUND_ROVER,
    };
    for (const MAV_TYPE vehicleType : otherVehicleTypes) {
        Vehicle otherVehicle(MAV_AUTOPILOT_PX4, vehicleType);
        QVERIFY(!otherVehicle.quadRover());
    }
}

void VehiclePIDTuningTelemetryTest::_mavlink2NegotiationAppliesRequestedRoverMode()
{
    QVERIFY(_setPrimaryLinkMavlink1(true));
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle()->roverTuningMavlink2Supported(), TestTimeout::shortMs());
    _resetRequests();

    vehicle()->setPIDTuningTelemetryMode(Vehicle::ModeRoverVelocity);
    QTest::qWait(100);
    QCOMPARE(_requests.size(), 0);

    QVERIFY(_setPrimaryLinkMavlink1(false));
    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->roverTuningMavlink2Supported(), TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ROVER_VELOCITY_TUNING_STATUS, 40000), 1,
                              TestTimeout::shortMs());
}

void VehiclePIDTuningTelemetryTest::_communicationLossRecoveryReappliesRequestedMode()
{
    QVERIFY(vehicle()->roverTuningMavlink2Supported());
    vehicle()->setPIDTuningTelemetryMode(Vehicle::ModeRoverPosition);
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 100000), 1,
                              TestTimeout::shortMs());
    _resetRequests();

    simulateCommLoss(true);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->vehicleLinkManager()->communicationLost(), TestTimeout::longMs());
    QTest::qWait(100);
    QCOMPARE(_matchingRequestCount(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 0), 0);

    simulateCommLoss(false);
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle()->vehicleLinkManager()->communicationLost(), TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 0) >= 1,
                             TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ROVER_POSITION_TUNING_STATUS, 100000) >= 1,
                             TestTimeout::shortMs());
}

void VehiclePIDTuningTelemetryTest::_staleAckDoesNotAdvanceCurrentGeneration()
{
    QVERIFY(vehicle()->roverTuningMavlink2Supported());
    mockLink()->setCommLost(true);

    vehicle()->setPIDTuningTelemetryMode(Vehicle::ModeRateAndAttitude);
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 10000), 1, TestTimeout::shortMs());

    QVERIFY(QMetaObject::invokeMethod(vehicle(), "_handleCommunicationLost", Qt::DirectConnection, Q_ARG(bool, true)));
    QVERIFY(QMetaObject::invokeMethod(vehicle(), "_handleCommunicationLost", Qt::DirectConnection, Q_ARG(bool, false)));
    QTest::qWait(100);
    QCOMPARE(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 10000), 1);
    QCOMPARE(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 0), 0);
    QCOMPARE(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_QUATERNION, 10000), 0);

    mockLink()->setCommLost(false);

    // Only after the stale command completes may cleanup and the current generation run.
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 0), 1, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 10000), 2, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_QUATERNION, 10000), 1,
                              TestTimeout::shortMs());
}

void VehiclePIDTuningTelemetryTest::_rejectedAckAbortsCurrentGeneration()
{
    QVERIFY(vehicle()->roverTuningMavlink2Supported());
    mockLink()->setCommLost(true);

    vehicle()->setPIDTuningTelemetryMode(Vehicle::ModeRateAndAttitude);
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 10000), 1, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(mockLink()->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());

    mockLink()->setMessageIntervalResponseEnabled(false);
    mockLink()->setCommLost(false);
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_UNSUPPORTED);

    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_TARGET, 0), 1, TestTimeout::shortMs());
    const QString requestError = Vehicle::tr("Telemetry stream request was rejected");
    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->roverTuningTelemetryError(), requestError, TestTimeout::shortMs());
    QCOMPARE(_matchingRequestCount(MAVLINK_MSG_ID_ATTITUDE_QUATERNION, 10000), 0);

    QTRY_COMPARE_WITH_TIMEOUT(mockLink()->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2,
                              TestTimeout::shortMs());
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_UNSUPPORTED);
    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->roverTuningTelemetryError(), requestError, TestTimeout::shortMs());
    const int intervalCommandCount = mockLink()->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL);
    QTest::qWait(250);
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), intervalCommandCount);
}

void VehiclePIDTuningTelemetryTest::_messageIntervalRequestsAreSerializedAcrossCallers()
{
    QVERIFY(vehicle()->roverTuningMavlink2Supported());
    mockLink()->setMessageIntervalResponseEnabled(false);

    vehicle()->setPIDTuningTelemetryMode(Vehicle::ModeRoverRate);
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_ROVER_RATE_TUNING_STATUS, 20000), 1,
                              TestTimeout::shortMs());
    const int requestCountWhilePIDCommandPending = _requests.size();

    vehicle()->setMessageRate(vehicle()->defaultComponentId(), MAVLINK_MSG_ID_HIGHRES_IMU, 20);
    QTest::qWait(100);
    QCOMPARE(_requests.size(), requestCountWhilePIDCommandPending);

    mockLink()->setMessageIntervalResponseEnabled(true);
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTRY_COMPARE_WITH_TIMEOUT(_matchingRequestCount(MAVLINK_MSG_ID_HIGHRES_IMU, 50000), 1, TestTimeout::shortMs());
    QVERIFY(vehicle()->roverTuningTelemetryError().isEmpty());
}

void VehiclePIDTuningTelemetryTest::_recordOutgoingBytes(const QByteArray& bytes)
{
    mavlink_message_t decodedMessage{};
    mavlink_status_t decodedStatus{};

    for (const char byte : bytes) {
        if (mavlink_frame_char_buffer(&_parserMessage, &_parserStatus, static_cast<uint8_t>(byte), &decodedMessage,
                                      &decodedStatus) != MAVLINK_FRAMING_OK) {
            continue;
        }

        if (decodedMessage.msgid != MAVLINK_MSG_ID_COMMAND_LONG) {
            continue;
        }

        mavlink_command_long_t command{};
        mavlink_msg_command_long_decode(&decodedMessage, &command);
        if (command.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
            _requests.append({
                static_cast<int>(std::lround(command.param1)),
                static_cast<int>(std::lround(command.param2)),
            });
        }
    }
}

bool VehiclePIDTuningTelemetryTest::_setPrimaryLinkMavlink1(bool mavlink1)
{
    if (!vehicle() || !vehicle()->vehicleLinkManager()) {
        return false;
    }

    const SharedLinkInterfacePtr link = vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!link || !link->mavlinkChannelIsSet()) {
        return false;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(static_cast<mavlink_channel_t>(link->mavlinkChannel()));
    if (mavlink1) {
        status->flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
    } else {
        status->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
    }

    MAVLinkProtocol::instance()->mavlinkMessageStatus(vehicle()->id(), 0, 0, 0, 0.f);
    return true;
}

int VehiclePIDTuningTelemetryTest::_matchingRequestCount(int messageId, int intervalUsec) const
{
    int count = 0;
    for (const IntervalRequest& request : _requests) {
        if (request.messageId == messageId && request.intervalUsec == intervalUsec) {
            ++count;
        }
    }
    return count;
}

void VehiclePIDTuningTelemetryTest::_resetRequests()
{
    QCoreApplication::processEvents();
    _requests.clear();
    mockLink()->clearReceivedMavCommandCounts();
}

UT_REGISTER_TEST(VehiclePIDTuningTelemetryTest, TestLabel::Integration, TestLabel::Vehicle)

#include "VehiclePIDTuningTelemetryTest.moc"
