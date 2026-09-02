#include "HybridOperatorQmlTest.h"

#include <QtCore/QCoreApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlError>

#include "ColoredSvgImageProvider.h"
#include "QGCCorePlugin.h"

void HybridOperatorQmlTest::_componentSafety_test()
{
    QQmlApplicationEngine* const engine = QGCCorePlugin::instance()->createQmlApplicationEngine(nullptr);
    engine->addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    QStringList qmlWarnings;
    connect(engine, &QQmlEngine::warnings, this, [&qmlWarnings](const QList<QQmlError>& warnings) {
        for (const QQmlError& warning : warnings) {
            qmlWarnings.append(warning.toString());
        }
    });

    engine->load(QUrl(QStringLiteral("qrc:/qml/HybridOperatorSafetyTest.qml")));
    QCoreApplication::processEvents();
    QVERIFY2(!engine->rootObjects().isEmpty(), qPrintable(qmlWarnings.join('\n')));

    QObject* const root = engine->rootObjects().constFirst();
    QVERIFY_TRUE_WAIT(root->property("complete").toBool(), TestTimeout::mediumMs());
    QVERIFY2(root->property("failure").toString().isEmpty(), qPrintable(root->property("failure").toString()));

    delete engine;
}

UT_REGISTER_TEST(HybridOperatorQmlTest, TestLabel::Unit)
