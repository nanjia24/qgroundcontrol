#include "PX4AirframeLoaderTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryFile>

#include "AirframeComponentAirframes.h"
#include "PX4AirframeLoader.h"

void PX4AirframeLoaderTest::_qgcOverlayReplacesOnlyDeclaredAirframe()
{
    AirframeComponentAirframes::clear();
    const auto cleanup = qScopeGuard([] { AirframeComponentAirframes::clear(); });

    QTemporaryFile cachedAirframes;
    QVERIFY(cachedAirframes.open());

    const QByteArray cachedXml = R"(
<airframes>
  <version>1</version>
  <airframe_group name="Quad-Rover" image="AirframeUnknown">
    <airframe name="Custom" id="22001"/>
    <airframe name="Legacy Airframe" id="12345"/>
  </airframe_group>
</airframes>
)";
    QCOMPARE(cachedAirframes.write(cachedXml), cachedXml.size());
    cachedAirframes.close();

    QVERIFY(PX4AirframeLoader::_loadAirframeMetaDataFile(cachedAirframes.fileName(), false));
    QVERIFY(PX4AirframeLoader::_loadAirframeMetaDataFile(
        QStringLiteral(":/AutoPilotPlugins/PX4/QGCAirframeFactMetaData.xml"), true));

    const auto& airframeTypes = AirframeComponentAirframes::get();
    QCOMPARE(airframeTypes.size(), 1);

    const AirframeComponentAirframes::AirframeType_t* quadRoverGroup =
        airframeTypes.value(QStringLiteral("Quad-Rover"));
    QVERIFY(quadRoverGroup);
    QCOMPARE(quadRoverGroup->imageResource, QStringLiteral("qrc:/qmlimages/Airframe/QuadRoverHybrid.svg"));
    QCOMPARE(quadRoverGroup->rgAirframeInfo.size(), 2);
    QCOMPARE(quadRoverGroup->rgAirframeInfo.at(0)->autostartId, 12345);
    QCOMPARE(quadRoverGroup->rgAirframeInfo.at(0)->name, QStringLiteral("Legacy Airframe"));
    QCOMPARE(quadRoverGroup->rgAirframeInfo.at(1)->autostartId, 22001);
    QCOMPARE(quadRoverGroup->rgAirframeInfo.at(1)->name, QStringLiteral("Quad-Rover Hybrid"));
}

UT_REGISTER_TEST(PX4AirframeLoaderTest, TestLabel::Unit)
