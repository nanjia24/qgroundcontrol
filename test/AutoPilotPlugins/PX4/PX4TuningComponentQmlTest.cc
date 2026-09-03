#include "PX4TuningComponentQmlTest.h"

#include <QtCore/QCoreApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlError>
#include <QtQuick/QQuickItem>

#include "ColoredSvgImageProvider.h"
#include "QGCCorePlugin.h"

PX4TuningComponentQmlTest::PX4TuningComponentQmlTest(QObject* parent) : VehicleTest(parent)
{
    setWaitForParameters(true);
}

void PX4TuningComponentQmlTest::_quadRoverRatePageHasUsablePlotLayout()
{
    QQmlApplicationEngine* const engine = QGCCorePlugin::instance()->createQmlApplicationEngine(nullptr);
    engine->addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());

    QStringList qmlWarnings;
    connect(engine, &QQmlEngine::warnings, this, [&qmlWarnings](const QList<QQmlError>& warnings) {
        for (const QQmlError& warning : warnings) {
            qmlWarnings.append(warning.toString());
        }
    });

    static constexpr char qml[] = R"(
import QtQuick
import QtQuick.Window

import QGroundControl
import QGroundControl.Controls

Window {
    width: 1280
    height: 800
    visible: true

    QtObject {
        id: globals
        readonly property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

    Loader {
        anchors.fill: parent
        property var vehicleComponent: null
        source: "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentQuadRover.qml"
    }
}
)";
    engine->loadData(QByteArray::fromRawData(qml, sizeof(qml) - 1),
                     QUrl(QStringLiteral("qrc:/qml/PX4TuningComponentQmlTest.qml")));
    QCoreApplication::processEvents();
    QVERIFY2(!engine->rootObjects().isEmpty(), qPrintable(qmlWarnings.join('\n')));

    QObject* const root = engine->rootObjects().constFirst();
    QQuickItem* pidTuning = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(
        (pidTuning = root->findChild<QQuickItem*>(QStringLiteral("copterRatePIDTuning"))) != nullptr,
        TestTimeout::mediumMs());
    QVERIFY2(pidTuning->width() > 0.0, qPrintable(qmlWarnings.join('\n')));
    QVERIFY2(pidTuning->height() > 0.0, qPrintable(qmlWarnings.join('\n')));

    QQuickItem* const chart = pidTuning->findChild<QQuickItem*>(QStringLiteral("pidTuningChart"));
    QVERIFY2(chart, qPrintable(qmlWarnings.join('\n')));
    QVERIFY(chart->width() > 0.0);
    QVERIFY(chart->height() > 0.0);
    QVERIFY(chart->isVisible());

    QQuickItem* const toggleButton = pidTuning->findChild<QQuickItem*>(QStringLiteral("pidTuningPlotToggleButton"));
    QVERIFY2(toggleButton, qPrintable(qmlWarnings.join('\n')));
    QVERIFY(toggleButton->width() > 0.0);
    QVERIFY(toggleButton->height() > 0.0);
    QVERIFY(toggleButton->isVisible());

    delete engine;
}

UT_REGISTER_TEST(PX4TuningComponentQmlTest, TestLabel::Integration, TestLabel::Vehicle)
