import QtQuick
import QtQuick.Controls
import QtGraphs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import "PIDTuningMath.js" as PIDTuningMath

GridLayout {
    id:            root
    columns:       _stackPanels ? 1 : 2
    rowSpacing:    _margins
    columnSpacing: _margins
    width:         availableWidth
    height:        availableHeight

    QGCPalette { id: qgcPal }

    property real   availableHeight
    property real   availableWidth
    property var    axis
    property string unit
    property string title
    property var    tuningMode
    property double chartDisplaySec:    8 // number of seconds to display
    property bool   showAutoModeChange: false
    property bool   showAutoTuning:     false
    property bool   useAutoTuning:      false
    property bool   useSourceTimestamp: false
    property bool   useFactMetadataRange: false
    property bool   parametersEnabled: true
    property double sourceTimestamp:    NaN
    property var    sourceResetCounter:  0

    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
    property int    _currentAxis:       0
    property var    _xAxis:             xAxis
    property var    _yAxis:             yAxis
    property int    _msecs:             0
    property double _last_t:            0
    property double _sourceTimestampOrigin: NaN
    property double _lastSourceTimestamp:   NaN
    property var    _sourceSampleQueue:     []
    property bool   _plottingActive:        true
    property var    _configuredVehicle:     null
    property bool   _componentComplete:      false
    property var    _savedTuningParamValues:    [ ]

    readonly property int _tickSeparation:      5
    readonly property int _maxTickSections:     10
    readonly property bool _stackPanels:        availableWidth < ScreenTools.defaultFontPixelWidth * 82
    readonly property real _chartPanelHeight:   _stackPanels
                                                    ? Math.max(0, Math.min(availableHeight - ScreenTools.defaultFontPixelHeight * 6 - _margins,
                                                                           availableHeight * 0.6))
                                                    : availableHeight
    readonly property real _rightPanelHeight:   _stackPanels
                                                    ? Math.max(0, availableHeight - _chartPanelHeight - _margins)
                                                    : availableHeight

    property string _chartTitle:        ""
    readonly property var _seriesColors: ["#21be2b", "#c62828", "#1565c0", "#f9a825", "#6a1b9a", "#00838f"]
    property var _legendModel:          []

    Component {
        id: lineSeriesComponent
        LineSeries { }
    }

    function adjustYAxisMin(yAxis, newValue) {
        var newMin = Math.min(yAxis.min, newValue)
        if (newMin % 5 != 0) {
            newMin -= 5
            newMin = Math.floor(newMin / _tickSeparation) * _tickSeparation
        }
        yAxis.min = newMin
    }

    function adjustYAxisMax(yAxis, newValue) {
        var newMax = Math.max(yAxis.max, newValue)
        if (newMax % 5 != 0) {
            newMax += 5
            newMax = Math.floor(newMax / _tickSeparation) * _tickSeparation
        }
        yAxis.max = newMax
    }

    function resetGraphs() {
        for (var i = 0; i < chart.seriesList.length; ++i) {
            chart.seriesList[i].clear()
        }
        _xAxis.min = -chartDisplaySec
        _xAxis.max = 0
        _yAxis.min = 0
        _yAxis.max = _tickSeparation
        _msecs = 0
        _last_t = 0
        _sourceTimestampOrigin = NaN
        _lastSourceTimestamp = NaN
        _sourceSampleQueue = []
    }

    function setPlottingActive(active) {
        if (_plottingActive === active) {
            return
        }

        _plottingActive = active
        _last_t = 0
        if (!active) {
            _sourceSampleQueue = []
        }
    }

    function metadataTickStep(fact) {
        return PIDTuningMath.metadataMajorTickStep(fact ? fact.min : NaN,
                                                   fact ? fact.max : NaN,
                                                   fact ? fact.increment : NaN)
    }

    function appendSample(sampleTimeSec, legacyFirstPoint, sourceValues) {
        var sourceTimestampSample = sourceValues !== undefined
        var plot = axis[_currentAxis].plot
        var len = plot.length
        var hasFiniteSample = false
        for (var valueIndex = 0; valueIndex < len; ++valueIndex) {
            var sampleValue = sourceTimestampSample ? Number(sourceValues[valueIndex]) : Number(plot[valueIndex].value)
            if (Number.isFinite(sampleValue)) {
                hasFiniteSample = true
            }
        }

        // Inactive Rover frames deliberately contain no finite plot values. Do not
        // move the axes or dirty an already empty graph for every source frame.
        if (sourceTimestampSample && !hasFiniteSample) {
            return false
        }

        _xAxis.max = sampleTimeSec
        _xAxis.min = sampleTimeSec - chartDisplaySec

        var firstPoint = sourceTimestampSample ? true : legacyFirstPoint
        if (sourceTimestampSample) {
            for (var seriesIndex = 0; seriesIndex < chart.seriesList.length; ++seriesIndex) {
                if (chart.seriesList[seriesIndex].count > 0) {
                    firstPoint = false
                    break
                }
            }
        }

        for (var i = 0; i < len; ++i) {
            var value = sourceTimestampSample ? Number(sourceValues[i]) : Number(plot[i].value)
            if (Number.isFinite(value)) {
                chart.seriesList[i].append(sampleTimeSec, value)
                if (firstPoint) {
                    _yAxis.min = value
                    _yAxis.max = value
                    if (sourceTimestampSample) {
                        firstPoint = false
                    }
                } else {
                    adjustYAxisMin(_yAxis, value)
                    adjustYAxisMax(_yAxis, value)
                }

                var series = chart.seriesList[i]
                var minSec = sampleTimeSec - 3 * 60
                var expiredCount = PIDTuningMath.expiredPointCount(series, minSec)
                if (expiredCount > 0) {
                    series.removeMultiple(0, expiredCount)
                }
            }
        }

        if (_yAxis.max <= _yAxis.min) {
            var range = PIDTuningMath.nonDegenerateAxisRange(_yAxis.min, _yAxis.max, _tickSeparation)
            _yAxis.min = range.minimum
            _yAxis.max = range.maximum
        }

        return hasFiniteSample
    }

    function queueSourceSample() {
        var timestamp = Number(sourceTimestamp)
        if (!Number.isFinite(timestamp) || timestamp <= 0) {
            resetGraphs()
            return
        }
        if (timestamp === _lastSourceTimestamp) {
            return
        }

        if (Number.isFinite(_lastSourceTimestamp) && timestamp < _lastSourceTimestamp) {
            resetGraphs()
        }

        _lastSourceTimestamp = timestamp
        var values = []
        var plot = axis[_currentAxis].plot
        for (var i = 0; i < plot.length; ++i) {
            values.push(Number(plot[i].value))
        }
        PIDTuningMath.enqueueSourceSample(_sourceSampleQueue, timestamp, values)
    }

    function flushSourceSamples() {
        var samples = _sourceSampleQueue
        _sourceSampleQueue = []
        for (var i = 0; i < samples.length; ++i) {
            var sample = samples[i]
            var firstSourceSample = !Number.isFinite(_sourceTimestampOrigin)
            var sourceOrigin = firstSourceSample ? sample.timestamp : _sourceTimestampOrigin
            if (appendSample((sample.timestamp - sourceOrigin) / 1000000, undefined, sample.values) && firstSourceSample) {
                _sourceTimestampOrigin = sample.timestamp
            }
        }
    }

    function configureVehicle(vehicle, resetChart) {
        if (_configuredVehicle === vehicle) {
            return
        }

        var previousVehicle = _configuredVehicle
        _configuredVehicle = null
        if (previousVehicle) {
            previousVehicle.setPIDTuningTelemetryMode(Vehicle.ModeDisabled)
        }

        if (vehicle) {
            vehicle.setPIDTuningTelemetryMode(tuningMode)
            _configuredVehicle = vehicle
        }
        if (resetChart !== false) {
            resetGraphs()
        }
    }

    // Save the current set of tuning values so we can reset to them
    function saveTuningParamValues() {
        _savedTuningParamValues = [ ]
        for (var i=0; i<axis[_currentAxis].params.count; i++) {
            var currentTuneParam = controller.getParameterFact(-1, axis[_currentAxis].params.get(i).param)
            _savedTuningParamValues.push(currentTuneParam.valueString)
        }
        savedRepeater.model = _savedTuningParamValues
    }

    function resetToSavedTuningParamValues() {
        for (var i=0; i<axis[_currentAxis].params.count; i++) {
            var currentTuneParam = controller.getParameterFact(-1,
                axis[_currentAxis].params.get(i).param)
            currentTuneParam.value = _savedTuningParamValues[i]
        }
    }

    function axisIndexChanged() {
        while (chart.seriesList.length > 0) {
            var s = chart.seriesList[0]
            chart.removeSeries(s)
            s.destroy()
        }
        var legendItems = []
        axis[_currentAxis].plot.forEach(function(e, idx) {
            var color = _seriesColors[idx % _seriesColors.length]
            var series = lineSeriesComponent.createObject(chart, {name: e.name, color: color})
            chart.addSeries(series)
            legendItems.push({name: e.name, color: color})
        })
        _legendModel = legendItems
        var chartTitle = axis[_currentAxis].plotTitle
        if (chartTitle == null)
            chartTitle = axis[_currentAxis].name
        _chartTitle = chartTitle + " " + title
        saveTuningParamValues()
        resetGraphs()
    }

    Component.onCompleted: {
        axisIndexChanged()
        configureVehicle(globals.activeVehicle)
        _componentComplete = true
    }

    Component.onDestruction: configureVehicle(null, false)
    on_CurrentAxisChanged: axisIndexChanged()
    onSourceResetCounterChanged: {
        if (_componentComplete && useSourceTimestamp) {
            resetGraphs()
        }
    }
    onSourceTimestampChanged: {
        if (_componentComplete && useSourceTimestamp && _plottingActive) {
            queueSourceSample()
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager

        function onActiveVehicleChanged(activeVehicle) {
            configureVehicle(activeVehicle)
        }

        function onVehicleRemoved(vehicle) {
            if (vehicle === _configuredVehicle) {
                configureVehicle(null)
            }
        }
    }

    Timer {
        id:         dataTimer
        interval:   10
        running:    root._plottingActive && !useSourceTimestamp
        repeat:     true

        onTriggered: {
            appendSample(_msecs / 1000, _msecs === 0)
            var t = new Date().getTime() // in ms
            if (_last_t > 0)
                _msecs += t-_last_t
            _last_t = t
        }

        property int _maxPointCount:    10000 / interval
    }

    Timer {
        id:         sourceDataTimer
        interval:   20
        running:    root._plottingActive && useSourceTimestamp
        repeat:     true

        onTriggered: {
            if (_sourceSampleQueue.length === 0) {
                return
            }
            flushSourceSamples()
        }
    }

    Column {
        id:                 leftPanel
        Layout.row:         0
        Layout.column:      0
        Layout.fillWidth:   true
        Layout.preferredHeight: root._chartPanelHeight
        Layout.maximumHeight:   root._chartPanelHeight
        Layout.alignment:   Qt.AlignTop
        spacing:            ScreenTools.defaultFontPixelHeight / 4
        clip:               true // chart has redraw problems

        QGCLabel {
            id:                 chartTitleLabel
            text:               _chartTitle
            font.pointSize:     ScreenTools.defaultFontPointSize
            font.family:        ScreenTools.normalFontFamily
            anchors.horizontalCenter: parent.horizontalCenter
        }

        GraphsView {
            id:                     chart
            width:                  root._stackPanels
                                        ? Math.max(1, availableWidth - _margins)
                                        : Math.max(_minChartWidth, availableWidth - rightPanel.width - root.columnSpacing - _margins)
            height:                 root._stackPanels
                                        ? Math.max(1, root._chartPanelHeight - leftPanelBottomColumn.height - chartTitleLabel.height - legendRow.height - _margins * 4)
                                        : Math.max(1, availableHeight - leftPanelBottomColumn.height - chartTitleLabel.height - legendRow.height - _margins * 4)

            property real _minChartWidth:   ScreenTools.defaultFontPixelWidth * 40

            theme: GraphsTheme {
                colorScheme:            qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
                plotAreaBackgroundColor: qgcPal.window
                grid.mainColor:         Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.5)
                grid.subColor:          Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.3)
                grid.mainWidth:         1
                labelBackgroundVisible: false
                labelTextColor:         qgcPal.text
            }

            axisX: ValueAxis {
                id:                     xAxis
                min:                    -root.chartDisplaySec
                max:                    0
                labelFormat:            "%.1f"
                titleText:              ScreenTools.isShortScreen ? "" : qsTr("sec")
                titleFont.pointSize:    ScreenTools.defaultFontPointSize
                titleFont.family:       ScreenTools.normalFontFamily
            }

            axisY: ValueAxis {
                id:                     yAxis
                min:                    0
                max:                    root._tickSeparation
                titleText:              unit
                tickInterval:           _tickSeparation
                titleFont.pointSize:    ScreenTools.defaultFontPointSize
                titleFont.family:       ScreenTools.normalFontFamily
            }

            // enable mouse dragging
            MouseArea {
                property var _startPoint: undefined
                property double _scaling: 0
                anchors.fill: parent
                onPressed: (mouse) => {
                    _startPoint = Qt.point(mouse.x, mouse.y)
                    if (chart.seriesList.length > 0) {
                        var start = chart.seriesList[0].dataPointCoordinatesAt(_startPoint.x, _startPoint.y)
                        var next = chart.seriesList[0].dataPointCoordinatesAt(mouse.x+1, mouse.y+1)
                        _scaling = next.x - start.x
                    }
                }
                onWheel: (wheel) => {
                    if (wheel.angleDelta.y > 0)
                        chartDisplaySec /= 1.2
                    else
                        chartDisplaySec *= 1.2
                    _xAxis.min = _xAxis.max - chartDisplaySec
                }
                onPositionChanged: (mouse) => {
                    if(_startPoint != undefined) {
                        setPlottingActive(false)
                        var cp = Qt.point(mouse.x, mouse.y)
                        var dx = (cp.x - _startPoint.x) * _scaling
                        _startPoint = cp
                        _xAxis.max -= dx
                        _xAxis.min -= dx
                    }
                }

                onReleased: {
                    _startPoint = undefined
                }
            }
        }

        Row {
            id:         legendRow
            spacing:    ScreenTools.defaultFontPixelWidth
            anchors.horizontalCenter: parent.horizontalCenter

            Repeater {
                model: _legendModel
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth / 2
                    Rectangle {
                        width:  ScreenTools.defaultFontPixelHeight
                        height: ScreenTools.defaultFontPixelHeight / 3
                        color:  modelData.color
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    QGCLabel {
                        text:               modelData.name
                        font.pointSize:     ScreenTools.smallFontPointSize
                    }
                }
            }
        }

        Column {
            id:         leftPanelBottomColumn
            spacing:    ScreenTools.defaultFontPixelHeight / 4

            RowLayout {
                spacing: _margins

                QGCButton {
                    text:       qsTr("Clear")
                    onClicked:  resetGraphs()
                }

                QGCButton {
                    text:       _plottingActive ? qsTr("Stop") : qsTr("Start")
                    onClicked: {
                        setPlottingActive(!_plottingActive)
                        if (showAutoModeChange && autoModeChange.checked && _configuredVehicle) {
                            _configuredVehicle.flightMode = _plottingActive ? _configuredVehicle.stabilizedFlightMode : _configuredVehicle.pauseFlightMode
                        }
                    }
                }
                Connections {
                    target: _configuredVehicle
                    function onArmedChanged(armed) {
                        if (armed && !_plottingActive) { // start plotting on arming if not already running
                            setPlottingActive(true)
                        }
                    }
                }
            }

            QGCCheckBox {
                visible: showAutoModeChange
                id:     autoModeChange
                text:   qsTr("Automatic Flight Mode Switching")
                onClicked: {
                    if (checked)
                        setPlottingActive(false)
                }
            }

            Column {
                visible: autoModeChange.checked
                QGCLabel {
                    text:            qsTr("Switches to 'Stabilized' when you click Start.")
                    font.pointSize:     ScreenTools.smallFontPointSize
                }

                QGCLabel {
                    text:            qsTr("Switches to '%1' when you click Stop.").arg(_configuredVehicle ? _configuredVehicle.pauseFlightMode : "")
                    font.pointSize:     ScreenTools.smallFontPointSize
                }
            }
        }
    }

    QGCFlickable {
        id:                 rightPanel
        Layout.row:         root._stackPanels ? 1 : 0
        Layout.column:      root._stackPanels ? 0 : 1
        Layout.fillWidth:   root._stackPanels
        Layout.preferredWidth: root._stackPanels ? root.availableWidth : ScreenTools.defaultFontPixelWidth * 40
        Layout.preferredHeight: root._rightPanelHeight
        Layout.maximumHeight:   root._rightPanelHeight
        Layout.alignment:   Qt.AlignTop
        contentWidth:       width
        contentHeight:      rightPanelContent.implicitHeight
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        ColumnLayout {
            id:     rightPanelContent
            width:  rightPanel.width

            RowLayout {
                visible: showAutoTuning

            QGCRadioButton {
                id:         useAutoTuningRadio
                text:       qsTr("Use auto-tuning")
                checked:    useAutoTuning
                onClicked:  useAutoTuning = true
            }
            QGCRadioButton {
                id:         useManualTuningRadio
                text:       qsTr("Use manual tuning")
                checked:    !useAutoTuning
                onClicked:  useAutoTuning = false
            }
        }

            AutotuneUI {
                visible: showAutoTuning && useAutoTuningRadio.checked
            }

            ColumnLayout {
                visible: !showAutoTuning || useManualTuningRadio.checked

            Column {
                RowLayout {
                    spacing: _margins
                    visible: axis.length > 1

                    QGCLabel { text: qsTr("Select Tuning:") }

                    Repeater {
                        model: axis
                        QGCRadioButton {
                            text:           modelData.name
                            checked:        index == _currentAxis
                            onClicked: _currentAxis = index
                        }
                    }
                }
            }

            // Instantiate all sliders (instead of switching the model), so that
            // values are not changed unexpectedly if they do not match with a tick value.
            Repeater {
                model: axis

                Repeater {
                    id: paramRepeater
                    model: axis[index].params

                    property int axisIndex: index

                    SettingsGroupLayout {
                        id:                     tuningGroup
                        heading:                title
                        headingDescription:     description
                        visible:                _currentAxis === paramRepeater.axisIndex
                        enabled:                root.parametersEnabled
                        Layout.fillWidth:       true

                        FactSlider {
                            fact:                   controller.getParameterFact(-1, param)
                            from:                   root.useFactMetadataRange ? fact.min : min
                            to:                     root.useFactMetadataRange ? fact.max : max
                            majorTickStepSize:      root.useFactMetadataRange ? root.metadataTickStep(fact) : step
                            Layout.fillWidth:       true
                        }
                    }
                }
            }

            Column {
                QGCLabel { text: qsTr("Clipboard Values:") }

                GridLayout {
                    rows:           savedRepeater.model.length
                    flow:           GridLayout.TopToBottom
                    rowSpacing:     0
                    columnSpacing:  _margins

                    Repeater {
                        model: axis[_currentAxis].params

                        QGCLabel { text: param }
                    }

                    Repeater {
                        id: savedRepeater

                        QGCLabel { text: modelData }
                    }
                }
            }

            RowLayout {
                spacing: _margins

                QGCButton {
                    text:       qsTr("Save To Clipboard")
                    onClicked:  saveTuningParamValues()
                }

                QGCButton {
                    text:       qsTr("Restore From Clipboard")
                    enabled:    root.parametersEnabled
                    onClicked:  resetToSavedTuningParamValues()
                }
            }
            }
        }
    }

} // GridLayout
