#include "QmlObjectListModel.h"
#include "VehicleLinkManagerTest.h"

#include <QtTest/QSignalSpy>

#include "LinkManager.h"
#include "MavCommandQueue.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

void VehicleLinkManagerTest::_simpleLinkTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    QVERIFY(mockConfig);
    QVERIFY(mockLink);
    const QSignalSpy spyConfigDelete(mockConfig.get(), &QObject::destroyed);
    const QSignalSpy spyLinkDelete(mockLink.get(), &QObject::destroyed);
    QVERIFY(spyConfigDelete.isValid());
    QVERIFY(spyLinkDelete.isValid());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::shortMs());
    QSignalSpy spyVehicleDelete(vehicle, &QObject::destroyed);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QCOMPARE(mockConfig.use_count(), 2);  // Refs: This method, MockLink
    QCOMPARE(mockLink.use_count(), 3);    // Refs: This method, LinkManager, Vehicle
    // We wait for the full initial connect sequence to complete to catch anby ComponentInformationManager bugs
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::longMs());
    // Drain queued command traffic before disconnect to avoid racing pending writes with link teardown.
    UnitTest::settleEventLoopForCleanup(2, 10);
    mockLink->disconnect();
    // Vehicle should go away due to disconnect
    QVERIFY_SIGNAL_WAIT(spyVehicleDelete, TestTimeout::shortMs());
    // Config/Link should still be alive due to the last refs being held by this method
    QCOMPARE(spyConfigDelete.count(), 0);
    QCOMPARE(spyLinkDelete.count(), 0);
    QCOMPARE(mockConfig.use_count(), 2);  // Refs: This method, MockLink
    QCOMPARE(mockLink.use_count(), 1);    // Refs: This method
    // Let go of our refs from this method and config and link should go away
    mockConfig.reset();
    mockLink.reset();
    QCOMPARE(mockConfig.use_count(), 0);
    QCOMPARE(mockLink.use_count(), 0);
    QCOMPARE(spyLinkDelete.count(), 1);
    QCOMPARE(spyConfigDelete.count(), 1);
}

void VehicleLinkManagerTest::_simpleCommLossTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink* const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::longMs());
    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    pMockLink->setCommLost(true);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);
    spyCommLostChanged.clear();
    pMockLink->setCommLost(false);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), false);
    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    pMockLink->setCommLost(true);
    QVERIFY_NO_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
}

void VehicleLinkManagerTest::_multiLinkSingleVehicleTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::longMs());
    // The first link to start sending a heartbeat will be the primary link.
    // Depending on how the thread scheduling works, that could be the mockLink2.
    const SharedLinkInterfacePtr primaryLink = vehicleLinkManager->primaryLink().lock();
    QVERIFY(primaryLink == mockLink1 || primaryLink == mockLink2);
    const int primaryStatusIndex = primaryLink == mockLink1 ? 0 : 1;
    const int secondaryStatusIndex = 1 - primaryStatusIndex;
    MockLink* pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    MockLink* pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    if (primaryLink == mockLink2) {
        std::swap(pMockLink1, pMockLink2);
    }
    const QStringList rgNames = vehicleLinkManager->linkNames();
    QStringList rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgNames.count(), 2);
    QCOMPARE(rgNames[0], mockConfig1->name());
    QCOMPARE(rgNames[1], mockConfig2->name());
    QCOMPARE(rgStatus.count(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(
        vehicleLinkManager->linkStatuses()[0].isEmpty() && vehicleLinkManager->linkStatuses()[1].isEmpty(),
        VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    rgStatus = vehicleLinkManager->linkStatuses();
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    MultiSignalSpy multiSpy;
    QVERIFY(multiSpy.init(vehicleLinkManager));
    // Comm lost on 2: 1 is primary, 2 is secondary so comm loss/regain on 2 should only update status text
    pMockLink2->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmittedOnce(_linkStatusesChangedSignalName));
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[primaryStatusIndex].isEmpty());
    QVERIFY(!rgStatus[secondaryStatusIndex].isEmpty());
    multiSpy.clearAllSignals();
    pMockLink2->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmittedOnce(_linkStatusesChangedSignalName));
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    multiSpy.clearAllSignals();
    // Comm loss on 1: 1 is primary so should trigger switch of primary to 2
    pMockLink1->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(
        multiSpy.onlyEmittedOnceByMask(multiSpy.mask(_primaryLinkChangedSignalName, _linkStatusesChangedSignalName)));
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(!rgStatus[primaryStatusIndex].isEmpty());
    QVERIFY(rgStatus[secondaryStatusIndex].isEmpty());
    multiSpy.clearAllSignals();
    // Comm regained on 1 should leave 2 as primary and only update status
    pMockLink1->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmittedOnce(_linkStatusesChangedSignalName));
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    multiSpy.clearAllSignals();
}

