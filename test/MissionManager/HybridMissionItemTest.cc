#include "HybridMissionItemTest.h"

#include <QtTest/QSignalSpy>

#include "MissionItem.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void HybridMissionItemTest::init()
{
    VehicleTestManualConnect::init();
    _connectQuadRoverMockLink();
    _missionManager = vehicle()->missionManager();
    QVERIFY(_missionManager);
    if (_missionManager->inProgress()) {
        QSignalSpy inProgressSpy(_missionManager, &PlanManager::inProgressChanged);
        QVERIFY(UnitTest::waitForSignal(inProgressSpy, TestTimeout::longMs(),
                                        QStringLiteral("PlanManager::inProgressChanged")));
    }
    QVERIFY(!_missionManager->inProgress());
}

void HybridMissionItemTest::_connectQuadRoverMockLink()
{
    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());
    _mockLink = MockLink::startPX4QuadRoverMockLink(false, false, false);
    QVERIFY(_mockLink);
    QVERIFY(UnitTest::waitForSignal(activeVehicleSpy, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")));
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->quadRover());
}

void HybridMissionItemTest::_planRoundTripsBothHybridTargets()
{
    for (const double target : {1.0, 2.0}) {
        MissionItem original(1, MAV_CMD_DO_HYBRID_TRANSITION, MAV_FRAME_MISSION, target, 0, 0, 0, 0, 0, 0, true, false);
        QJsonObject json;
        original.save(json);

        MissionItem restored;
        QString error;
        QVERIFY2(restored.load(json, 1, error), qPrintable(error));
        QCOMPARE(restored.command(), MAV_CMD_DO_HYBRID_TRANSITION);
        QCOMPARE(restored.param1(), target);
        QCOMPARE(restored.param2(), 0.0);
        QCOMPARE(restored.param7(), 0.0);
    }
}

bool HybridMissionItemTest::_missionItemIsRejected(double param1, double param2, double param3, double param4,
                                                   double param5, double param6, double param7)
{
    QList<MissionItem*> items;
    items.append(
        new MissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false));
    items.append(new MissionItem(1, MAV_CMD_DO_HYBRID_TRANSITION, MAV_FRAME_MISSION, param1, param2, param3, param4,
                                 param5, param6, param7, true, false));
    QSignalSpy errorSpy(_missionManager, &PlanManager::error);
    mockLink()->clearReceivedMavlinkMessageCounts();
    expectLogMessage(QtDebugMsg,
                     QRegularExpression(QStringLiteral("QGCApplication::showAppMessage.*Hybrid transition")));
    _missionManager->writeMissionItems(items);
    if (_missionManager->inProgress()) {
        QSignalSpy sendCompleteSpy(_missionManager, &PlanManager::sendComplete);
        (void)UnitTest::waitForSignal(sendCompleteSpy, TestTimeout::longMs(),
                                      QStringLiteral("PlanManager::sendComplete"));
        return false;
    }
    const bool rejected =
        errorSpy.count() == 1 && mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MISSION_COUNT) == 0;
    qDeleteAll(items);
    return rejected;
}

void HybridMissionItemTest::_invalidHybridTransitionsAreRejectedBeforeMissionCount()
{
    QVERIFY(_missionItemIsRejected(1.5, 0, 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(qQNaN(), 0, 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(qInf(), 0, 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(0, 0, 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(3, 0, 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(1, 1, 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(1, qQNaN(), 0, 0, 0, 0, 0));
    QVERIFY(_missionItemIsRejected(1, 0, 0, 0, 0, 0, qInf()));
}

bool HybridMissionItemTest::_missionItemIsAccepted(double param1, double param2, double param3, double param4,
                                                   double param5, double param6, double param7)
{
    QList<MissionItem*> items;
    items.append(
        new MissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false));
    items.append(new MissionItem(1, MAV_CMD_DO_HYBRID_TRANSITION, MAV_FRAME_MISSION, param1, param2, param3, param4,
                                 param5, param6, param7, true, false));
    QSignalSpy sendCompleteSpy(_missionManager, &PlanManager::sendComplete);
    mockLink()->clearReceivedMavlinkMessageCounts();
    _missionManager->writeMissionItems(items);
    if (!UnitTest::waitForSignal(sendCompleteSpy, TestTimeout::longMs(), QStringLiteral("PlanManager::sendComplete"))) {
        return false;
    }
    return mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MISSION_COUNT) == 1 &&
           _missionManager->missionItems().count() == 1 &&
           _missionManager->missionItems().constFirst()->command() == MAV_CMD_DO_HYBRID_TRANSITION &&
           _missionManager->missionItems().constFirst()->param1() == param1;
}

void HybridMissionItemTest::_validHybridTransitionUploadsAndDownloadsUnchanged()
{
    QVERIFY(_missionItemIsAccepted(2, 0, 0, 0, 0, 0, 0));
    QSignalSpy inProgressSpy(_missionManager, &PlanManager::inProgressChanged);
    _missionManager->loadFromVehicle();
    QVERIFY(UnitTest::waitForSignal(inProgressSpy, TestTimeout::longMs(),
                                    QStringLiteral("PlanManager::inProgressChanged")));
    QCOMPARE(_missionManager->missionItems().count(), 1);
    QCOMPARE(_missionManager->missionItems().constFirst()->command(), MAV_CMD_DO_HYBRID_TRANSITION);
    QCOMPARE(_missionManager->missionItems().constFirst()->param1(), 2.0);
}

#include "UnitTest.h"

UT_REGISTER_TEST(HybridMissionItemTest, TestLabel::Integration, TestLabel::MissionManager)
