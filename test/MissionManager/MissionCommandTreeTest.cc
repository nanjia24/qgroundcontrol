#include "MissionCommandTreeTest.h"

#include "MissionCommandList.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "PX4FirmwarePlugin.h"
#include "UnitTest.h"
#include "Vehicle.h"

#include <algorithm>

void MissionCommandTreeTest::init()
{
    UnitTest::init();
    _commandTree = new MissionCommandTree(true /* unitTest */, this);
}

void MissionCommandTreeTest::cleanup()
{
    UnitTest::cleanup();
}

QString MissionCommandTreeTest::_rawName(int id) const
{
    return QStringLiteral("UNITTEST_%1").arg(id);
}

QString MissionCommandTreeTest::_friendlyName(int id) const
{
    return QStringLiteral("Unit Test %1").arg(id);
}

QString MissionCommandTreeTest::_paramLabel(int index) const
{
    return QStringLiteral("param%1").arg(index);
}

void MissionCommandTreeTest::_checkFullInfoMap(const MissionCommandUIInfo* const uiInfo)
{
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_rawNameJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_categoryJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_descriptionJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_friendlyEditJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_friendlyNameJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_standaloneCoordinateJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_specifiesCoordinateJsonKey));
}

void MissionCommandTreeTest::_checkBaseValues(const MissionCommandUIInfo* const uiInfo, int command)
{
    QVERIFY(uiInfo != nullptr);
    _checkFullInfoMap(uiInfo);
    QCOMPARE(uiInfo->command(), static_cast<MAV_CMD>(command));
    QCOMPARE(uiInfo->rawName(), _rawName(command));
    QCOMPARE(uiInfo->category(), QStringLiteral("category"));
    QCOMPARE(uiInfo->description(), QStringLiteral("description"));
    QCOMPARE(uiInfo->friendlyEdit(), true);
    QCOMPARE(uiInfo->friendlyName(), _friendlyName(command));
    QCOMPARE(uiInfo->isStandaloneCoordinate(), true);
    QCOMPARE(uiInfo->specifiesCoordinate(), true);
    for (int i = 1; i <= 7; i++) {
        bool showUI;
        const MissionCmdParamInfo* const paramInfo = uiInfo->getParamInfo(i, showUI);
        QVERIFY(showUI);
        QVERIFY(paramInfo);
        QCOMPARE(paramInfo->decimalPlaces(), 1);
        QCOMPARE(paramInfo->defaultValue(), 1.0);
        QCOMPARE(paramInfo->enumStrings().count(), 2);
        QCOMPARE(paramInfo->enumStrings()[0], QStringLiteral("1"));
        QCOMPARE(paramInfo->enumStrings()[1], QStringLiteral("2"));
        QCOMPARE(paramInfo->enumValues().count(), 2);
        QCOMPARE(paramInfo->enumValues()[0].toDouble(), 1.0);
        QCOMPARE(paramInfo->enumValues()[1].toDouble(), 2.0);
        QCOMPARE(paramInfo->label(), _paramLabel(i));
        QCOMPARE(paramInfo->param(), i);
        QCOMPARE(paramInfo->units(), QStringLiteral("units"));
    }
}

void MissionCommandTreeTest::_checkOverrideParamValues(const MissionCommandUIInfo* uiInfo, int command, int paramIndex)
{
    QString overrideString = QStringLiteral("override fw %1 %2").arg(command).arg(paramIndex);
    bool showUI;
    const MissionCmdParamInfo* const paramInfo = uiInfo->getParamInfo(paramIndex, showUI);
    QVERIFY(showUI);
    QVERIFY(paramInfo);
    QCOMPARE(paramInfo->decimalPlaces(), 1);
    QCOMPARE(paramInfo->defaultValue(), 1.0);
    QCOMPARE(paramInfo->enumStrings().count(), 2);
    QCOMPARE(paramInfo->enumStrings()[0], QStringLiteral("1"));
    QCOMPARE(paramInfo->enumStrings()[1], QStringLiteral("2"));
    QCOMPARE(paramInfo->enumValues().count(), 2);
    QCOMPARE(paramInfo->enumValues()[0].toDouble(), 1.0);
    QCOMPARE(paramInfo->enumValues()[1].toDouble(), 2.0);
    QCOMPARE(paramInfo->label(), overrideString);
    QCOMPARE(paramInfo->param(), paramIndex);
    QCOMPARE(paramInfo->units(), overrideString);
}