void VehicleLinkManagerTest::_manualPrimaryLinkTuningStreamTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());

    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);
    const SharedLinkInterfacePtr primaryLink = vehicleLinkManager->primaryLink().lock();
    QVERIFY(primaryLink == mockLink1 || primaryLink == mockLink2);
    const SharedLinkInterfacePtr secondaryLink = primaryLink == mockLink1 ? mockLink2 : mockLink1;
    auto* const primaryMockLink = qobject_cast<MockLink*>(primaryLink.get());
    auto* const secondaryMockLink = qobject_cast<MockLink*>(secondaryLink.get());
    QVERIFY(primaryMockLink);
    QVERIFY(secondaryMockLink);
    primaryMockLink->setMessageIntervalAccepted(true);
    secondaryMockLink->setMessageIntervalAccepted(true);

    primaryMockLink->clearReceivedMavCommandCounts();
    secondaryMockLink->clearReceivedMavCommandCounts();
    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeRoverRate);
    QTRY_VERIFY_WITH_TIMEOUT(primaryMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL) >= 1,
                             TestTimeout::shortMs());

    primaryMockLink->clearReceivedMavCommandCounts();
    secondaryMockLink->clearReceivedMavCommandCounts();
    vehicleLinkManager->setPrimaryLinkByName(secondaryLink->linkConfiguration()->name());

    QTRY_VERIFY_WITH_TIMEOUT(primaryMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL) >= 1,
                             TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(secondaryMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL) >= 1,
                             TestTimeout::shortMs());
    QVERIFY(vehicleLinkManager->primaryLink().lock() == secondaryLink);

    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeDisabled);
}

void VehicleLinkManagerTest::_pendingTuningAckRestoresOriginalPrimaryLinkTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());

    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    const SharedLinkInterfacePtr originalPrimary = vehicleLinkManager->primaryLink().lock();
    const SharedLinkInterfacePtr newPrimary = originalPrimary == mockLink1 ? mockLink2 : mockLink1;
    auto* const originalMockLink = qobject_cast<MockLink*>(originalPrimary.get());
    auto* const newMockLink = qobject_cast<MockLink*>(newPrimary.get());
    QVERIFY(originalMockLink);
    QVERIFY(newMockLink);
    originalMockLink->setMessageIntervalAccepted(true);
    originalMockLink->setMessageIntervalResponseEnabled(false);
    newMockLink->setMessageIntervalAccepted(true);
    originalMockLink->clearReceivedMavCommandCounts();
    newMockLink->clearReceivedMavCommandCounts();

    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeRoverRate);
    QTRY_COMPARE_WITH_TIMEOUT(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());
    vehicleLinkManager->setPrimaryLinkByName(newPrimary->linkConfiguration()->name());
    newMockLink->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTest::qWait(100);
    QCOMPARE(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);
    QCOMPARE(newMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 0);

    originalMockLink->setMessageIntervalResponseEnabled(true);
    originalMockLink->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTRY_COMPARE_WITH_TIMEOUT(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2,
                              TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(newMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());

    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeDisabled);
}

