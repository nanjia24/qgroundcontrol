#include "VehicleLinkManager.h"
#include "MAVLinkLib.h"
#include "Vehicle.h"
#include "LinkManager.h"
#include "AppMessages.h"
#include "AudioOutput.h"
#include "TCPLink.h"
#include "UDPLink.h"
#ifndef QGC_NO_SERIAL_LINK
    #include "SerialLink.h"
#endif
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VehicleLinkManagerLog, "Vehicle.VehicleLinkManager")

namespace {

QString linkDetailsForConfig(const LinkConfiguration *config)
{
    if (!config) {
        return QString();
    }

    switch (config->type()) {
#ifndef QGC_NO_SERIAL_LINK
    case LinkConfiguration::TypeSerial: {
        const SerialConfiguration *const serialConfig = qobject_cast<const SerialConfiguration*>(config);
        if (!serialConfig) {
            break;
        }

        const QString portName = serialConfig->portDisplayName().isEmpty() ? serialConfig->portName() : serialConfig->portDisplayName();
        return VehicleLinkManager::tr("Serial %1 @ %2").arg(portName, QString::number(serialConfig->baud()));
    }
#endif
    case LinkConfiguration::TypeUdp: {
        const UDPConfiguration *const udpConfig = qobject_cast<const UDPConfiguration*>(config);
        if (!udpConfig) {
            break;
        }

        const QStringList hosts = udpConfig->hostList();
        if (!hosts.isEmpty()) {
            return VehicleLinkManager::tr("UDP %1").arg(hosts.join(QStringLiteral(", ")));
        }

        return VehicleLinkManager::tr("UDP listening on port %1").arg(udpConfig->localPort());
    }
    case LinkConfiguration::TypeTcp: {
        const TCPConfiguration *const tcpConfig = qobject_cast<const TCPConfiguration*>(config);
        if (!tcpConfig) {
            break;
        }

        return VehicleLinkManager::tr("TCP %1:%2").arg(tcpConfig->host(), QString::number(tcpConfig->port()));
    }
    case LinkConfiguration::TypeBluetooth:
        return VehicleLinkManager::tr("Bluetooth %1").arg(config->name());
    case LinkConfiguration::TypeLogReplay:
        return VehicleLinkManager::tr("Log Replay %1").arg(config->name());
#ifdef QT_DEBUG
    case LinkConfiguration::TypeMock:
        return VehicleLinkManager::tr("Mock %1").arg(config->name());
#endif
    case LinkConfiguration::TypeLast:
    default:
        break;
    }

    return config->name();
}

QString linkIdentifierForConfig(const LinkConfiguration *config)
{
    if (!config) {
        return QString();
    }

    switch (config->type()) {
#ifndef QGC_NO_SERIAL_LINK
    case LinkConfiguration::TypeSerial: {
        const SerialConfiguration *const serialConfig = qobject_cast<const SerialConfiguration*>(config);
        if (!serialConfig) {
            break;
        }

        return serialConfig->portDisplayName().isEmpty() ? serialConfig->portName() : serialConfig->portDisplayName();
    }
#endif
    case LinkConfiguration::TypeUdp: {
        const UDPConfiguration *const udpConfig = qobject_cast<const UDPConfiguration*>(config);
        if (!udpConfig) {
            break;
        }

        const QStringList hosts = udpConfig->hostList();
        if (!hosts.isEmpty()) {
            return hosts.first();
        }

        return VehicleLinkManager::tr("UDP:%1").arg(udpConfig->localPort());
    }
    case LinkConfiguration::TypeTcp: {
        const TCPConfiguration *const tcpConfig = qobject_cast<const TCPConfiguration*>(config);
        if (!tcpConfig) {
            break;
        }

        return tcpConfig->host();
    }
    case LinkConfiguration::TypeBluetooth:
    case LinkConfiguration::TypeLogReplay:
#ifdef QT_DEBUG
    case LinkConfiguration::TypeMock:
#endif
    case LinkConfiguration::TypeLast:
    default:
        break;
    }

    return config->name();
}

QString linkIdentifierForLink(const LinkInterface *link)
{
    if (!link) {
        return QString();
    }

    const UDPLink *const udpLink = qobject_cast<const UDPLink*>(link);
    if (udpLink && !udpLink->currentDataSourceAddress().isEmpty()) {
        return udpLink->currentDataSourceAddress();
    }

    return linkIdentifierForConfig(link->linkConfiguration().get());
}

QString linkDetailsForLink(const LinkInterface *link)
{
    if (!link) {
        return QString();
    }

    const UDPLink *const udpLink = qobject_cast<const UDPLink*>(link);
    if (udpLink && !udpLink->currentDataSourceAddress().isEmpty() && (udpLink->currentDataSourcePort() != 0)) {
        const UDPConfiguration *const udpConfig = qobject_cast<const UDPConfiguration*>(link->linkConfiguration().get());
        const quint16 localPort = udpConfig ? udpConfig->localPort() : 0;
        if (localPort != 0) {
            return VehicleLinkManager::tr("UDP %1:%2 -> L:%3").arg(
                udpLink->currentDataSourceAddress(),
                QString::number(udpLink->currentDataSourcePort()),
                QString::number(localPort)
            );
        }

        return VehicleLinkManager::tr("UDP %1:%2").arg(udpLink->currentDataSourceAddress(), QString::number(udpLink->currentDataSourcePort()));
    }

    return linkDetailsForConfig(link->linkConfiguration().get());
}

}

