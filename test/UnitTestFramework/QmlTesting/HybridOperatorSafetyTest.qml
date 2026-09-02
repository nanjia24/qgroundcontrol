import QtQuick

import QGroundControl.FlyView

Item {
    id: root

    width:  640
    height: 320

    property bool complete: false
    property string failure
    property int stage: 0

    function check(condition, message) {
        if (!condition && root.failure.length === 0) {
            root.failure = message
        }
    }

    component MockVehicle: QtObject {
        property int vehicleId
        property bool initialConnectComplete: true
        property bool manualControlRover: false
        property bool quadRover: true
        property bool manualControlProfileKnown: true
        property bool armed: false
        property bool flying: false
        property bool landing: false
        property bool fixedWing: false
        property bool inFwdFlight: false
        property bool haveFWSpeedLimits: false
        property bool haveMRSpeedLimits: false
        property int checkListState: 0
        property int sensorsPresentBits: 0
        property string flightMode: ""
        property string pauseFlightMode: "Pause"
        property string rtlFlightMode: "RTL"
        property string smartRTLFlightMode: "Smart RTL"
        property string landFlightMode: "Land"
        property string missionFlightMode: "Mission"
        property var supports: ({
            throttleModeCenterZero: true,
            guidedMode: false,
            guidedTakeoffWithAltitude: false,
            guidedTakeoffWithoutAltitude: false,
            pauseVehicle: false,
            roiMode: false,
            orbitMode: false
        })
        property var healthAndArmingCheckReport: ({ supported: false, canArm: true, canTakeoff: true, canStartMission: true })
        property var rcRSSI: ({ rawValue: 100 })
        property var homePosition: ({ isValid: false, altitude: 0 })
        property var altitudeRelative: ({ value: 0, rawValue: 0 })
        property int tabletSendCount: 0
        property real lastTabletThrottle: -1
        property int setHomeCount: 0
        property int pauseCount: 0

        function virtualTabletJoystickValue(roll, pitch, yaw, throttle) {
            tabletSendCount++
            lastTabletThrottle = throttle
        }
        function doSetHome(coordinate) { setHomeCount++ }
        function pauseVehicle() { pauseCount++ }
    }

    MockVehicle { id: vehicleA; vehicleId: 1 }
    MockVehicle { id: vehicleB; vehicleId: 2 }
    MockVehicle { id: vehicleBReplacement; vehicleId: 2 }

    QtObject {
        id: selectedModel
        property var vehicles: []
        readonly property int count: vehicles.length
        function get(index) { return vehicles[index] }
    }

    QtObject {
        id: mainWindow
        signal armVehicleRequest()
        signal forceArmVehicleRequest()
        signal disarmVehicleRequest()
    }

    QtObject {
        id: missionController
        property bool containsItems: false
        property var visualItems: ({ count: 0 })
        property int currentMissionIndex: 0
        property int resumeMissionIndex: 0
        signal resumeMissionUploadFail()
    }

    QtObject {
        id: guidedValueSlider
        property bool visible: false
        function getOutputValue() { return 0 }
        function setupSlider(type, minimum, maximum, value, label) {}
    }

    QtObject {
        id: messageDisplay
        property real opacity: 1
    }

    PropertyAnimation {
        id: messageOpacityAnimation
        target: messageDisplay
        property: "opacity"
        from: 1
        to: 0
        duration: 500
    }

    Timer {
        id: messageFadeTimer
        interval: 4000
        onTriggered: messageOpacityAnimation.start()
    }

    QtObject {
        id: mapIndicator
        property int confirmedCount: 0
        property int cancelledCount: 0
        function actionConfirmed() { confirmedCount++ }
        function actionCancelled() { cancelledCount++ }
    }

    VirtualJoystick {
        id: virtualJoystick
        width:  400
        height: 200
        visible: false
        autoCenterThrottle: false
        leftHandedMode: true
        activeVehicle: vehicleA
        sendingEnabled: true
    }

    GuidedActionsController {
        id: guidedController
        activeVehicle: vehicleA
        selectedVehicles: selectedModel
        missionController: missionController
        guidedValueSlider: guidedValueSlider
        _guidedActionsEnabled: false
    }

    GuidedActionConfirm {
        id: guidedConfirm
        guidedController: guidedController
        guidedValueSlider: guidedValueSlider
        messageDisplay: messageDisplay
    }

    function verifyVirtualJoystickPolicy() {
        const bools = [false, true]
        for (const leftHanded of bools) {
            for (const autoCenter of bools) {
                for (const roverProfile of bools) {
                    virtualJoystick.leftHandedMode = leftHanded
                    virtualJoystick.autoCenterThrottle = autoCenter
                    vehicleA.manualControlRover = roverProfile
                    virtualJoystick._restoreVirtualJoystickPositions()
                    const expected = roverProfile ? 0 : (autoCenter ? 0.5 : 0)
                    root.check(Math.abs(virtualJoystick.throttleYAxis - expected) < 0.0001,
                               "actual throttle stick reset mismatch: left=" + leftHanded +
                               ", auto=" + autoCenter + ", rover=" + roverProfile)
                }
            }
        }

        virtualJoystick.leftHandedMode = true
        virtualJoystick.autoCenterThrottle = false
        vehicleA.manualControlRover = false
        vehicleA.tabletSendCount = 0
        virtualJoystick._scheduleVirtualJoystickRestore()
    }

    function verifyGuidedActions() {
        vehicleA.armed = false
        vehicleB.armed = false
        selectedModel.vehicles = [vehicleA, vehicleB]
        guidedController.confirmAction(guidedController.actionMVArm, undefined, mapIndicator)
        root.check(guidedConfirm.actionVehicles.length === 2, "multi-vehicle confirmation did not capture selection")

        selectedModel.vehicles = [vehicleB]
        guidedConfirm.accept()
        root.check(!vehicleA.armed && !vehicleB.armed, "changed selection executed a confirmed action")
        root.check(mapIndicator.cancelledCount === 1 && mapIndicator.confirmedCount === 0,
                   "selection mismatch did not cancel map indicator exactly once")

        mapIndicator.cancelledCount = 0
        selectedModel.vehicles = [vehicleA, vehicleB]
        guidedController.confirmAction(guidedController.actionMVArm, undefined, mapIndicator)
        root.check(guidedConfirm.actionVehicle === vehicleA && guidedConfirm.actionVehicles.length === 2,
                   "matching confirmation did not retain its targets")
        const matchingSelectionSucceeded = guidedConfirm.accept()
        root.check(matchingSelectionSucceeded,
                   "matching confirmed selection was rejected: " + guidedController._lastExecutionRejectionReason)
        root.check(vehicleA.armed && vehicleB.armed,
                   "confirmed snapshot was not executed: a=" + vehicleA.armed + ", b=" + vehicleB.armed)
        root.check(mapIndicator.confirmedCount === 1 && mapIndicator.cancelledCount === 0,
                   "successful snapshot did not confirm map indicator exactly once")

        vehicleA.armed = false
        vehicleB.armed = false
        vehicleBReplacement.armed = false
        mapIndicator.confirmedCount = 0
        selectedModel.vehicles = [vehicleA, vehicleB]
        guidedController.confirmAction(guidedController.actionMVArm, undefined, mapIndicator)
        selectedModel.vehicles = [vehicleA, vehicleBReplacement]
        const replacementSelectionSucceeded = guidedConfirm.accept()
        root.check(replacementSelectionSucceeded,
                   "same-ID replacement was rejected: " + guidedController._lastExecutionRejectionReason)
        root.check(vehicleA.armed && !vehicleB.armed && vehicleBReplacement.armed,
                   "same-ID replacement did not execute current live objects")
        root.check(mapIndicator.confirmedCount === 1 && mapIndicator.cancelledCount === 0,
                   "same-ID replacement did not confirm map indicator exactly once")

        vehicleA.armed = false
        root.check(guidedController.executeAction(guidedController.actionArm, undefined, 0, false),
                   "legacy single-vehicle call failed")
        root.check(vehicleA.armed, "legacy single-vehicle call targeted no vehicle")

        vehicleA.setHomeCount = 0
        root.check(guidedController.executeAction(guidedController.actionSetHome, ({ latitude: 1 })),
                   "map direct-call signature failed")
        root.check(vehicleA.setHomeCount === 1, "map direct call did not reach active vehicle")

        vehicleA.armed = false
        vehicleB.armed = false
        root.check(!guidedController.executeAction(guidedController.actionMVArm, undefined, 0, false, vehicleA),
                   "multi-vehicle call without selection snapshot did not fail closed")
        root.check(!vehicleA.armed && !vehicleB.armed, "five-argument multi-vehicle call executed")
    }

    Timer {
        id: testTimer
        interval: 120
        repeat: false
        onTriggered: {
            if (root.stage === 0) {
                root.check(vehicleA.tabletSendCount === 0, "hidden virtual joystick sent control data")
                virtualJoystick.visible = true
                virtualJoystick._scheduleVirtualJoystickRestore()
                root.stage = 1
                testTimer.restart()
                return
            }

            root.check(vehicleA.tabletSendCount > 0, "visible restored virtual joystick did not resume sending")
            root.check(Math.abs(vehicleA.lastTabletThrottle) < 0.0001,
                       "left-handed non-auto-centered Quad resumed with nonzero throttle")
            root.verifyGuidedActions()
            root.complete = true
        }
    }

    Component.onCompleted: {
        root.verifyVirtualJoystickPolicy()
        testTimer.start()
    }
}