void VehicleLinkManagerTest::_failedOldLinkRestoreDoesNotBlockNewPrimaryTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());

    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    const SharedLinkInterfacePtr originalPrimary = vehicleLinkManager->primaryLink().lock();
    const SharedLinkInterfacePtr newPrimary = originalPrimary == mockLink1 ? mockLink2 : mockLink1;
    auto* const originalMockLink = qobject_cast<MockLink*>(originalPrimary.get());
    auto* const newMockLink = qobject_cast<MockLink*>(newPrimary.get());
    QVERIFY(originalMockLink);
    QVERIFY(newMockLink);
    originalMockLink->setMessageIntervalAccepted(true);
    newMockLink->setMessageIntervalAccepted(true);

    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeRoverRate);
    QTRY_VERIFY_WITH_TIMEOUT(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL) >= 1,
                             TestTimeout::shortMs());
    QTest::qWait(100);
    originalMockLink->clearReceivedMavCommandCounts();
    newMockLink->clearReceivedMavCommandCounts();
    originalMockLink->setMessageIntervalResponseEnabled(false);

    vehicleLinkManager->setPrimaryLinkByName(newPrimary->linkConfiguration()->name());
    QTRY_COMPARE_WITH_TIMEOUT(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(newMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle->roverTuningTelemetryError().isEmpty(), MavCommandQueue::kTestMaxWaitMs);

    originalMockLink->setMessageIntervalResponseEnabled(true);
    originalMockLink->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTest::qWait(100);
    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeDisabled);
    QTRY_COMPARE_WITH_TIMEOUT(newMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2,
                              TestTimeout::shortMs());

    originalMockLink->clearReceivedMavCommandCounts();
    vehicleLinkManager->setPrimaryLinkByName(originalPrimary->linkConfiguration()->name());
    QTRY_COMPARE_WITH_TIMEOUT(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->roverTuningTelemetryError().isEmpty(), TestTimeout::shortMs());
}

void VehicleLinkManagerTest::_setMessageIntervalQueueIsolationTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());
    auto* const commandQueue = vehicle->findChild<MavCommandQueue*>();
    auto* const firstMockLink = qobject_cast<MockLink*>(mockLink1.get());
    auto* const secondMockLink = qobject_cast<MockLink*>(mockLink2.get());
    QVERIFY(commandQueue);
    QVERIFY(firstMockLink);
    QVERIFY(secondMockLink);

    const int componentId = vehicle->defaultComponentId();
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::longMs());
    firstMockLink->setMessageIntervalAccepted(true);
    firstMockLink->setMessageIntervalResponseEnabled(false);
    secondMockLink->setMessageIntervalAccepted(true);
    secondMockLink->setMessageIntervalResponseEnabled(false);
    firstMockLink->clearReceivedMavCommandCounts();
    secondMockLink->clearReceivedMavCommandCounts();

    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink1, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_ATTITUDE, 20000);
    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink2, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_LOCAL_POSITION_NED, 20000);
    QTRY_COMPARE_WITH_TIMEOUT(firstMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(secondMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());

    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink1, componentId,
                                               MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED);
    QTRY_COMPARE_WITH_TIMEOUT(firstMockLink->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED),
                              1, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED),
                             TestTimeout::shortMs());

    firstMockLink->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    secondMockLink->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::shortMs());
}