VehicleLinkManager::VehicleLinkManager(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _commLostCheckTimer(new QTimer(this))
{
    // qCDebug(VehicleLinkManagerLog) << Q_FUNC_INFO << this;

    (void) connect(this, &VehicleLinkManager::linkNamesChanged, this, &VehicleLinkManager::linkStatusesChanged);
    (void) connect(_commLostCheckTimer, &QTimer::timeout, this, &VehicleLinkManager::_commLostCheck);

    _commLostCheckTimer->setSingleShot(false);
    _commLostCheckTimer->setInterval(_commLostCheckTimeoutMSecs);
}

VehicleLinkManager::~VehicleLinkManager()
{
    // qCDebug(VehicleLinkManagerLog) << Q_FUNC_INFO << this;
}

void VehicleLinkManager::mavlinkMessageReceived(LinkInterface *link, const mavlink_message_t &message)
{
    // Radio status messages come from Sik Radios directly. It doesn't indicate there is any life on the other end.
    if (message.msgid == MAVLINK_MSG_ID_RADIO_STATUS) {
        return;
    }

    const int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        _addLink(link);
        return;
    }

    LinkInfo_t &linkInfo = _rgLinkInfo[linkIndex];
    const QString details = linkDetailsForLink(link);
    if (linkInfo.details != details) {
        const bool primaryLinkDetailsChanged = link == _primaryLink.lock().get();
        linkInfo.details = details;
        emit linkNamesChanged();
        if (primaryLinkDetailsChanged) {
            emit primaryLinkChanged();
        }
    }
    linkInfo.heartbeatElapsedTimer.restart();
    if (_rgLinkInfo[linkIndex].commLost) {
        _commRegainedOnLink(link);
    }
}