void MissionCommandTreeTest::_checkOverrideValues(const MissionCommandUIInfo* uiInfo, int command)
{
    bool showUI;
    QString overrideString = QString("override fw %1").arg(command);
    QVERIFY(uiInfo != nullptr);
    _checkFullInfoMap(uiInfo);
    QCOMPARE(uiInfo->command(), static_cast<MAV_CMD>(command));
    QCOMPARE(uiInfo->rawName(), _rawName(command));
    QCOMPARE(uiInfo->category(), overrideString);
    QCOMPARE(uiInfo->description(), overrideString);
    QCOMPARE(uiInfo->friendlyEdit(), true);
    QCOMPARE(uiInfo->friendlyName(), _friendlyName(command));
    QCOMPARE(uiInfo->isStandaloneCoordinate(), false);
    QCOMPARE(uiInfo->specifiesCoordinate(), false);
    QVERIFY(uiInfo->getParamInfo(2, showUI));
    QCOMPARE(showUI, false);
    QVERIFY(uiInfo->getParamInfo(4, showUI));
    QCOMPARE(showUI, false);
    QVERIFY(uiInfo->getParamInfo(6, showUI));
    QCOMPARE(showUI, false);
    _checkOverrideParamValues(uiInfo, command, 1);
    _checkOverrideParamValues(uiInfo, command, 3);
    _checkOverrideParamValues(uiInfo, command, 5);
}

void MissionCommandTreeTest::testJsonLoad()
{
    // Test loading from the bad command list
    MissionCommandList* const commandList = _commandTree->_staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC];
    QVERIFY(commandList != nullptr);
    // Command 1 should have all values defaulted, no params
    const MissionCommandUIInfo* uiInfo = commandList->getUIInfo(static_cast<MAV_CMD>(1));
    QVERIFY(uiInfo != nullptr);
    _checkFullInfoMap(uiInfo);
    QCOMPARE(uiInfo->command(), static_cast<MAV_CMD>(1));
    QCOMPARE(uiInfo->rawName(), _rawName(1));
    QVERIFY(uiInfo->category() == MissionCommandUIInfo::_advancedCategory);
    QVERIFY(uiInfo->description().isEmpty());
    QCOMPARE(uiInfo->friendlyEdit(), false);
    QCOMPARE(uiInfo->friendlyName(), uiInfo->rawName());
    QCOMPARE(uiInfo->isStandaloneCoordinate(), false);
    QCOMPARE(uiInfo->specifiesCoordinate(), false);
    bool showUI;
    for (int i = 1; i <= 7; i++) {
        QVERIFY(uiInfo->getParamInfo(i, showUI) == nullptr);
        QCOMPARE(showUI, false);
    }
    // Command 2 should all values defaulted for param 1
    uiInfo = commandList->getUIInfo(static_cast<MAV_CMD>(2));
    QVERIFY(uiInfo != nullptr);
    const MissionCmdParamInfo* const paramInfo = uiInfo->getParamInfo(1, showUI);
    QVERIFY(paramInfo);
    QCOMPARE(showUI, true);
    QCOMPARE(paramInfo->decimalPlaces(), -1);
    QCOMPARE(paramInfo->defaultValue(), 0.0);
    QCOMPARE(paramInfo->enumStrings().count(), 0);
    QCOMPARE(paramInfo->enumValues().count(), 0);
    QCOMPARE(paramInfo->label(), _paramLabel(1));
    QCOMPARE(paramInfo->param(), 1);
    QVERIFY(paramInfo->units().isEmpty());
    for (int i = 2; i <= 7; i++) {
        QVERIFY(uiInfo->getParamInfo(i, showUI) == nullptr);
        QCOMPARE(showUI, false);
    }
    // Command 3 should have all values set
    _checkBaseValues(commandList->getUIInfo(static_cast<MAV_CMD>(3)), 3);
}

void MissionCommandTreeTest::testOverride()
{
    // Generic/Generic should not have any overrides
    Vehicle* vehicle = new Vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC, this);
    _checkBaseValues(_commandTree->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, static_cast<MAV_CMD>(4)), 4);
    delete vehicle;
    // Generic/FixedWing should have overrides
    vehicle = new Vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_FIXED_WING, this);
    _checkOverrideValues(_commandTree->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, static_cast<MAV_CMD>(4)), 4);
    delete vehicle;
}

void MissionCommandTreeTest::testAllTrees()
{
    QList<MAV_AUTOPILOT> firmwareList;
    QList<MAV_TYPE> vehicleList;
    firmwareList << MAV_AUTOPILOT_GENERIC << MAV_AUTOPILOT_PX4 << MAV_AUTOPILOT_ARDUPILOTMEGA;
    vehicleList << MAV_TYPE_GENERIC << MAV_TYPE_QUADROTOR << MAV_TYPE_FIXED_WING << MAV_TYPE_GROUND_ROVER
                << MAV_TYPE_SUBMARINE << MAV_TYPE_VTOL_TAILSITTER_QUADROTOR;
    // This will cause all of the variants of collapsed trees to be built
    for (MAV_AUTOPILOT firmwareType : firmwareList) {
        for (MAV_TYPE vehicleType : vehicleList) {
            if ((firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) && (vehicleType == MAV_TYPE_VTOL_TAILSITTER_QUADROTOR)) {
                // VTOL in ArduPilot shows up as plane so we can test this pair
                continue;
            }
            qDebug() << firmwareType << vehicleType;
            Vehicle* const vehicle = new Vehicle(firmwareType, vehicleType, this);
            QVERIFY(MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassMultiRotor,
                                                              MAV_CMD_NAV_WAYPOINT) != nullptr);
            delete vehicle;
        }
    }
}

