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
    readonly property int _driveType:   _roverTuning ? Number(_roverTuning.attitudeDriveType.value) : 0
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
        if (!_roverTuning.attitudeActive) {
            return qsTr("Attitude controller inactive")
        }
        return _roverTuning.attitudeAvailable ? "" : qsTr("Telemetry unavailable")
    }

    QGCPalette { id: qgcPal }

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
        title:                  qsTr("Yaw Attitude")
        unit:                   qsTr("deg")
        tuningMode:             Vehicle.ModeRoverAttitude
        useFactMetadataRange:   true
        parametersEnabled:      root._driveType === 0 || root._driveType === 1
        useSourceTimestamp:     true
        sourceTimestamp:        root._roverTuning && root._roverTuning.attitudeAvailable ? Number(root._roverTuning.attitudeTimestamp.value) : NaN
        sourceResetCounter:     root._roverTuning ? root._roverTuning.attitudeResetCounter : 0
        axis:                   [yawAxis]

        property var yawAxis: QtObject {
            property string name: qsTr("Yaw")
            property var plot: [
                { name: qsTr("Response"), value: root._roverTuning ? root._roverTuning.attitudeYawResponse.value : NaN },
                { name: qsTr("Setpoint"), value: root._roverTuning ? root._roverTuning.attitudeYawSetpoint.value : NaN }
            ]
            property var params: ListModel {
                ListElement {
                    title:       qsTr("Proportional gain (RO_YAW_P)")
                    description: qsTr("Yaw attitude proportional control gain.")
                    param:       "RO_YAW_P"
                }
                ListElement {
                    title:       qsTr("Rate limit (RO_YAW_RATE_LIM)")
                    description: qsTr("Maximum yaw-rate command from the attitude controller.")
                    param:       "RO_YAW_RATE_LIM"
                }
                ListElement {
                    title:       qsTr("Acceleration limit (RO_YAW_ACCEL_LIM)")
                    description: qsTr("Yaw-rate acceleration limit.")
                    param:       "RO_YAW_ACCEL_LIM"
                }
                ListElement {
                    title:       qsTr("Deceleration limit (RO_YAW_DECEL_LIM)")
                    description: qsTr("Yaw-rate deceleration limit.")
                    param:       "RO_YAW_DECEL_LIM"
                }
            }
        }
    }
}
