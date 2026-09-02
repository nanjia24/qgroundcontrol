import QtQuick
import QtTest

import "../../../../src/QmlControls/PIDTuningMath.js" as PIDTuningMath

TestCase {
    name: "PIDTuningMathTest"

    function test_largeMetadataRangeIsBounded() {
        var step = PIDTuningMath.metadataMajorTickStep(0, 10000, 0.01)

        compare(step, 200)
        verify(Math.ceil(10000 / step) + 1 <= PIDTuningMath.maximumMajorTicks + 1)
    }

    function test_incrementWinsForSmallRange() {
        compare(PIDTuningMath.metadataMajorTickStep(0, 1, 0.1), 0.1)
    }

    function test_invalidMetadataFallsBackSafely() {
        compare(PIDTuningMath.metadataMajorTickStep(1, 1, 0.5), 0.5)
        compare(PIDTuningMath.metadataMajorTickStep(NaN, NaN, 0), 0.01)
    }

    function test_displayStepUsesReadableValue() {
        var step = PIDTuningMath.metadataMajorTickStep(0, 10001, 0.01)

        compare(step, 250)
        verify(Math.ceil(10001 / step) + 1 <= PIDTuningMath.maximumMajorTicks + 1)
    }

    function test_axisRangeNeverDegenerates() {
        var existingRange = PIDTuningMath.nonDegenerateAxisRange(-3, 7, 5)
        compare(existingRange.minimum, -3)
        compare(existingRange.maximum, 7)

        var zeroRange = PIDTuningMath.nonDegenerateAxisRange(0, 0, 5)
        compare(zeroRange.minimum, -5)
        compare(zeroRange.maximum, 5)

        var invalidRange = PIDTuningMath.nonDegenerateAxisRange(NaN, NaN, 0)
        compare(invalidRange.minimum, -1)
        compare(invalidRange.maximum, 1)
    }

    function test_expiredPointCountFindsOnlyLeadingHistory() {
        var sampleTimes = [-181, -180.1, -180, -10, 0]
        var series = {
            count: sampleTimes.length,
            at: function(index) { return { x: sampleTimes[index] } }
        }

        compare(PIDTuningMath.expiredPointCount(series, -180), 2)
        compare(PIDTuningMath.expiredPointCount(series, -200), 0)
        compare(PIDTuningMath.expiredPointCount(series, 1), sampleTimes.length)
        compare(PIDTuningMath.expiredPointCount(series, NaN), 0)
    }

    function test_expiredPointCountStopsAtInvalidPoint() {
        var sampleTimes = [-2, NaN, -1]
        var series = {
            count: sampleTimes.length,
            at: function(index) { return { x: sampleTimes[index] } }
        }

        compare(PIDTuningMath.expiredPointCount(series, 0), 1)
    }

    function test_sourceSampleQueueKeepsEverySnapshot() {
        var queue = []
        var firstValues = [1, 2]

        PIDTuningMath.enqueueSourceSample(queue, 1000000, firstValues)
        firstValues[0] = 99
        PIDTuningMath.enqueueSourceSample(queue, 1020000, [3, 4])

        compare(queue.length, 2)
        compare(queue[0].timestamp, 1000000)
        compare(queue[0].values[0], 1)
        compare(queue[1].timestamp, 1020000)
        compare(queue[1].values[0], 3)
    }
}
