#include "MavCommandQueue.h"

#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include "FirmwarePlugin.h"
#include "LinkManager.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MissionCommandTree.h"
#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

QGC_LOGGING_CATEGORY(MavCommandQueueLog, "Vehicle.MavCommandQueue")

MavCommandQueue::MavCommandQueue(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    _responseCheckTimer.setSingleShot(false);
    _responseCheckTimer.setInterval(_responseCheckIntervalMSecs());
    _responseCheckTimer.start();
    connect(&_responseCheckTimer, &QTimer::timeout, this, &MavCommandQueue::_responseTimeoutCheck);
}

MavCommandQueue::~MavCommandQueue()
{
    _responseCheckTimer.stop();
    _responseCheckTimer.disconnect();
}

void MavCommandQueue::stop()
{
    qCDebug(MavCommandQueueLog) << "stop - clearing pending commands and halting response timer";
    _stopped = true;
    _responseCheckTimer.stop();
    _responseCheckTimer.disconnect();
    _list.clear();
    _setMessageIntervalTombstones.clear();
    _observedSetMessageIntervalLinks.clear();
}

void MavCommandQueue::sendCommand(int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    sendWorker(false,                   // commandInt
               showError,
               nullptr,                 // no handlers
               compId,
               command,
               MAV_FRAME_GLOBAL,
               param1, param2, param3, param4, param5, param6, param7);
}

void MavCommandQueue::sendCommandDelayed(int compId, MAV_CMD command, bool showError, int milliseconds, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    QTimer::singleShot(milliseconds, this, [=, this] {
        sendCommand(compId, command, showError, param1, param2, param3, param4, param5, param6, param7);
    });
}

void MavCommandQueue::sendCommandInt(int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    sendWorker(true,                    // commandInt
               showError,
               nullptr,                 // no handlers
               compId,
               command,
               frame,
               param1, param2, param3, param4, param5, param6, param7);
}

void MavCommandQueue::sendCommandWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    sendWorker(false,                   // commandInt
               false,                   // showError
               ackHandlerInfo,
               compId,
               command,
               MAV_FRAME_GLOBAL,
               param1, param2, param3, param4, param5, param6, param7);
}

void MavCommandQueue::sendCommandWithHandlerOnLink(const MavCmdAckHandlerInfo_t* ackHandlerInfo,
                                                   const SharedLinkInterfacePtr& link, int compId,
                                                   MAV_CMD command, float param1, float param2, float param3,
                                                   float param4, float param5, float param6, float param7)
{
    sendWorker(false,                   // commandInt
               false,                   // showError
               ackHandlerInfo,
               compId,
               command,
               MAV_FRAME_GLOBAL,
               param1, param2, param3, param4, param5, param6, param7,
               link);
}

void MavCommandQueue::sendCommandIntWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, MAV_FRAME frame, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    sendWorker(true,                    // commandInt
               false,                   // showError
               ackHandlerInfo,
               compId,
               command,
               frame,
               param1, param2, param3, param4, param5, param6, param7);
}

namespace {

struct LambdaFallbackHandlerData {
    Vehicle*              vehicle;
    bool                  showError;
    std::function<void()> unsupportedLambda;
};

void lambdaFallbackResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, VehicleTypes::MavCmdResultFailureCode_t /*failureCode*/)
{
    auto* data          = static_cast<LambdaFallbackHandlerData*>(resultHandlerData);
    auto* instanceData  = data->vehicle->firmwarePluginInstanceData();

    switch (ack.result) {
    case MAV_RESULT_ACCEPTED:
        instanceData->setCommandSupported(MAV_CMD(ack.command), FirmwarePluginInstanceData::CommandSupportedResult::SUPPORTED);
        break;
    case MAV_RESULT_UNSUPPORTED:
        instanceData->setCommandSupported(MAV_CMD(ack.command), FirmwarePluginInstanceData::CommandSupportedResult::UNSUPPORTED);
        data->unsupportedLambda();
        break;
    default:
        if (data->showError) {
            MavCommandQueue::showCommandAckError(ack);
        }
        break;
    }

    delete data;
}

} // namespace

