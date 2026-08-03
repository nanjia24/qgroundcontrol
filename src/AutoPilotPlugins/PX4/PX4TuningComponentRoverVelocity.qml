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
    readonly property int _driveType:   _roverTuning ? Number(_roverTuning.velocityDriveType.value) : 0
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
        if (!_roverTuning.velocityActive) {
            return qsTr("Velocity controller inactive")
        }
        return _roverTuning.velocityAvailable ? "" : qsTr("Telemetry unavailable")
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
        title:                  qsTr("Body-X Velocity")
        unit:                   qsTr("m/s")
        tuningMode:             Vehicle.ModeRoverVelocity
        useFactMetadataRange:   true
        parametersEnabled:      root._driveType === 0 || root._driveType === 1
        useSourceTimestamp:     true
        sourceTimestamp:        root._roverTuning && root._roverTuning.velocityAvailable ? Number(root._roverTuning.velocityTimestamp.value) : NaN
        sourceResetCounter:     root._roverTuning ? root._roverTuning.velocityResetCounter : 0
        axis:                   [bodyXAxis]

        property var bodyXAxis: QtObject {
            property string name: qsTr("Body-X Speed")
            property var plot: [
                { name: qsTr("Response"), value: root._roverTuning ? root._roverTuning.velocityBodyXResponse.value : NaN },
                { name: qsTr("Setpoint"), value: root._roverTuning ? root._roverTuning.velocityBodyXSetpoint.value : NaN }
            ]
            property var params: ListModel {
                ListElement {
                    title:       qsTr("Proportional gain (RO_SPEED_P)")
                    description: qsTr("Body-X speed proportional control gain.")
                    param:       "RO_SPEED_P"
                }
                ListElement {
                    title:       qsTr("Integral gain (RO_SPEED_I)")
                    description: qsTr("Body-X speed steady-state error correction.")
                    param:       "RO_SPEED_I"
                }
                ListElement {
                    title:       qsTr("Throttle authority (RO_MAX_THR_SPEED)")
                    description: qsTr("Maximum speed-controller throttle authority.")
                    param:       "RO_MAX_THR_SPEED"
                }
                ListElement {
                    title:       qsTr("Speed limit (RO_SPEED_LIM)")
                    description: qsTr("Maximum commanded body-X speed.")
                    param:       "RO_SPEED_LIM"
                }
                ListElement {
                    title:       qsTr("Acceleration limit (RO_ACCEL_LIM)")
                    description: qsTr("Body-X acceleration limit.")
                    param:       "RO_ACCEL_LIM"
                }
                ListElement {
                    title:       qsTr("Deceleration limit (RO_DECEL_LIM)")
                    description: qsTr("Body-X deceleration limit.")
                    param:       "RO_DECEL_LIM"
                }
                ListElement {
                    title:       qsTr("Jerk limit (RO_JERK_LIM)")
                    description: qsTr("Body-X jerk limit.")
                    param:       "RO_JERK_LIM"
                }
                ListElement {
                    title:       qsTr("Measurement threshold (RO_SPEED_TH)")
                    description: qsTr("Filters only the measured speed; the setpoint is unchanged.")
                    param:       "RO_SPEED_TH"
                }
            }
        }
    }
}