void VehicleLinkManager::_commRegainedOnLink(LinkInterface *link)
{

    const int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        return;
    }

    _rgLinkInfo[linkIndex].commLost = false;

    // Notify the user of communication regained
    QString commRegainedMessage;
    const bool isPrimaryLink = link == _primaryLink.lock().get();
    if (_rgLinkInfo.count() > 1) {
        commRegainedMessage = tr("%1Communication regained on %2 link").arg(_vehicle->_vehicleIdSpeech()).arg(isPrimaryLink ? tr("primary") : tr("secondary"));
    } else {
        commRegainedMessage = tr("%1Communication regained").arg(_vehicle->_vehicleIdSpeech());
    }

    // Try to switch to another link
    QString primarySwitchMessage;
    if (_updatePrimaryLink()) {
        primarySwitchMessage = tr("%1Switching communication to new primary link").arg(_vehicle->_vehicleIdSpeech());
    }

    if (!commRegainedMessage.isEmpty()) {
        AudioOutput::instance()->say(commRegainedMessage.toLower());
    }

    if (!primarySwitchMessage.isEmpty()) {
        AudioOutput::instance()->say(primarySwitchMessage.toLower());
        QGC::showAppMessage(primarySwitchMessage);
    }

    emit linkStatusesChanged();

    // Check recovery from total communication loss
    if (!_communicationLost) {
        return;
    }

    bool noCommunicationLoss = true;
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            noCommunicationLoss = false;
            break;
        }
    }

    if (noCommunicationLoss) {
        _communicationLost = false;
        emit communicationLostChanged(_communicationLost);
    }
}

void VehicleLinkManager::_commLostCheck()
{
    if (!_communicationLostEnabled) {
        return;
    }

    // Use much shorter heartbeat timeout in unit tests since MockLink sends heartbeats instantly
    const int heartbeatTimeout = QGC::runningUnitTests() ? kTestHeartbeatTimeoutMs : _heartbeatMaxElpasedMSecs;

    bool linkStatusChange = false;
    for (LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost && !linkInfo.link->linkConfiguration()->isHighLatency() && (linkInfo.heartbeatElapsedTimer.elapsed() > heartbeatTimeout)) {
            linkInfo.commLost = true;
            linkStatusChange = true;

            // Notify the user of individual link communication loss
            const bool isPrimaryLink = linkInfo.link.get() == _primaryLink.lock().get();
            if (_rgLinkInfo.count() > 1) {
                const QString msg = tr("%1Communication lost on %2 link.").arg(_vehicle->_vehicleIdSpeech()).arg(isPrimaryLink ? tr("primary") : tr("secondary"));
                AudioOutput::instance()->say(msg.toLower());
            }
        }
    }

    if (linkStatusChange) {
        emit linkStatusesChanged();
    }

    if (_updatePrimaryLink()) {
        QString msg = tr("%1Switching communication to secondary link.").arg(_vehicle->_vehicleIdSpeech());
        AudioOutput::instance()->say(msg.toLower());
        QGC::showAppMessage(msg);
    }

    if (_communicationLost) {
        return;
    }

    bool totalCommunicationLoss = true;
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost) {
            totalCommunicationLoss = false;
            break;
        }
    }

    if (totalCommunicationLoss) {
        if (_autoDisconnect) {
            // There is only one link to the vehicle and we want to auto disconnect from it
            closeVehicle();
            return;
        }

        AudioOutput::instance()->say(tr("%1Communication lost").arg(_vehicle->_vehicleIdSpeech()).toLower());

        _communicationLost = true;
        emit communicationLostChanged(_communicationLost);
    }
}

int VehicleLinkManager::_containsLinkIndex(const LinkInterface *link) const
{
    for (int i = 0; i < _rgLinkInfo.count(); i++) {
        if (_rgLinkInfo[i].link.get() == link) {
            return i;
        }
    }

    return -1;
}

void VehicleLinkManager::_addLink(LinkInterface *link)
{
    if (_containsLinkIndex(link) != -1) {
        qCWarning(VehicleLinkManagerLog) << "_addLink call with link which is already in the list";
        return;
    }

    SharedLinkInterfacePtr sharedLink = LinkManager::instance()->sharedLinkInterfacePointerForLink(link);
    if (!sharedLink) {
        qCDebug(VehicleLinkManagerLog) << "_addLink stale link" << (void*)link;
        return;
    }

    qCDebug(VehicleLinkManagerLog) << "_addLink:" << link->linkConfiguration()->name() << QString("%1").arg((qulonglong)link, 0, 16);

    link->addVehicleReference();

    LinkInfo_t linkInfo;
    linkInfo.link = sharedLink;
    linkInfo.details = linkDetailsForLink(link);
    if (!link->linkConfiguration()->isHighLatency()) {
        linkInfo.heartbeatElapsedTimer.start();
    }
    _rgLinkInfo.append(linkInfo);

    _updatePrimaryLink();

    (void) connect(link, &LinkInterface::disconnected, this, &VehicleLinkManager::_linkDisconnected);

    emit linkNamesChanged();

    if (_rgLinkInfo.count() == 1) {
        _commLostCheckTimer->start();
    }
}