void MavCommandQueue::sendCommandWithLambdaFallback(std::function<void()> lambda, int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    auto* instanceData = _vehicle->firmwarePluginInstanceData();

    switch (instanceData->getCommandSupported(command)) {
    case FirmwarePluginInstanceData::CommandSupportedResult::UNSUPPORTED:
        lambda();
        break;
    case FirmwarePluginInstanceData::CommandSupportedResult::SUPPORTED:
        sendCommand(compId, command, showError, param1, param2, param3, param4, param5, param6, param7);
        break;
    case FirmwarePluginInstanceData::CommandSupportedResult::UNKNOWN: {
        auto* data = new LambdaFallbackHandlerData { _vehicle, showError, std::move(lambda) };
        const MavCmdAckHandlerInfo_t handlerInfo {
            /* .resultHandler         = */ &lambdaFallbackResultHandler,
            /* .resultHandlerData     = */ data,
            /* .progressHandler       = */ nullptr,
            /* .progressHandlerData   = */ nullptr,
        };
        sendCommandWithHandler(&handlerInfo, compId, command, param1, param2, param3, param4, param5, param6, param7);
        break;
    }
    }
}

bool MavCommandQueue::isPending(int targetCompId, MAV_CMD command) const
{
    return findEntryIndex(targetCompId, command) != -1;
}

int MavCommandQueue::findEntryIndex(int targetCompId, MAV_CMD command) const
{
    for (int i = 0; i < _list.count(); i++) {
        const MavCommandListEntry_t& entry = _list[i];
        if (entry.targetCompId == targetCompId && entry.command == command) {
            return i;
        }
    }
    return -1;
}

int MavCommandQueue::_responseCheckIntervalMSecs()
{
    // Use shorter check interval during unit tests for faster test execution
    return QGC::runningUnitTests() ? 50 : 500;
}

int MavCommandQueue::_ackTimeoutMSecs(MAV_CMD command, const LinkInterface* link) const
{
    // Use shorter ack timeout during unit tests for faster test execution
    if (QGC::runningUnitTests()) {
        return kTestAckTimeoutMs;
    }

    // PX4 moves stream subscription changes onto its MAVLink main thread. USB
    // startup load can delay that handoff, and this ACK cannot identify param1.
    if (_vehicle->px4Firmware() && command == MAV_CMD_SET_MESSAGE_INTERVAL && LinkManager::isLinkUSBDirect(link)) {
        return _usbSetMessageIntervalAckTimeoutMSecs;
    }

    return 1200;
}

bool MavCommandQueue::_shouldRetry(MAV_CMD command)
{
    switch (command) {
#ifdef QT_DEBUG
    // These MockLink commands should be retried so we can create unit tests to test retry code
    case MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE:
        return true;
#endif

        // In general we should not retry any commands. This is for safety reasons. For example you don't want an ARM command
        // to timeout with no response over a noisy link twice and then suddenly have the third try work 6 seconds later. At that
        // point the user could have walked up to the vehicle to see what is going wrong.
        //
        // We do retry commands which are part of the initial vehicle connect sequence. This makes this process work better over noisy
        // links where commands could be lost. Also these commands tend to just be requesting status so if they end up being delayed
        // there are no safety concerns that could occur.
    case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
    case MAV_CMD_REQUEST_MESSAGE:
    case MAV_CMD_PREFLIGHT_STORAGE:
    case MAV_CMD_RUN_PREARM_CHECKS:
        return true;

    default:
        return false;
    }
}

bool MavCommandQueue::_canBeDuplicated(MAV_CMD command)
{
    // For some commands we don't care about response as much as we care about sending them regularly.
    // This test avoids commands not being sent due to an ACK not being received yet.
    // MOTOR_TEST in ardusub is a case where we need a constant stream of commands so it doesn't time out.
    switch (command) {
    case MAV_CMD_DO_MOTOR_TEST:
        return true;
    default:
        return false;
    }
}

