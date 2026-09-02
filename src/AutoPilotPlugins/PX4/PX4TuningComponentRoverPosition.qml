import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    id: root

    property real _availableHeight: availableHeight
    property real _availableWidth:  availableWidth

    readonly property var _vehicle:     globals.activeVehicle
    readonly property var _roverTuning: _vehicle ? _vehicle.roverTuning : null
    readonly property int _driveType:   _roverTuning ? Number(_roverTuning.positionDriveType.value) : 0
    readonly property string _statusText: {
        if (!_vehicle || !_roverTuning) {
            return qsTr("Telemetry unavailable")
        }
        if (!_vehicle.roverTuningMavlink2Supported) {
            return qsTr("MAVLink 2 required")
        }
        if (_vehicle.roverTuningTelemetryError.length > 0) {
            return _vehicle.roverTuningTelemetryError
        }
        if (_driveType !== 0 && _driveType !== 1) {
            return qsTr("Unsupported rover drive type")
        }
        if (!_roverTuning.positionActive) {
            return qsTr("Path controller inactive")
        }
        if (!_roverTuning.positionAvailable) {
            return qsTr("Path telemetry unavailable")
        }
        return _roverTuning.positionPathAvailable ? "" : qsTr("Path diagnostics unavailable")
    }

    QGCPalette { id: qgcPal }

    ListModel {
        id: pathParameters

        ListElement {
            title:       qsTr("Lookahead gain (PP_LOOKAHD_GAIN)")
            description: qsTr("Pure Pursuit speed-to-lookahead gain.")
            param:       "PP_LOOKAHD_GAIN"
        }
        ListElement {
            title:       qsTr("Minimum lookahead (PP_LOOKAHD_MIN)")
            description: qsTr("Minimum Pure Pursuit lookahead distance.")
            param:       "PP_LOOKAHD_MIN"
        }
        ListElement {
            title:       qsTr("Maximum lookahead (PP_LOOKAHD_MAX)")
            description: qsTr("Maximum Pure Pursuit lookahead distance.")
            param:       "PP_LOOKAHD_MAX"
        }
        ListElement {
            title:       qsTr("Turn-to-drive transition (RD_TRANS_TRN_DRV)")
            description: qsTr("Threshold for entering path driving from a turn.")
            param:       "RD_TRANS_TRN_DRV"
        }
        ListElement {
            title:       qsTr("Drive-to-turn transition (RD_TRANS_DRV_TRN)")
            description: qsTr("Threshold for entering a turn from path driving.")
            param:       "RD_TRANS_DRV_TRN"
        }
        ListElement {
            title:       qsTr("Miss-speed gain (RD_MISS_SPD_GAIN)")
            description: qsTr("Path-speed reduction from waypoint miss distance.")
            param:       "RD_MISS_SPD_GAIN"
        }
    }

    QGCLabel {
        text:               root._statusText
        color:              qgcPal.warningText
        visible:            text.length > 0
        Layout.alignment:   Qt.AlignHCenter
    }

    PIDTuning {
        id:                     pidTuning
        availableWidth:         root._availableWidth
        availableHeight:        root._availableHeight - pidTuning.y
        title:                  qsTr("Path Tracking (Pure Pursuit)")
        unit:                   qsTr("m")
        tuningMode:             Vehicle.ModeRoverPosition
        chartDisplaySec:        50
        useFactMetadataRange:   true
        parametersEnabled:      root._driveType === 0 || root._driveType === 1
        useSourceTimestamp:     true
        sourceTimestamp:        root._roverTuning && root._roverTuning.positionAvailable ? Number(root._roverTuning.positionTimestamp.value) : NaN
        sourceResetCounter:     root._roverTuning ? root._roverTuning.positionResetCounter : 0
        axis:                   [northAxis, eastAxis, crosstrackAxis]

        property var northAxis: QtObject {
            property string name: qsTr("North")
            property var plot: [
                { name: qsTr("Response"), value: root._roverTuning ? root._roverTuning.positionNorth.value : NaN },
                { name: qsTr("Target"), value: root._roverTuning ? root._roverTuning.targetNorth.value : NaN }
            ]
            property var params: pathParameters
        }

        property var eastAxis: QtObject {
            property string name: qsTr("East")
            property var plot: [
                { name: qsTr("Response"), value: root._roverTuning ? root._roverTuning.positionEast.value : NaN },
                { name: qsTr("Target"), value: root._roverTuning ? root._roverTuning.targetEast.value : NaN }
            ]
            property var params: pathParameters
        }

        property var crosstrackAxis: QtObject {
            property string name: qsTr("Cross-track")
            property var plot: [
                { name: qsTr("Error"), value: root._roverTuning ? root._roverTuning.crosstrackError.value : NaN }
            ]
            property var params: pathParameters
        }
    }
}
