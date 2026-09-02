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
    readonly property int _driveType:   _roverTuning ? Number(_roverTuning.rateDriveType.value) : 0
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
        if (!_roverTuning.rateActive) {
            return qsTr("Rate controller inactive")
        }
        return _roverTuning.rateAvailable ? "" : qsTr("Telemetry unavailable")
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
        title:                  qsTr("Yaw Rate")
        unit:                   qsTr("deg/s")
        tuningMode:             Vehicle.ModeRoverRate
        useFactMetadataRange:   true
        parametersEnabled:      root._driveType === 0 || root._driveType === 1
        useSourceTimestamp:     true
        sourceTimestamp:        root._roverTuning && root._roverTuning.rateAvailable ? Number(root._roverTuning.rateTimestamp.value) : NaN
        sourceResetCounter:     root._roverTuning ? root._roverTuning.rateResetCounter : 0
        axis:                   [yawRateAxis]

        property var yawRateAxis: QtObject {
            property string name: qsTr("Yaw Rate")
            property var plot: [
                { name: qsTr("Response"), value: root._roverTuning ? root._roverTuning.rateYawResponse.value : NaN },
                { name: qsTr("Setpoint"), value: root._roverTuning ? root._roverTuning.rateYawSetpoint.value : NaN }
            ]
            property var params: ListModel {
                ListElement {
                    title:       qsTr("Proportional gain (RO_YAW_RATE_P)")
                    description: qsTr("Yaw-rate proportional control gain.")
                    param:       "RO_YAW_RATE_P"
                }
                ListElement {
                    title:       qsTr("Integral gain (RO_YAW_RATE_I)")
                    description: qsTr("Yaw-rate steady-state error correction.")
                    param:       "RO_YAW_RATE_I"
                }
                ListElement {
                    title:       qsTr("Control authority (RD_MAX_THR_YAW_R)")
                    description: qsTr("Maximum yaw-rate control authority.")
                    param:       "RD_MAX_THR_YAW_R"
                }
                ListElement {
                    title:       qsTr("Rate limit (RO_YAW_RATE_LIM)")
                    description: qsTr("Maximum commanded yaw rate.")
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
                ListElement {
                    title:       qsTr("Measurement threshold (RO_YAW_RATE_TH)")
                    description: qsTr("Filters only the measured yaw rate; the setpoint is unchanged.")
                    param:       "RO_YAW_RATE_TH"
                }
            }
        }
    }
}