bool MavCommandQueue::_mustBeSerialized(MAV_CMD command)
{
    // COMMAND_ACK does not echo the message id from param1. Sending more than one
    // interval request to the same component and link would make their ACKs ambiguous.
    return command == MAV_CMD_SET_MESSAGE_INTERVAL;
}

bool MavCommandQueue::_hasSetMessageIntervalEntry(int targetCompId, LinkInterface* targetLink) const
{
    for (const MavCommandListEntry_t& entry : _list) {
        if (entry.targetCompId == targetCompId && entry.command == MAV_CMD_SET_MESSAGE_INTERVAL &&
            entry.targetLinkIdentity == targetLink) {
            return true;
        }
    }
    return false;
}

bool MavCommandQueue::_isSetMessageIntervalQuarantined(int targetCompId, LinkInterface* targetLink) const
{
    for (const SetMessageIntervalTombstone& tombstone : _setMessageIntervalTombstones) {
        if (tombstone.targetCompId == targetCompId && tombstone.targetLink == targetLink) {
            return true;
        }
    }
    return false;
}

void MavCommandQueue::_observeSetMessageIntervalLink(LinkInterface* targetLink)
{
    if (!targetLink || _observedSetMessageIntervalLinks.contains(targetLink)) {
        return;
    }

    _observedSetMessageIntervalLinks.insert(targetLink);
    connect(targetLink, &LinkInterface::disconnected, this, [this, targetLink]() {
        _clearSetMessageIntervalTombstones(targetLink);
        _failSetMessageIntervalEntriesForLink(targetLink);
    });
    connect(targetLink, &QObject::destroyed, this, [this, targetLink]() {
        _clearSetMessageIntervalTombstones(targetLink);
        _failSetMessageIntervalEntriesForLink(targetLink);
        _observedSetMessageIntervalLinks.remove(targetLink);
    });
}

void MavCommandQueue::_failSetMessageIntervalEntriesForLink(LinkInterface* targetLink)
{
    QList<MavCommandListEntry_t> failedEntries;
    for (int i = _list.count() - 1; i >= 0; --i) {
        const MavCommandListEntry_t& entry = _list[i];
        if (entry.command == MAV_CMD_SET_MESSAGE_INTERVAL && entry.targetLinkIdentity == targetLink) {
            failedEntries.prepend(_list.takeAt(i));
        }
    }

    for (const MavCommandListEntry_t& failedEntry : std::as_const(failedEntries)) {
        QPointer<MavCommandQueue> guard(this);
        if (failedEntry.ackHandlerInfo.resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.command = failedEntry.command;
            ack.result = MAV_RESULT_FAILED;
            (*failedEntry.ackHandlerInfo.resultHandler)(failedEntry.ackHandlerInfo.resultHandlerData,
                                                        failedEntry.targetCompId, ack,
                                                        MavCmdResultFailureNoResponseToCommand);
        } else {
            emit commandResult(_vehicle->id(), failedEntry.targetCompId, failedEntry.command,
                               MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        }
        if (!guard) {
            return;
        }
        if (failedEntry.showError) {
            QGC::showAppMessage(tr("Vehicle did not respond to command: %1")
                                    .arg(MissionCommandTree::instance()->friendlyName(failedEntry.command)));
        }
    }
}

void MavCommandQueue::_addSetMessageIntervalTombstone(int targetCompId, LinkInterface* targetLink)
{
    if (!targetLink || !targetLink->isConnected() ||
        _isSetMessageIntervalQuarantined(targetCompId, targetLink)) {
        return;
    }

    _setMessageIntervalTombstones.append({targetCompId, targetLink});
    _observeSetMessageIntervalLink(targetLink);
}

bool MavCommandQueue::_consumeSetMessageIntervalTombstone(int targetCompId, LinkInterface* targetLink)
{
    for (int i = 0; i < _setMessageIntervalTombstones.count(); ++i) {
        const SetMessageIntervalTombstone& tombstone = _setMessageIntervalTombstones[i];
        if (tombstone.targetCompId == targetCompId && tombstone.targetLink == targetLink) {
            _setMessageIntervalTombstones.removeAt(i);
            return true;
        }
    }
    return false;
}

void MavCommandQueue::_clearSetMessageIntervalTombstones(LinkInterface* targetLink)
{
    for (int i = _setMessageIntervalTombstones.count() - 1; i >= 0; --i) {
        if (_setMessageIntervalTombstones[i].targetLink == targetLink ||
            _setMessageIntervalTombstones[i].targetLink.isNull()) {
            _setMessageIntervalTombstones.removeAt(i);
        }
    }
}

QString MavCommandQueue::_formatCommand(MAV_CMD command, float param1)
{
    QString rawName = MissionCommandTree::instance()->rawName(command);
    QString friendlyName = MissionCommandTree::instance()->friendlyName(command);
    QString commandStr = friendlyName.isEmpty() ? rawName : QStringLiteral("%1 (%2)").arg(friendlyName, rawName);

    if (command == MAV_CMD_REQUEST_MESSAGE) {
        const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<int>(param1));
        if (info) {
            commandStr += QStringLiteral(" [%1]").arg(info->name);
        }
    }

    return commandStr;
}