void VehicleLinkManager::_removeLink(LinkInterface *link)
{
    const int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        qCWarning(VehicleLinkManagerLog) << "_removeLink call with link which is already in the list";
        return;
    }

    qCDebug(VehicleLinkManagerLog) << "_removeLink:" << QString("%1").arg((qulonglong)link, 0, 16);

    if (link == _primaryLink.lock().get()) {
        _primaryLink.reset();
        emit primaryLinkChanged();
    }

    disconnect(link, &LinkInterface::disconnected, this, &VehicleLinkManager::_linkDisconnected);
    link->removeVehicleReference();
    emit linkNamesChanged();
    _rgLinkInfo.removeAt(linkIndex); // Remove the link last since it may cause the link itself to be deleted

    if (_rgLinkInfo.isEmpty()) {
        _commLostCheckTimer->stop();
    }
}

void VehicleLinkManager::_linkDisconnected()
{
    qCDebug(VehicleLinkManagerLog) << Q_FUNC_INFO << "linkCount" << _rgLinkInfo.count();

    LinkInterface *link = qobject_cast<LinkInterface*>(sender());
    if (!link) {
        return;
    }

    _removeLink(link);
    _updatePrimaryLink();
    if (_rgLinkInfo.isEmpty() && !_allLinksRemovedSignalledByCloseVehicle) {
        qCDebug(VehicleLinkManagerLog) << "signalling allLinksRemoved";
        // Stop command processing timers immediately to prevent callbacks during the
        // asynchronous vehicle destruction sequence
        _vehicle->_stopCommandProcessing();
        emit allLinksRemoved(_vehicle);
    }
}

SharedLinkInterfacePtr VehicleLinkManager::_bestActivePrimaryLink()
{
#ifndef QGC_NO_SERIAL_LINK
    // Best choice is a USB connection
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            continue;
        }

        SharedLinkInterfacePtr candidateLink = linkInfo.link;
        auto linkInterface = candidateLink.get();
        if (linkInterface && LinkManager::isLinkUSBDirect(linkInterface)) {
            return candidateLink;
        }
    }
#endif

    // Next best is normal latency link
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            continue;
        }

        SharedLinkInterfacePtr candidateLink = linkInfo.link;
        const SharedLinkConfigurationPtr config = candidateLink->linkConfiguration();
        if (config && !config->isHighLatency()) {
            return candidateLink;
        }
    }

    // Last possible choice is a high latency link
    SharedLinkInterfacePtr primaryLink = _primaryLink.lock();
    if (primaryLink && primaryLink->linkConfiguration()->isHighLatency()) {
        // Best choice continues to be the current high latency link
        return primaryLink;
    }

    // Pick any high latency link if one exists
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            continue;
        }

        SharedLinkInterfacePtr candidateLink = linkInfo.link;
        const SharedLinkConfigurationPtr config = candidateLink->linkConfiguration();
        if (config && config->isHighLatency()) {
            return candidateLink;
        }
    }

    return {};
}

