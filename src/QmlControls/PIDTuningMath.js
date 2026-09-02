.pragma library

var maximumMajorTicks = 50

function _niceStepCeiling(value) {
    var exponent = Math.floor(Math.log(value) / Math.LN10)
    var magnitude = Math.pow(10, exponent)
    var normalized = value / magnitude
    var epsilon = 1e-9
    var multiplier = normalized <= 1 + epsilon ? 1
                   : normalized <= 2 + epsilon ? 2
                   : normalized <= 2.5 + epsilon ? 2.5
                   : normalized <= 5 + epsilon ? 5
                   : 10

    return multiplier * magnitude
}

function metadataMajorTickStep(minimum, maximum, increment) {
    var minimumValue = Number(minimum)
    var maximumValue = Number(maximum)
    var incrementValue = Number(increment)
    var validIncrement = Number.isFinite(incrementValue) && incrementValue > 0 ? incrementValue : 0

    if (!Number.isFinite(minimumValue) || !Number.isFinite(maximumValue) || maximumValue <= minimumValue) {
        return validIncrement > 0 ? validIncrement : 0.01
    }

    var minimumDisplayStep = (maximumValue - minimumValue) / maximumMajorTicks
    return validIncrement >= minimumDisplayStep ? validIncrement : _niceStepCeiling(minimumDisplayStep)
}

function nonDegenerateAxisRange(minimum, maximum, padding) {
    var minimumValue = Number(minimum)
    var maximumValue = Number(maximum)
    if (Number.isFinite(minimumValue) && Number.isFinite(maximumValue) && maximumValue > minimumValue) {
        return { minimum: minimumValue, maximum: maximumValue }
    }

    var center = Number.isFinite(minimumValue) ? minimumValue
               : Number.isFinite(maximumValue) ? maximumValue
               : 0
    var rangePadding = Number(padding)
    if (!Number.isFinite(rangePadding) || rangePadding <= 0) {
        rangePadding = 1
    }

    return { minimum: center - rangePadding, maximum: center + rangePadding }
}

function expiredPointCount(series, minimumTime) {
    var threshold = Number(minimumTime)
    if (!series || !Number.isFinite(threshold)) {
        return 0
    }

    var expiredCount = 0
    while (expiredCount < series.count) {
        var pointTime = Number(series.at(expiredCount).x)
        if (!Number.isFinite(pointTime) || pointTime >= threshold) {
            break
        }
        ++expiredCount
    }

    return expiredCount
}

function enqueueSourceSample(queue, timestamp, values) {
    queue.push({ timestamp: Number(timestamp), values: values.slice(0) })
}