void MavCommandQueue::sendWorker(bool commandInt, bool showError,
                                 const MavCmdAckHandlerInfo_t* ackHandlerInfo,
                                 int targetCompId, MAV_CMD command, MAV_FRAME frame,
                                 float param1, float param2, float param3, float param4, double param5, double param6,
                                 float param7, const SharedLinkInterfacePtr& targetLink)
{
    if (_stopped) {
        return;
    }

    // We can't send commands to compIdAll using this method. The reason being that we would get responses back possibly from multiple components
    // which this code can't handle.
    // We also can't send the majority of commands again if we are already waiting for a response from that same command. If we did that we would not be able to discern
    // which ack was associated with which command.
    const bool serializedCommand = _mustBeSerialized(command);
    SharedLinkInterfacePtr sharedLink = serializedCommand
                                            ? (targetLink ? targetLink
                                                          : _vehicle->vehicleLinkManager()->primaryLink().lock())
                                            : SharedLinkInterfacePtr{};
    const bool commandPending = serializedCommand && sharedLink
                                    ? _hasSetMessageIntervalEntry(targetCompId, sharedLink.get())
                                    : isPending(targetCompId, command);
    const bool waitingToSend = commandPending && serializedCommand;
    if ((targetCompId == MAV_COMP_ID_ALL) ||
        (commandPending && !_canBeDuplicated(command) && !_mustBeSerialized(command))) {
        bool    compIdAll       = targetCompId == MAV_COMP_ID_ALL;
        QString rawCommandName  = MissionCommandTree::instance()->rawName(command);

        qCDebug(MavCommandQueueLog) << QStringLiteral("sendWorker failing %1").arg(compIdAll ? "MAV_COMP_ID_ALL not supported" : "duplicate command") << rawCommandName << param1 << param2 << param3 << param4 << param5 << param6 << param7;

        MavCmdResultFailureCode_t failureCode = compIdAll ? MavCmdResultCommandResultOnly : MavCmdResultFailureDuplicateCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit commandResult(_vehicle->id(), targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (showError) {
            QGC::showAppMessage(tr("Unable to send command: %1.").arg(compIdAll ? tr("Internal error - MAV_COMP_ID_ALL not supported") : tr("Waiting on previous response to same command.")));
        }
        return;
    }

    if (!sharedLink) {
        sharedLink = targetLink ? targetLink : _vehicle->vehicleLinkManager()->primaryLink().lock();
    }
    if (!sharedLink) {
        qCDebug(MavCommandQueueLog) << "sendWorker: primary link gone!";

        const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureNoResponseToCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.command = command;
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit commandResult(_vehicle->id(), targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }

        if (showError) {
            QGC::showAppMessage(tr("Unable to send command: Vehicle is not connected."));
        }
        return;
    }

    if (serializedCommand) {
        _observeSetMessageIntervalLink(sharedLink.get());
        if (!sharedLink->isConnected()) {
            const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureNoResponseToCommand;
            QPointer<MavCommandQueue> guard(this);
            if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
                mavlink_command_ack_t ack = {};
                ack.command = command;
                ack.result = MAV_RESULT_FAILED;
                (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
            } else {
                emit commandResult(_vehicle->id(), targetCompId, command, MAV_RESULT_FAILED, failureCode);
            }
            if (guard && showError) {
                QGC::showAppMessage(tr("Unable to send command: Vehicle is not connected."));
            }
            return;
        }
    }

    if (serializedCommand && _isSetMessageIntervalQuarantined(targetCompId, sharedLink.get())) {
        qCWarning(MavCommandQueueLog) << "Rejecting SET_MESSAGE_INTERVAL while awaiting a late ACK"
                                      << "component:link" << targetCompId << sharedLink->linkConfiguration()->name();

        const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureNoResponseToCommand;
        QPointer<MavCommandQueue> guard(this);
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.command = command;
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit commandResult(_vehicle->id(), targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (guard && showError) {
            QGC::showAppMessage(tr("Unable to send command: Waiting on previous response to same command."));
        }
        return;
    }

    MavCommandListEntry_t entry;
    entry.useCommandInt     = commandInt;
    entry.targetCompId      = targetCompId;
    entry.command           = command;
    entry.frame             = frame;
    entry.showError         = showError;
    entry.ackHandlerInfo    = {};
    if (ackHandlerInfo) {
        entry.ackHandlerInfo = *ackHandlerInfo;
    }
    entry.rgParam1          = param1;
    entry.rgParam2          = param2;
    entry.rgParam3          = param3;
    entry.rgParam4          = param4;
    entry.rgParam5          = param5;
    entry.rgParam6          = param6;
    entry.rgParam7          = param7;
    entry.maxTries          = _shouldRetry(command) ? kMaxRetryCount : 1;
    entry.ackTimeoutMSecs   = sharedLink->linkConfiguration()->isHighLatency()
                                  ? _ackTimeoutMSecsHighLatency
                                  : _ackTimeoutMSecs(command, sharedLink.get());
    entry.targetLink        = serializedCommand ? sharedLink : targetLink;
    entry.targetLinkIdentity = entry.targetLink.lock().get();
    entry.targetLinkPinned  = static_cast<bool>(entry.targetLink.lock());
    entry.waitingToSend     = waitingToSend;
    if (!waitingToSend) {
        entry.elapsedTimer.start();
    }

    qCDebug(MavCommandQueueLog) << (waitingToSend ? "Queueing" : "Sending") << _formatCommand(command, param1)
                                << "param1-7:" << command << param1 << param2 << param3 << param4 << param5
                                << param6 << param7;

    _list.append(entry);
    if (!waitingToSend) {
        _sendFromList(_list.count() - 1);
    }
}

void MavCommandQueue::_sendFromList(int index)
{
    MavCommandListEntry_t commandEntry = _list[index];

    QString rawCommandName = MissionCommandTree::instance()->rawName(commandEntry.command);
    QString friendlyName = MissionCommandTree::instance()->friendlyName(commandEntry.command);

    if (++_list[index].tryCount > commandEntry.maxTries) {
        QString logMsg = QStringLiteral("Giving up sending command after max retries: %1").arg(rawCommandName);

        // For REQUEST_MESSAGE commands, also log which message was being requested
        if (commandEntry.command == MAV_CMD_REQUEST_MESSAGE) {
            int requestedMsgId = static_cast<int>(commandEntry.rgParam1);
            const mavlink_message_info_t *info = mavlink_get_message_info_by_id(requestedMsgId);
            logMsg += QStringLiteral(" requesting: %1").arg(info ? info->name : QString::number(requestedMsgId));
        }

        qCWarning(MavCommandQueueLog) << logMsg;

        if (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
            const SharedLinkInterfacePtr timedOutLink = commandEntry.targetLink.lock();
            _addSetMessageIntervalTombstone(commandEntry.targetCompId, timedOutLink.get());

            QList<MavCommandListEntry_t> failedEntries;
            for (int i = _list.count() - 1; i >= 0; --i) {
                const MavCommandListEntry_t& entry = _list[i];
                const bool sameKey = entry.targetCompId == commandEntry.targetCompId &&
                                     entry.command == MAV_CMD_SET_MESSAGE_INTERVAL &&
                                     entry.targetLinkIdentity == commandEntry.targetLinkIdentity;
                if (i == index || sameKey) {
                    failedEntries.prepend(_list.takeAt(i));
                }
            }

            for (const MavCommandListEntry_t& failedEntry : std::as_const(failedEntries)) {
                QPointer<MavCommandQueue> guard(this);
                if (failedEntry.ackHandlerInfo.resultHandler) {
                    mavlink_command_ack_t ack = {};
                    ack.command = failedEntry.command;
                    ack.result = MAV_RESULT_FAILED;
                    (*failedEntry.ackHandlerInfo.resultHandler)(failedEntry.ackHandlerInfo.resultHandlerData,
                                                                failedEntry.targetCompId, ack,
                                                                MavCmdResultFailureNoResponseToCommand);
                } else {
                    emit commandResult(_vehicle->id(), failedEntry.targetCompId, failedEntry.command,
                                       MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
                }
                if (!guard) {
                    return;
                }
                if (failedEntry.showError) {
                    QGC::showAppMessage(tr("Vehicle did not respond to command: %1").arg(friendlyName));
                }
            }
            return;
        }

        _list.removeAt(index);
        QPointer<MavCommandQueue> guard(this);
        if (commandEntry.ackHandlerInfo.resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData, commandEntry.targetCompId, ack, MavCmdResultFailureNoResponseToCommand);
        } else {
            emit commandResult(_vehicle->id(), commandEntry.targetCompId, commandEntry.command, MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        }
        if (!guard) {
            return;
        }
        if (commandEntry.showError) {
            QGC::showAppMessage(tr("Vehicle did not respond to command: %1").arg(friendlyName));
        }
        _sendNextWaitingCommand(commandEntry.targetCompId, commandEntry.command, nullptr);
        return;
    }

    if (commandEntry.tryCount > 1 && !_vehicle->px4Firmware() && commandEntry.command == MAV_CMD_START_RX_PAIR) {
        // The implementation of this command comes from the IO layer and is shared across stacks. So for other firmwares
        // we aren't really sure whether they are correct or not.
        return;
    }

    qCDebug(MavCommandQueueLog) << "Sending" << _formatCommand(commandEntry.command, commandEntry.rgParam1)
                                << "timeoutMs:tryCount:param1-7" << commandEntry.ackTimeoutMSecs
                                << commandEntry.tryCount << commandEntry.rgParam1 << commandEntry.rgParam2
                                << commandEntry.rgParam3 << commandEntry.rgParam4 << commandEntry.rgParam5
                                << commandEntry.rgParam6 << commandEntry.rgParam7;

    SharedLinkInterfacePtr sharedLink = commandEntry.targetLinkPinned
                                            ? commandEntry.targetLink.lock()
                                            : _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(MavCommandQueueLog) << "_sendFromList: primary link gone!";
        return;
    }

    mavlink_message_t msg;

    if (commandEntry.useCommandInt) {
        mavlink_command_int_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _vehicle->id();
        cmd.target_component =  commandEntry.targetCompId;
        cmd.command =           commandEntry.command;
        cmd.frame =             commandEntry.frame;
        cmd.param1 =            commandEntry.rgParam1;
        cmd.param2 =            commandEntry.rgParam2;
        cmd.param3 =            commandEntry.rgParam3;
        cmd.param4 =            commandEntry.rgParam4;
        cmd.x =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam5 : commandEntry.rgParam5 * 1e7;
        cmd.y =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam6 : commandEntry.rgParam6 * 1e7;
        cmd.z =                 commandEntry.rgParam7;
        mavlink_msg_command_int_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                            MAVLinkProtocol::getComponentId(),
                                            sharedLink->mavlinkChannel(),
                                            &msg,
                                            &cmd);
    } else {
        mavlink_command_long_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _vehicle->id();
        cmd.target_component =  commandEntry.targetCompId;
        cmd.command =           commandEntry.command;
        // MAVLink spec: confirmation increments on each resend.
        cmd.confirmation =      static_cast<uint8_t>(qMin(commandEntry.tryCount - 1, 255));
        cmd.param1 =            commandEntry.rgParam1;
        cmd.param2 =            commandEntry.rgParam2;
        cmd.param3 =            commandEntry.rgParam3;
        cmd.param4 =            commandEntry.rgParam4;
        cmd.param5 =            static_cast<float>(commandEntry.rgParam5);
        cmd.param6 =            static_cast<float>(commandEntry.rgParam6);
        cmd.param7 =            commandEntry.rgParam7;
        mavlink_msg_command_long_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                             MAVLinkProtocol::getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &msg,
                                             &cmd);
    }

    _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void MavCommandQueue::_sendNextWaitingCommand(int targetCompId, MAV_CMD command, LinkInterface* targetLink)
{
    for (int i = 0; i < _list.count(); ++i) {
        MavCommandListEntry_t& entry = _list[i];
        if (entry.targetCompId != targetCompId || entry.command != command || !entry.waitingToSend ||
            entry.targetLink.lock().get() != targetLink) {
            continue;
        }

        const SharedLinkInterfacePtr sharedLink = entry.targetLink.lock();
        if (sharedLink) {
            entry.ackTimeoutMSecs = sharedLink->linkConfiguration()->isHighLatency()
                                        ? _ackTimeoutMSecsHighLatency
                                        : _ackTimeoutMSecs(entry.command, sharedLink.get());
        }
        entry.waitingToSend = false;
        entry.elapsedTimer.start();
        _sendFromList(i);
        return;
    }
}

int MavCommandQueue::_findActiveEntryIndex(int targetCompId, MAV_CMD command, LinkInterface* incomingLink) const
{
    for (int i = 0; i < _list.count(); ++i) {
        const MavCommandListEntry_t& entry = _list[i];
        if (entry.targetCompId != targetCompId || entry.command != command || entry.waitingToSend) {
            continue;
        }
        if (entry.targetLinkPinned && entry.targetLink.lock().get() != incomingLink) {
            continue;
        }
        return i;
    }
    return -1;
}

void MavCommandQueue::_responseTimeoutCheck()
{
    if (_list.isEmpty()) {
        return;
    }

    // Walk the list backwards since _sendFromList can remove entries
    for (int i = _list.count() - 1; i >= 0; i--) {
        MavCommandListEntry_t& commandEntry = _list[i];
        if (commandEntry.waitingToSend) {
            continue;
        }
        if (commandEntry.elapsedTimer.elapsed() > commandEntry.ackTimeoutMSecs) {
            // Try sending command again
            _sendFromList(i);
        }
    }
}

void MavCommandQueue::showCommandAckError(const mavlink_command_ack_t& ack)
{
    QString rawName      = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(ack.command));
    QString friendlyName = MissionCommandTree::instance()->friendlyName(static_cast<MAV_CMD>(ack.command));
    QString commandStr   = friendlyName.isEmpty() ? rawName : QStringLiteral("%1 (%2)").arg(friendlyName, rawName);

    switch (ack.result) {
    case MAV_RESULT_TEMPORARILY_REJECTED:
        QGC::showAppMessage(tr("%1 command temporarily rejected").arg(commandStr));
        break;
    case MAV_RESULT_DENIED:
        QGC::showAppMessage(tr("%1 command denied").arg(commandStr));
        break;
    case MAV_RESULT_UNSUPPORTED:
        QGC::showAppMessage(tr("%1 command not supported").arg(commandStr));
        break;
    case MAV_RESULT_FAILED:
        QGC::showAppMessage(tr("%1 command failed").arg(commandStr));
        break;
    default:
        // Do nothing
        break;
    }
}