bool VehicleLinkManager::_updatePrimaryLink()
{
    SharedLinkInterfacePtr primaryLink = _primaryLink.lock();
    const int linkIndex = _containsLinkIndex(primaryLink.get());

    if ((linkIndex != -1) && !_rgLinkInfo[linkIndex].commLost && !primaryLink->linkConfiguration()->isHighLatency()) {
        // Current priority link is still valid
        return false;
    }

    SharedLinkInterfacePtr bestActivePrimaryLink = _bestActivePrimaryLink();
    if ((linkIndex != -1) && !bestActivePrimaryLink) {
        // Nothing better available, leave things set to current primary link
        return false;
    }

    if (bestActivePrimaryLink == primaryLink) {
        return false;
    }

    _vehicle->_pidTuningPrimaryLinkAboutToChange();

    if (primaryLink && primaryLink->linkConfiguration()->isHighLatency()) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_AUTOPILOT1,
            MAV_CMD_CONTROL_HIGH_LATENCY,
            true,
            0 // Stop transmission on this link
        );
    }

    _primaryLink = bestActivePrimaryLink;
    emit primaryLinkChanged();

    if (bestActivePrimaryLink && bestActivePrimaryLink->linkConfiguration()->isHighLatency()) {
        _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                       MAV_CMD_CONTROL_HIGH_LATENCY,
                       true,
                       1); // Start transmission on this link
    }

    return true;
}

void VehicleLinkManager::closeVehicle()
{
    // Vehicle is no longer communicating with us. Remove all link references

    const QList<LinkInfo_t> rgLinkInfoCopy = _rgLinkInfo;
    for (const LinkInfo_t &linkInfo: rgLinkInfoCopy) {
        _removeLink(linkInfo.link.get());
    }

    _rgLinkInfo.clear();

    _allLinksRemovedSignalledByCloseVehicle = true; // Prevent double signal of allLinksRemoved
    // Stop command processing timers immediately to prevent callbacks during the
    // asynchronous vehicle destruction sequence
    _vehicle->_stopCommandProcessing();
    emit allLinksRemoved(_vehicle);
}

void VehicleLinkManager::setCommunicationLostEnabled(bool communicationLostEnabled)
{
    if (_communicationLostEnabled != communicationLostEnabled) {
        _communicationLostEnabled = communicationLostEnabled;
        emit communicationLostEnabledChanged(communicationLostEnabled);
    }
}

bool VehicleLinkManager::containsLink(LinkInterface* link)
{
    return (_containsLinkIndex(link) != -1);
}

QString VehicleLinkManager::primaryLinkName() const
{
    if (!_primaryLink.expired()) {
        return _primaryLink.lock()->linkConfiguration()->name();
    }

    return QString();
}

QString VehicleLinkManager::primaryLinkIdentifier() const
{
    if (!_primaryLink.expired()) {
        return linkIdentifierForLink(_primaryLink.lock().get());
    }

    return QString();
}

QString VehicleLinkManager::primaryLinkDetails() const
{
    if (_primaryLink.expired()) {
        return QString();
    }

    const SharedLinkInterfacePtr primaryLink = _primaryLink.lock();
    const int linkIndex = _containsLinkIndex(primaryLink.get());
    if (linkIndex != -1) {
        return _rgLinkInfo[linkIndex].details;
    }

    return linkDetailsForLink(primaryLink.get());
}

void VehicleLinkManager::setPrimaryLinkByName(const QString& name)
{
    for (const LinkInfo_t& linkInfo : _rgLinkInfo) {
        if (linkInfo.link->linkConfiguration()->name() == name) {
            if (_primaryLink.lock() == linkInfo.link) {
                return;
            }
            _vehicle->_pidTuningPrimaryLinkAboutToChange();
            _primaryLink = linkInfo.link;
            emit primaryLinkChanged();
            return;
        }
    }
}

QStringList VehicleLinkManager::linkNames() const
{
    QStringList rgNames;

    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        rgNames.append(linkInfo.link->linkConfiguration()->name());
    }

    return rgNames;
}

QStringList VehicleLinkManager::linkDetails() const
{
    QStringList rgDetails;

    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        rgDetails.append(linkInfo.details);
    }

    return rgDetails;
}

QStringList VehicleLinkManager::linkStatuses() const
{
    QStringList rgStatuses;

    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        rgStatuses.append(linkInfo.commLost ? tr("Comm Lost") : "");
    }

    return rgStatuses;
}
