import QtQuick
import QtTest

import "../../../../src/FlyView/GuidedActionSelection.js" as GuidedActionSelection

TestCase {
    name: "GuidedActionSelectionTest"

    function _model(vehicles) {
        return {
            count: vehicles.length,
            get: function(index) { return vehicles[index] }
        }
    }

    function test_selectionSnapshotDoesNotDrift() {
        const vehicle1 = { id: 1 }
        const vehicle2 = { id: 2 }
        const vehicle3 = { id: 3 }
        const selected = _model([vehicle1, vehicle2])
        const expected = GuidedActionSelection.snapshot(selected)

        verify(GuidedActionSelection.matches(selected, expected))
        verify(GuidedActionSelection.matches(_model([vehicle2, vehicle1]), expected))
        verify(!GuidedActionSelection.matches(_model([vehicle1, vehicle3]), expected))
        verify(!GuidedActionSelection.matches(_model([vehicle1]), expected))
        verify(!GuidedActionSelection.matches(_model([vehicle1, vehicle2, vehicle3]), expected))
        verify(GuidedActionSelection.matches(_model([{ id: 1 }, vehicle2]), expected))
        verify(!GuidedActionSelection.matches(_model([vehicle1, vehicle1]), expected))
    }

    function test_missingSelectionFailsClosed() {
        compare(GuidedActionSelection.snapshot(undefined).length, 0)
        verify(!GuidedActionSelection.matches(undefined, []))
        verify(!GuidedActionSelection.matches(_model([]), undefined))
    }
}
