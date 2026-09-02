import QtQuick
import QtTest

import "../../../../src/FlyView/VirtualJoystickLogic.js" as VirtualJoystickLogic

TestCase {
    name: "VirtualJoystickLogicTest"

    function test_shapePolicy_data() {
        const rows = []
        const bools = [false, true]

        for (const leftHandedMode of bools) {
            for (const autoCenterThrottle of bools) {
                for (const roverProfile of bools) {
                    rows.push({
                        tag: "leftHanded=" + leftHandedMode + ",autoCenter=" + autoCenterThrottle + ",rover=" + roverProfile,
                        leftHandedMode: leftHandedMode,
                        autoCenterThrottle: autoCenterThrottle,
                        roverProfile: roverProfile
                    })
                }
            }
        }

        return rows
    }

    function test_shapePolicy(data) {
        const throttleOnLeft = !data.leftHandedMode
        const leftPositiveRange = VirtualJoystickLogic.yAxisPositiveRangeOnly(true, data.leftHandedMode, data.roverProfile)
        const rightPositiveRange = VirtualJoystickLogic.yAxisPositiveRangeOnly(false, data.leftHandedMode, data.roverProfile)
        const leftReCenters = VirtualJoystickLogic.yAxisReCenter(true, data.leftHandedMode, data.autoCenterThrottle)
        const rightReCenters = VirtualJoystickLogic.yAxisReCenter(false, data.leftHandedMode, data.autoCenterThrottle)

        compare(VirtualJoystickLogic.isThrottleStick(true, data.leftHandedMode), throttleOnLeft)
        compare(VirtualJoystickLogic.isThrottleStick(false, data.leftHandedMode), !throttleOnLeft)
        compare(leftPositiveRange, throttleOnLeft && !data.roverProfile)
        compare(rightPositiveRange, !throttleOnLeft && !data.roverProfile)
        compare(leftReCenters, throttleOnLeft ? data.autoCenterThrottle : true)
        compare(rightReCenters, throttleOnLeft ? true : data.autoCenterThrottle)

        const throttlePositiveRange = throttleOnLeft ? leftPositiveRange : rightPositiveRange
        const throttleReCenters = throttleOnLeft ? leftReCenters : rightReCenters
        const expectedThrottleReset = data.roverProfile ? 0 : (data.autoCenterThrottle ? 0.5 : 0)
        compare(VirtualJoystickLogic.resetYAxis(throttlePositiveRange, throttleReCenters), expectedThrottleReset)
    }
}
