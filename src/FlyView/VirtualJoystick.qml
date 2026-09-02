import QtQuick

import QGroundControl
import QGroundControl.Controls
import "VirtualJoystickLogic.js" as VirtualJoystickLogic

Item {
    id: virtualJoysticks

    required property bool autoCenterThrottle ///< true: throttle snaps back to center on release
    required property bool leftHandedMode     ///< true: virtual joystick layout is reversed
    property var   activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property bool  sendingEnabled:            QGroundControl.settingsManager.appSettings.virtualJoystick.value
    property var   _activeVehicle:            activeVehicle
    property bool  _initialConnectComplete:   _activeVehicle ? _activeVehicle.initialConnectComplete : false
    readonly property bool _autoCenterThrottle: autoCenterThrottle
    readonly property bool _leftHandedMode:     leftHandedMode
    property real  _throttleYAxisValue:         0
    property var   calibration:               false
    property real  uiTotalWidth:           0
    property real  uiRealX:                 0
    property bool  _activeJoystickSending:  joystickManager.activeJoystick && joystickManager.activeJoystickEnabledForActiveVehicle
    property bool  _restorePending:          false
    readonly property bool _centerThrottleDisplay: virtualJoysticks._activeVehicle && virtualJoysticks._activeVehicle.supports.throttleModeCenterZero &&
                                                   (virtualJoysticks._activeVehicle.manualControlRover ||
                                                    (virtualJoysticks._activeJoystickSending && joystickManager.activeJoystick && joystickManager.activeJoystick.settings.throttleModeCenterZero.rawValue))
    readonly property bool _manualControlAvailable: !!virtualJoysticks._activeVehicle &&
                                                    (!virtualJoysticks._activeVehicle.quadRover || virtualJoysticks._activeVehicle.manualControlProfileKnown)
    readonly property real throttleYAxis: virtualJoysticks._leftHandedMode ? rightStick.yAxis : leftStick.yAxis

    function _clampAxis(axisValue) {
        return Math.max(-1, Math.min(axisValue, 1))
    }

    function _clampPositiveAxis(axisValue) {
        return Math.max(0, Math.min(axisValue, 1))
    }

    function _setStickAxes(stick, xValue, yValue, positiveRangeY) {
        if (!stick || stick.width <= 0 || stick.height <= 0) {
            return
        }

        const xAxis = _clampAxis(xValue)
        const yAxis = positiveRangeY ? _clampPositiveAxis(yValue) : _clampAxis(yValue)
        stick.stickPositionX = ((xAxis + 1) / 2) * stick.width
        stick.stickPositionY = positiveRangeY ? (1 - yAxis) * stick.height : (1 - ((yAxis + 1) / 2)) * stick.height
    }

    function _setActiveJoystickDisplayValues(roll, pitch, yaw, throttle) {
        const throttlePositiveRange = !_centerThrottleDisplay
        if (leftHandedMode) {
            _setStickAxes(leftStick, roll, pitch, false)
            _setStickAxes(rightStick, yaw, throttle, throttlePositiveRange)
            return
        }
        _setStickAxes(leftStick, yaw, throttle, throttlePositiveRange)
        _setStickAxes(rightStick, roll, pitch, false)
    }

    function _restoreVirtualJoystickPositions() {
        leftStick.reCenter()
        rightStick.reCenter()
        const throttleStick = virtualJoysticks._leftHandedMode ? rightStick : leftStick
        const resetThrottle = VirtualJoystickLogic.resetYAxis(throttleStick.yAxisPositiveRangeOnly, throttleStick.yAxisReCenter)
        virtualJoysticks._setStickAxes(throttleStick, 0, resetThrottle, throttleStick.yAxisPositiveRangeOnly)
        leftStick.syncAxes()
        rightStick.syncAxes()
        virtualJoysticks._throttleYAxisValue = throttleStick.yAxis
    }

    function _scheduleVirtualJoystickRestore() {
        if (virtualJoysticks._restorePending) {
            return
        }
        virtualJoysticks._restorePending = true
        Qt.callLater(function() {
            virtualJoysticks._restoreVirtualJoystickPositions()
            virtualJoysticks._restorePending = false
        })
    }

    Timer {
        interval:   40  // 25Hz, same as real joystick rate
        running:    virtualJoysticks.sendingEnabled && virtualJoysticks.visible
        repeat:     true
        onTriggered: {
            if (virtualJoysticks._activeVehicle &&
                virtualJoysticks._initialConnectComplete &&
                virtualJoysticks._manualControlAvailable &&
                !virtualJoysticks._restorePending &&
                !virtualJoysticks._activeJoystickSending) {
                virtualJoysticks._leftHandedMode ? virtualJoysticks._activeVehicle.virtualTabletJoystickValue(leftStick.xAxis, leftStick.yAxis, rightStick.xAxis, rightStick.yAxis) : virtualJoysticks._activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
                virtualJoysticks._throttleYAxisValue = virtualJoysticks._leftHandedMode ? rightStick.yAxis : leftStick.yAxis
            }
        }
    }

    Connections {
        target: joystickManager.activeJoystick
        enabled: virtualJoysticks._activeJoystickSending

        function onAxisValues(roll, pitch, yaw, throttle) {
            virtualJoysticks._setActiveJoystickDisplayValues(roll, pitch, yaw, throttle)
        }
    }

    onHeightChanged:        { keepYAxisWhileChanged() }
    onWidthChanged:         { keepXAxisWhileChanged() }
    onVisibleChanged:       { if (virtualJoysticks.visible) virtualJoysticks._scheduleVirtualJoystickRestore() }
    onCalibrationChanged:   { calibration ? calibrateJoysticks() : undefined }
    on_ActiveVehicleChanged:          { virtualJoysticks._scheduleVirtualJoystickRestore() }
    on_ManualControlAvailableChanged: { virtualJoysticks._scheduleVirtualJoystickRestore() }
    on_CenterThrottleDisplayChanged:  { virtualJoysticks._scheduleVirtualJoystickRestore() }
    on_AutoCenterThrottleChanged:     { virtualJoysticks._scheduleVirtualJoystickRestore() }
    on_LeftHandedModeChanged:         { virtualJoysticks._scheduleVirtualJoystickRestore() }
    on_ActiveJoystickSendingChanged: {
        if (!virtualJoysticks._activeJoystickSending) {
            virtualJoysticks._restoreVirtualJoystickPositions()
        }
    }

    function calibrateJoysticks() {
        if( virtualJoysticks.visible ) {
            virtualJoysticks._restoreVirtualJoystickPositions()
        }
    }

    function keepYAxisWhileChanged () {
        if( virtualJoysticks.visible ) {
            const throttleStick = virtualJoysticks._leftHandedMode ? rightStick : leftStick
            const attitudeStick = virtualJoysticks._leftHandedMode ? leftStick : rightStick
            throttleStick.resize(virtualJoysticks._throttleYAxisValue)
            attitudeStick.reCenter()
        }
    }

    function keepXAxisWhileChanged () {
        if( virtualJoysticks.visible ) {
            leftStick.reCenter()
            rightStick.reCenter()
        }
    }

    JoystickThumbPad {
        id:                     leftStick
        anchors.leftMargin:     xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 parent.height
        yAxisPositiveRangeOnly: VirtualJoystickLogic.yAxisPositiveRangeOnly(true, virtualJoysticks._leftHandedMode, virtualJoysticks._centerThrottleDisplay)
        yAxisReCenter:          VirtualJoystickLogic.yAxisReCenter(true, virtualJoysticks._leftHandedMode, virtualJoysticks._autoCenterThrottle)
        inputEnabled:           !virtualJoysticks._activeJoystickSending && virtualJoysticks._manualControlAvailable && !virtualJoysticks._restorePending
    }

    JoystickThumbPad {
        id:                     rightStick
        anchors.rightMargin:    -xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.right:          parent.right
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 parent.height
        yAxisPositiveRangeOnly: VirtualJoystickLogic.yAxisPositiveRangeOnly(false, virtualJoysticks._leftHandedMode, virtualJoysticks._centerThrottleDisplay)
        yAxisReCenter:          VirtualJoystickLogic.yAxisReCenter(false, virtualJoysticks._leftHandedMode, virtualJoysticks._autoCenterThrottle)
        inputEnabled:           !virtualJoysticks._activeJoystickSending && virtualJoysticks._manualControlAvailable && !virtualJoysticks._restorePending
    }

    Component.onCompleted: virtualJoysticks._scheduleVirtualJoystickRestore()
}