void MavCommandQueue::handleCommandAck(const mavlink_message_t& message, const mavlink_command_ack_t& ack,
                                       LinkInterface* incomingLink)
{
    int entryIndex = _findActiveEntryIndex(message.compid, static_cast<MAV_CMD>(ack.command), incomingLink);
    if (entryIndex == -1) {
        if (ack.command == MAV_CMD_SET_MESSAGE_INTERVAL && incomingLink &&
            _consumeSetMessageIntervalTombstone(message.compid, incomingLink)) {
            qCDebug(MavCommandQueueLog) << "Consumed late SET_MESSAGE_INTERVAL ACK"
                                        << "component:link" << message.compid
                                        << incomingLink->linkConfiguration()->name();
            return;
        }
        QString rawCommandName = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(ack.command));
        qCDebug(MavCommandQueueLog) << "handleCommandAck Ack not in list" << rawCommandName;
        return;
    }

    if (ack.result == MAV_RESULT_IN_PROGRESS) {
        MavCommandListEntry_t commandEntry;
        if (_vehicle->px4Firmware() && ack.command == MAV_CMD_DO_AUTOTUNE_ENABLE) {
            // Hack to support PX4 autotune which does not send final result ack and just sends in progress
            commandEntry = _list.takeAt(entryIndex);
        } else {
            // Command has not completed yet, don't remove
            MavCommandListEntry_t& commandEntryRef = _list[entryIndex];
            commandEntryRef.maxTries = 1;         // Vehicle responded to command so don't retry
            commandEntryRef.elapsedTimer.start(); // We've heard from vehicle, restart elapsed timer for no ack received timeout
            commandEntry = commandEntryRef;
        }

        if (commandEntry.ackHandlerInfo.progressHandler) {
            (*commandEntry.ackHandlerInfo.progressHandler)(commandEntry.ackHandlerInfo.progressHandlerData, message.compid, ack);
        }
        return;
    }

    MavCommandListEntry_t commandEntry = _list.takeAt(entryIndex);
    const SharedLinkInterfacePtr completedLink = commandEntry.targetLink.lock();
    QPointer<MavCommandQueue> guard(this);
    if (commandEntry.ackHandlerInfo.resultHandler) {
        (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData, message.compid, ack, MavCmdResultCommandResultOnly);
    } else {
        if (commandEntry.showError) {
            showCommandAckError(ack);
        }
        emit commandResult(_vehicle->id(), message.compid, ack.command, ack.result, MavCmdResultCommandResultOnly);
    }
    if (guard) {
        _sendNextWaitingCommand(commandEntry.targetCompId, commandEntry.command, completedLink.get());
    }
}

QString MavCommandQueue::failureCodeToString(MavCmdResultFailureCode_t failureCode)
{
    switch (failureCode) {
    case MavCmdResultCommandResultOnly:
        return QStringLiteral("Command Result Only");
    case MavCmdResultFailureNoResponseToCommand:
        return QStringLiteral("No Response To Command");
    case MavCmdResultFailureDuplicateCommand:
        return QStringLiteral("Duplicate Command");
    default:
        return QStringLiteral("Unknown (%1)").arg(failureCode);
    }
}