void VehicleLinkManagerTest::_setMessageIntervalLateAckQuarantineTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig, mockLink);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());
    auto* const commandQueue = vehicle->findChild<MavCommandQueue*>();
    auto* const mockLinkRaw = qobject_cast<MockLink*>(mockLink.get());
    QVERIFY(commandQueue);
    QVERIFY(mockLinkRaw);

    const int componentId = vehicle->defaultComponentId();
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::longMs());
    mockLinkRaw->setMessageIntervalAccepted(true);
    mockLinkRaw->setMessageIntervalResponseEnabled(false);
    mockLinkRaw->clearReceivedMavCommandCounts();
    QSignalSpy resultSpy(commandQueue, &MavCommandQueue::commandResult);
    QVERIFY(resultSpy.isValid());
    const auto intervalResultCount = [&resultSpy]() {
        int count = 0;
        for (const QList<QVariant>& arguments : resultSpy) {
            if (arguments[2].toInt() == static_cast<int>(MAV_CMD_SET_MESSAGE_INTERVAL)) {
                ++count;
            }
        }
        return count;
    };

    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_ATTITUDE, 20000);
    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_LOCAL_POSITION_NED, 20000);
    QTRY_COMPARE_WITH_TIMEOUT(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(intervalResultCount(), 2, MavCommandQueue::kTestMaxWaitMs);
    QCOMPARE(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);
    for (const QList<QVariant>& arguments : resultSpy) {
        if (arguments[2].toInt() == static_cast<int>(MAV_CMD_SET_MESSAGE_INTERVAL)) {
            QCOMPARE(arguments[4].toInt(), static_cast<int>(Vehicle::MavCmdResultFailureNoResponseToCommand));
        }
    }

    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_VFR_HUD, 20000);
    QCOMPARE(intervalResultCount(), 3);
    QCOMPARE(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);

    mockLinkRaw->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTest::qWait(100);
    QCOMPARE(intervalResultCount(), 3);
    QCOMPARE(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);

    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_VFR_HUD, 20000);
    QTRY_COMPARE_WITH_TIMEOUT(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2,
                              TestTimeout::shortMs());
    mockLinkRaw->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTRY_COMPARE_WITH_TIMEOUT(intervalResultCount(), 4, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::shortMs());

    // A truly lost ACK must not quarantine all future interval requests for the
    // lifetime of the connection. Reuse stays fail-closed for one ACK window,
    // then becomes available again without unplugging the link.
    mockLinkRaw->setMessageIntervalResponseEnabled(false);
    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_ATTITUDE, 20000);
    QTRY_COMPARE_WITH_TIMEOUT(intervalResultCount(), 5, MavCommandQueue::kTestMaxWaitMs);
    const int sendsAfterSecondTimeout = mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL);

    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_VFR_HUD, 20000);
    QCOMPARE(intervalResultCount(), 6);
    QCOMPARE(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), sendsAfterSecondTimeout);

    QTest::qWait(MavCommandQueue::kTestAckTimeoutMs + 100);
    commandQueue->sendCommandWithHandlerOnLink(nullptr, mockLink, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_VFR_HUD, 20000);
    QTRY_COMPARE_WITH_TIMEOUT(mockLinkRaw->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL),
                              sendsAfterSecondTimeout + 1, TestTimeout::shortMs());
    mockLinkRaw->sendUnexpectedCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
    QTRY_COMPARE_WITH_TIMEOUT(intervalResultCount(), 7, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::shortMs());
}

void VehicleLinkManagerTest::_pendingSetMessageIntervalDisconnectFailsOverTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());
    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    auto* const commandQueue = vehicle->findChild<MavCommandQueue*>();
    const SharedLinkInterfacePtr originalPrimary = vehicleLinkManager->primaryLink().lock();
    const SharedLinkInterfacePtr backupLink = originalPrimary == mockLink1 ? mockLink2 : mockLink1;
    auto* const originalMockLink = qobject_cast<MockLink*>(originalPrimary.get());
    auto* const backupMockLink = qobject_cast<MockLink*>(backupLink.get());
    QVERIFY(commandQueue);
    QVERIFY(originalMockLink);
    QVERIFY(backupMockLink);

    const int componentId = vehicle->defaultComponentId();
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::longMs());
    originalMockLink->setMessageIntervalAccepted(true);
    originalMockLink->setMessageIntervalResponseEnabled(false);
    backupMockLink->setMessageIntervalAccepted(true);
    originalMockLink->clearReceivedMavCommandCounts();
    backupMockLink->clearReceivedMavCommandCounts();

    QSignalSpy resultSpy(commandQueue, &MavCommandQueue::commandResult);
    QVERIFY(resultSpy.isValid());
    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeRoverRate);
    QTRY_COMPARE_WITH_TIMEOUT(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1,
                              TestTimeout::shortMs());
    commandQueue->sendCommandWithHandlerOnLink(nullptr, originalPrimary, componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
                                               MAVLINK_MSG_ID_HIGHRES_IMU, 50000);
    QCOMPARE(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);

    originalMockLink->disconnect();

    int intervalFailureCount = 0;
    for (const QList<QVariant>& arguments : resultSpy) {
        if (arguments[2].toInt() == static_cast<int>(MAV_CMD_SET_MESSAGE_INTERVAL)) {
            ++intervalFailureCount;
            QCOMPARE(arguments[4].toInt(), static_cast<int>(Vehicle::MavCmdResultFailureNoResponseToCommand));
        }
    }
    QCOMPARE(intervalFailureCount, 1);
    QTRY_VERIFY_WITH_TIMEOUT(vehicleLinkManager->primaryLink().lock() == backupLink, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(backupMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL) >= 1,
                             TestTimeout::shortMs());
    QCOMPARE(originalMockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);

    vehicle->setPIDTuningTelemetryMode(Vehicle::ModeDisabled);
    QTRY_VERIFY_WITH_TIMEOUT(!commandQueue->isPending(componentId, MAV_CMD_SET_MESSAGE_INTERVAL),
                             TestTimeout::shortMs());
}

