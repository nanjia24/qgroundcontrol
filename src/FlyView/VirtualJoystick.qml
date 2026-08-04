import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    // The following properties must be passed in from the Loader
    // property bool autoCenterThrottle - true: throttle will snap back to center when released
    // property bool leftHandedMode - true: virtual joystick layout will be reversed

    id: virtualJoysticks

    property var   _activeVehicle:            QGroundControl.multiVehicleManager.activeVehicle
    property bool  _initialConnectComplete:   _activeVehicle ? _activeVehicle.initialConnectComplete : false
    property real  leftYAxisValue:            autoCenterThrottle ? height / 2 : height
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
        if (!leftStick.yAxisReCenter) {
            leftStick.stickPositionY = leftStick.yAxisPositiveRangeOnly ? leftStick.height : leftStick.height / 2
        }
        leftStick.syncAxes()
        rightStick.syncAxes()
        leftYAxisValue = leftStick.yAxis
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
        running:    QGroundControl.settingsManager.appSettings.virtualJoystick.value && virtualJoysticks.visible
        repeat:     true
        onTriggered: {
            if (virtualJoysticks._activeVehicle &&
                virtualJoysticks._initialConnectComplete &&
                virtualJoysticks._manualControlAvailable &&
                !virtualJoysticks._restorePending &&
                !virtualJoysticks._activeJoystickSending) {
                leftHandedMode ? virtualJoysticks._activeVehicle.virtualTabletJoystickValue(leftStick.xAxis, leftStick.yAxis, rightStick.xAxis, rightStick.yAxis) : virtualJoysticks._activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
                leftYAxisValue = leftStick.yAxis // We keep Y axis value from the throttle stick for using it while there is a resize
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
    on_ActiveJoystickSendingChanged: {
        if (!virtualJoysticks._activeJoystickSending) {
            virtualJoysticks._restoreVirtualJoystickPositions()
        }
    }

    function calibrateJoysticks() {
        if( virtualJoysticks.visible ) {
        keepXAxisWhileChanged()
        leftYAxisValue = leftStick.yAxisReCentered() // Keep track of the correct leftYAxisValue while the width is adjusted at first start up
        }
    }

    function keepYAxisWhileChanged () {
        if( virtualJoysticks.visible ) {
            leftStick.resize( leftYAxisValue )
            rightStick.reCenter()
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
        yAxisPositiveRangeOnly: _activeVehicle && !_activeVehicle.manualControlRover && !leftHandedMode && !_centerThrottleDisplay
        yAxisReCenter:          autoCenterThrottle
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
        yAxisPositiveRangeOnly: _activeVehicle && !_activeVehicle.manualControlRover && leftHandedMode && !_centerThrottleDisplay
        yAxisReCenter:          true
        inputEnabled:           !virtualJoysticks._activeJoystickSending && virtualJoysticks._manualControlAvailable && !virtualJoysticks._restorePending
    }
}
