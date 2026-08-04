import QtQuick

import QGroundControl
import QGroundControl.Controls

PreFlightCheckButton {
    id: _root

    name:           qsTr("Hybrid system")
    manualText:     qsTr("Transform path clear and displayed shape matches the vehicle?")
    telemetryFailure: _root._failureText.length > 0
    telemetryTextFailure: _root._failureText

    property var _vehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _state:      _root._vehicle ? _root._vehicle.hybridVehicleState : null
    property var _controller: _root._vehicle ? _root._vehicle.hybridTransitionController : null

    readonly property string _failureText: {
        const state = _root._state
        const controller = _root._controller

        if (!_root._vehicle || !state || !controller) {
            return qsTr("Failure. Hybrid status is unavailable.")
        }
        if (state.resetCandidateActive) {
            return qsTr("Failure. Waiting for vehicle restart synchronization.")
        }
        if (state.stale) {
            return qsTr("Failure. Hybrid status is stale.")
        }
        if (!state.hasValidStatus) {
            return qsTr("Failure. Waiting for valid hybrid status.")
        }
        if (state.currentState === state.TransitionFault || state.faultReason !== 0) {
            return qsTr("Failure. %1.").arg(state.faultReasonText)
        }
        if (controller.transactionState !== controller.Idle) {
            return qsTr("Failure. %1.").arg(controller.requestUnavailableReason)
        }
        if (!state.actuatorOnline) {
            return qsTr("Failure. Transform actuator is offline.")
        }
        if (!state.actuatorHealthy) {
            return qsTr("Failure. Transform actuator is unhealthy.")
        }
        if (!state.actuatorConfigVerified) {
            return qsTr("Failure. Transform actuator configuration is not verified.")
        }
        if (state.sensorsEnabled && (!state.positionValid || !state.positionConfirmed)) {
            return qsTr("Failure. Transform position is not confirmed.")
        }
        if (state.currentState !== state.Quad && state.currentState !== state.Rover) {
            return qsTr("Failure. Hybrid shape is not stable.")
        }
        if (!state.landDetectionSampleFresh || !state.landed) {
            return qsTr("Failure. Vehicle landed state is not confirmed.")
        }
        return ""
    }
}