void VehicleLinkManagerTest::_connectionRemovedTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink* const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::mediumMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::longMs());
    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    // Connection removed should just signal communication lost
    pMockLink->simulateConnectionRemoved();
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);
}

void VehicleLinkManagerTest::_highLatencyLinkTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, true /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    MockLink* const pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::mediumMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);
    MultiSignalSpy multiSpyVLM;
    QVERIFY(multiSpyVLM.init(vehicleLinkManager));
    // Addition of second non high latency link should:
    //  Change primary link from 1 to 2
    //  Stop high latency transmission on 1
    QSignalSpy spyTransmissionEnabledChanged(pMockLink1, &MockLink::highLatencyTransmissionEnabledChanged);
    QVERIFY(spyTransmissionEnabledChanged.isValid());
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);
    MockLink* pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    QCOMPARE(multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, TestTimeout::shortMs()), true);
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    // Wait for the MAV_CMD_CONTROL_HIGH_LATENCY command to be processed
    if (spyTransmissionEnabledChanged.count() == 0) {
        QVERIFY_SIGNAL_WAIT(spyTransmissionEnabledChanged, TestTimeout::shortMs());
    }
    QCOMPARE(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), false);
    multiSpyVLM.clearAllSignals();
    spyTransmissionEnabledChanged.clear();
    // Comm lost on primary:2 should:
    //  Switch primary to 1
    //  Re-enable high latency transmission on 1
    pMockLink2->setCommLost(true);
    QCOMPARE(
        multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
        true);
    QCOMPARE(pMockLink1, vehicleLinkManager->primaryLink().lock().get());
    // Wait for the MAV_CMD_CONTROL_HIGH_LATENCY command to be processed
    if (spyTransmissionEnabledChanged.count() == 0) {
        QVERIFY_SIGNAL_WAIT(spyTransmissionEnabledChanged, TestTimeout::shortMs());
    }
    QCOMPARE(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), true);
    spyTransmissionEnabledChanged.clear();
}

void VehicleLinkManagerTest::_startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId,
                                            SharedLinkConfigurationPtr& mockConfig, SharedLinkInterfacePtr& mockLink)
{
    MockConfiguration* const pMockConfig = new MockConfiguration(QStringLiteral("Mock %1").arg(mockIndex));
    mockConfig = SharedLinkConfigurationPtr(pMockConfig);
    pMockConfig->setDynamic(true);
    pMockConfig->setHighLatency(highLatency);
    pMockConfig->setIncrementVehicleId(incrementVehicleId);
    QVERIFY(linkManager()->createConnectedLink(mockConfig));
    QVERIFY(mockConfig->link());
    mockLink = linkManager()->sharedLinkInterfacePointerForLink(mockConfig->link());
    QVERIFY(mockLink);
}

UT_REGISTER_TEST(VehicleLinkManagerTest, TestLabel::Integration, TestLabel::Vehicle)
