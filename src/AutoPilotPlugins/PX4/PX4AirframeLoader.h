#pragma once

#include <QtCore/QObject>
#include <QtCore/QMap>
class AutoPilotPlugin;

class FactMetaData;

/// Collection of Parameter Facts for PX4 AutoPilot

class PX4AirframeLoader : QObject
{
    Q_OBJECT

    friend class PX4AirframeLoaderTest;

public:
    /// @param uas Uas which this set of facts is associated with
    PX4AirframeLoader(AutoPilotPlugin* autpilot, QObject* parent = nullptr);

    static void loadAirframeMetaData(void);

    /// @return Location of PX4 airframe fact meta data file
    static QString aiframeMetaDataFile(void);

private:
    static bool _loadAirframeMetaDataFile(const QString& airframeFilename, bool replaceExistingAirframes);
    static void _removeAirframe(int autostartId);

    enum {
        XmlStateNone,
        XmlStateFoundAirframes,
        XmlStateFoundVersion,
        XmlStateFoundGroup,
        XmlStateFoundAirframe,
        XmlStateDone
    };

    static bool _airframeMetaDataLoaded;   ///< true: parameter meta data already loaded
    static QMap<QString, FactMetaData*> _mapParameterName2FactMetaData; ///< Maps from a parameter name to FactMetaData
};
