.pragma library

function isThrottleStick(isLeftStick, leftHandedMode) {
    return isLeftStick !== leftHandedMode
}

function yAxisPositiveRangeOnly(isLeftStick, leftHandedMode, centerThrottleDisplay) {
    return isThrottleStick(isLeftStick, leftHandedMode) && !centerThrottleDisplay
}

function yAxisReCenter(isLeftStick, leftHandedMode, autoCenterThrottle) {
    return !isThrottleStick(isLeftStick, leftHandedMode) || autoCenterThrottle
}

function resetYAxis(positiveRangeOnly, reCenter) {
    return positiveRangeOnly && reCenter ? 0.5 : 0
}
