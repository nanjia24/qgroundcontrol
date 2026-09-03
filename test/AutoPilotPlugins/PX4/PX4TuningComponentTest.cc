#include "PX4TuningComponentTest.h"

#include <QtCore/QFile>

#include "PX4TuningComponent.h"

void PX4TuningComponentTest::_vehicleTypeRouting()
{
    QCOMPARE(PX4TuningComponent::setupSourceForVehicleType(MAV_TYPE_QUAD_ROVER),
             QUrl(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentQuadRover.qml")));

    QCOMPARE(PX4TuningComponent::setupSourceForVehicleType(MAV_TYPE_VTOL_FIXEDROTOR),
             QUrl(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentVTOL.qml")));

    QCOMPARE(PX4TuningComponent::setupSourceForVehicleType(MAV_TYPE_QUADROTOR),
             QUrl(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentCopter.qml")));

    QCOMPARE(PX4TuningComponent::setupSourceForVehicleType(MAV_TYPE_FIXED_WING),
             QUrl(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentPlane.qml")));
}

void PX4TuningComponentTest::_quadRoverTabsDoNotDependOnLegacyParameter()
{
    QFile source(QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentQuadRover.qml"));
    QVERIFY(source.open(QIODevice::ReadOnly));
    const QByteArray qml = source.readAll();
    QVERIFY(qml.contains("SetupPage {"));
    QVERIFY(qml.contains("pageComponent: pageComponent"));
    QVERIFY(qml.contains("buttonText: qsTr(\"Multirotor\")"));
    QVERIFY(qml.contains("buttonText: qsTr(\"Rover\")"));
    QVERIFY(!qml.contains("HYBR_QUAD_ROV"));
    QVERIFY(!qml.contains("MAV_TYPE_VTOL_FIXEDROTOR"));

    QFile roverSource(QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentRoverAll.qml"));
    QVERIFY(roverSource.open(QIODevice::ReadOnly));
    const QByteArray roverQml = roverSource.readAll();
    QVERIFY(roverQml.contains("buttonText: qsTr(\"Rate\")"));
    QVERIFY(roverQml.contains("buttonText: qsTr(\"Attitude\")"));
    QVERIFY(roverQml.contains("buttonText: qsTr(\"Velocity\")"));
    QVERIFY(roverQml.contains("buttonText: qsTr(\"Path Tracking\")"));
    QVERIFY(!roverQml.contains("HYBR_QUAD_ROV"));
}

UT_REGISTER_TEST(PX4TuningComponentTest, TestLabel::Unit)