void MissionCommandTreeTest::testHybridTransitionCommandIsQuadRoverOnly()
{
    PX4FirmwarePlugin px4FirmwarePlugin;
    QCOMPARE(px4FirmwarePlugin.missionCommandOverrides(QGCMAVLink::VehicleClassQuadRover),
             QStringLiteral(":/json/PX4-MavCmdInfoQuadRover.json"));

    const MAV_CMD hybridTransitionCommand = MAV_CMD_DO_HYBRID_TRANSITION;
    Vehicle quadRover(MAV_AUTOPILOT_PX4, MAV_TYPE_QUAD_ROVER, this);
    MissionCommandTree* const commandTree = MissionCommandTree::instance();

    const MissionCommandUIInfo* const hybridInfo =
        commandTree->getUIInfo(&quadRover, QGCMAVLink::VehicleClassGeneric, hybridTransitionCommand);
    QVERIFY(hybridInfo);
    QCOMPARE(hybridInfo->command(), hybridTransitionCommand);
    QVERIFY(!hybridInfo->specifiesCoordinate());
    QVERIFY(!hybridInfo->isStandaloneCoordinate());
    bool showUI = false;
    const MissionCmdParamInfo* const param1Info = hybridInfo->getParamInfo(1, showUI);
    QVERIFY(showUI);
    QVERIFY(param1Info);
    QCOMPARE(param1Info->enumStrings(), QStringList({QStringLiteral("Quad"), QStringLiteral("Rover")}));
    QCOMPARE(param1Info->enumValues(), QVariantList({1, 2}));
    for (int parameter = 2; parameter <= 7; ++parameter) {
        QVERIFY(hybridInfo->getParamInfo(parameter, showUI) == nullptr);
        QVERIFY(!showUI);
    }

    QVERIFY(px4FirmwarePlugin.supportedMissionCommands(QGCMAVLink::VehicleClassQuadRover)
                .contains(hybridTransitionCommand));
    QVERIFY(!px4FirmwarePlugin.supportedMissionCommands(QGCMAVLink::VehicleClassMultiRotor)
                 .contains(hybridTransitionCommand));
    QVERIFY(
        !px4FirmwarePlugin.supportedMissionCommands(QGCMAVLink::VehicleClassVTOL).contains(hybridTransitionCommand));
    QVERIFY(!px4FirmwarePlugin.supportedMissionCommands(QGCMAVLink::VehicleClassRoverBoat)
                 .contains(hybridTransitionCommand));

    const auto commandIsExposed = [commandTree, hybridTransitionCommand](Vehicle* vehicle) {
        const QStringList categories = commandTree->categoriesForVehicle(vehicle);
        for (const QString& category : categories) {
            const QVariantList commands = commandTree->getCommandsForCategory(vehicle, category, true);
            if (std::any_of(commands.cbegin(), commands.cend(), [hybridTransitionCommand](const QVariant& command) {
                    const MissionCommandUIInfo* const uiInfo = command.value<MissionCommandUIInfo*>();
                    return uiInfo && uiInfo->command() == hybridTransitionCommand;
                })) {
                return true;
            }
        }
        return false;
    };

    Vehicle multirotor(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, this);
    Vehicle vtol(MAV_AUTOPILOT_PX4, MAV_TYPE_VTOL_TILTROTOR, this);
    Vehicle rover(MAV_AUTOPILOT_PX4, MAV_TYPE_GROUND_ROVER, this);
    Vehicle generic(MAV_AUTOPILOT_PX4, MAV_TYPE_GENERIC, this);

    QVERIFY(commandIsExposed(&quadRover));
    QVERIFY(!commandIsExposed(&multirotor));
    QVERIFY(!commandIsExposed(&vtol));
    QVERIFY(!commandIsExposed(&rover));
    QVERIFY(!commandIsExposed(&generic));
}

void MissionCommandTreeTest::testUnknownCommandFallbacks()
{
    constexpr MAV_CMD unknownCommand = static_cast<MAV_CMD>(9999);
    QCOMPARE(_commandTree->friendlyName(unknownCommand), QStringLiteral("MAV_CMD(9999)"));
    QCOMPARE(_commandTree->rawName(unknownCommand), QStringLiteral("MAV_CMD(9999)"));
    QVERIFY(!_commandTree->isLandCommand(unknownCommand));
    QVERIFY(!_commandTree->isTakeoffCommand(unknownCommand));
}

UT_REGISTER_TEST(MissionCommandTreeTest, TestLabel::Unit, TestLabel::MissionManager)
