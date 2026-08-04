#include "PX4AirframeLoader.h"
#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "AirframeComponentAirframes.h"
#include "AutoPilotPlugin.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(PX4AirframeLoaderLog, "AutoPilotPlugins.PX4AirframeLoader")

namespace {
constexpr const char* kBuiltInAirframeMetaData = ":/AutoPilotPlugins/PX4/AirframeFactMetaData.xml";
constexpr const char* kQGCAirframeMetaData = ":/AutoPilotPlugins/PX4/QGCAirframeFactMetaData.xml";
}  // namespace

bool PX4AirframeLoader::_airframeMetaDataLoaded = false;

PX4AirframeLoader::PX4AirframeLoader(AutoPilotPlugin* autopilot, QObject* parent)
{
    Q_UNUSED(autopilot);
    Q_UNUSED(parent);
}

QString PX4AirframeLoader::aiframeMetaDataFile(void)
{
    QSettings settings;
    QDir parameterDir = QFileInfo(settings.fileName()).dir();
    return parameterDir.filePath("PX4AirframeFactMetaData.xml");
}

/// Load Airframe Fact meta data
///
/// The meta data comes from firmware airframes.xml file.
void PX4AirframeLoader::loadAirframeMetaData(void)
{
    if (_airframeMetaDataLoaded) {
        return;
    }

    qCDebug(PX4AirframeLoaderLog) << "Loading PX4 airframe fact meta data";

    if (AirframeComponentAirframes::get().count() != 0) {
        qCWarning(PX4AirframeLoaderLog) << "Internal error";
        return;
    }

    QString airframeFilename;

    // We want unit test builds to always use the resource based meta data to provide repeatable results
    if (!QGC::runningUnitTests()) {
        // First look for meta data that comes from a firmware download. Fall back to resource if not there.
        airframeFilename = aiframeMetaDataFile();
    }
    if (airframeFilename.isEmpty() || !QFile(airframeFilename).exists()) {
        airframeFilename = QString::fromLatin1(kBuiltInAirframeMetaData);
    }

    if (!_loadAirframeMetaDataFile(airframeFilename, false)) {
        AirframeComponentAirframes::clear();

        const QString builtInAirframeFilename = QString::fromLatin1(kBuiltInAirframeMetaData);
        if (airframeFilename == builtInAirframeFilename || !_loadAirframeMetaDataFile(builtInAirframeFilename, false)) {
            AirframeComponentAirframes::clear();
            return;
        }
    }

    // Firmware metadata caches intentionally remain the base. This QGC-owned overlay only replaces the
    // airframe IDs declared in its dedicated resource, instead of merging the full built-in PX4 catalog.
    if (!_loadAirframeMetaDataFile(QString::fromLatin1(kQGCAirframeMetaData), true)) {
        AirframeComponentAirframes::clear();
        return;
    }

    _airframeMetaDataLoaded = true;
}

bool PX4AirframeLoader::_loadAirframeMetaDataFile(const QString& airframeFilename, bool replaceExistingAirframes)
{
    qCDebug(PX4AirframeLoaderLog) << "Loading meta data file:" << airframeFilename;

    QFile xmlFile(airframeFilename);
    if (!xmlFile.exists()) {
        qCWarning(PX4AirframeLoaderLog) << "Airframe XML does not exist:" << airframeFilename;
        return false;
    }

    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qCWarning(PX4AirframeLoaderLog) << "Failed opening airframe XML:" << airframeFilename;
        return false;
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();

    QString airframeGroup;
    QString image;
    int xmlState = XmlStateNone;

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "airframes") {
                if (xmlState != XmlStateNone) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }
                xmlState = XmlStateFoundAirframes;

            } else if (elementName == "version") {
                if (xmlState != XmlStateFoundAirframes) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }
                xmlState = XmlStateFoundVersion;

                bool convertOk;
                QString strVersion = xml.readElementText();
                int intVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }
                if (intVersion < 1) {
                    // We can't read these old files
                    qDebug() << "Airframe version stamp too old, skipping load. Found:" << intVersion
                             << "Want: 3 File:" << airframeFilename;
                    return false;
                }

            } else if (elementName == "airframe_version_major") {
                // Just skip over for now
            } else if (elementName == "airframe_version_minor") {
                // Just skip over for now

            } else if (elementName == "airframe_group") {
                if (xmlState != XmlStateFoundVersion) {
                    // We didn't get a version stamp, assume older version we can't read
                    qDebug() << "Parameter version stamp not found, skipping load" << airframeFilename;
                    return false;
                }
                xmlState = XmlStateFoundGroup;

                if (!xml.attributes().hasAttribute("name") || !xml.attributes().hasAttribute("image")) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }
                airframeGroup = xml.attributes().value("name").toString();
                image = xml.attributes().value("image").toString();
                qCDebug(PX4AirframeLoaderLog) << "Found group: " << airframeGroup << " image:" << image;

            } else if (elementName == "airframe") {
                if (xmlState != XmlStateFoundGroup) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }
                xmlState = XmlStateFoundAirframe;

                if (!xml.attributes().hasAttribute("name") || !xml.attributes().hasAttribute("id")) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }

                QString name = xml.attributes().value("name").toString();
                bool convertOk = false;
                int id = xml.attributes().value("id").toInt(&convertOk);
                if (!convertOk) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }

                qCDebug(PX4AirframeLoaderLog)
                    << "Found airframe name:" << name << " type:" << airframeGroup << " id:" << id;

                if (replaceExistingAirframes) {
                    _removeAirframe(id);
                }

                // Now that we know type we can airframe meta data object and add it to the system
                AirframeComponentAirframes::insert(airframeGroup, image, name, id, replaceExistingAirframes);

            } else {
                // We should be getting meta data now
                if (xmlState != XmlStateFoundAirframe) {
                    qCWarning(PX4AirframeLoaderLog) << "Badly formed XML";
                    return false;
                }
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "airframe") {
                // Reset for next airframe
                xmlState = XmlStateFoundGroup;
            } else if (elementName == "airframe_group") {
                xmlState = XmlStateFoundVersion;
            } else if (elementName == "airframes") {
                xmlState = XmlStateFoundAirframes;
            }
        }
        xml.readNext();
    }

    if (xml.hasError()) {
        qCWarning(PX4AirframeLoaderLog) << "Badly formed XML" << xml.errorString();
        return false;
    }

    return true;
}

void PX4AirframeLoader::_removeAirframe(int autostartId)
{
    auto& airframeTypes = AirframeComponentAirframes::get();
    auto groupIterator = airframeTypes.begin();

    while (groupIterator != airframeTypes.end()) {
        AirframeComponentAirframes::AirframeType_t* group = groupIterator.value();
        auto airframeIterator = group->rgAirframeInfo.begin();

        while (airframeIterator != group->rgAirframeInfo.end()) {
            AirframeComponentAirframes::AirframeInfo_t* airframe = *airframeIterator;
            if (airframe->autostartId == autostartId) {
                airframeIterator = group->rgAirframeInfo.erase(airframeIterator);
                delete airframe;
            } else {
                ++airframeIterator;
            }
        }

        if (group->rgAirframeInfo.isEmpty()) {
            delete group;
            groupIterator = airframeTypes.erase(groupIterator);
        } else {
            ++groupIterator;
        }
    }
}
