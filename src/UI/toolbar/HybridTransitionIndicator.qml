import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id:                     control
    Layout.fillHeight:      true
    spacing:                ScreenTools.defaultFontPixelWidth / 2
    visible:                !!activeVehicle && activeVehicle.quadRover

    property var activeVehicle: null
    property var state:         activeVehicle ? activeVehicle.hybridVehicleState : null
    property var controller:    activeVehicle ? activeVehicle.hybridTransitionController : null

    readonly property string _stateText: {
        if (!state) {
            return qsTr("Unknown")
        }
        if (state.currentState === state.Quad) {
            return qsTr("Quad")
        }
        if (state.currentState === state.Rover) {
            return qsTr("Rover")
        }
        if (state.currentState === state.Transitioning) {
            return qsTr("Transitioning")
        }
        if (state.currentState === state.TransitionFault) {
            return qsTr("Fault")
        }
        return qsTr("Unknown")
    }

    readonly property color _stateColor: state && state.currentState === state.TransitionFault ? qgcPal.warningText : qgcPal.text

    QGCPalette { id: qgcPal }

    QGCLabel {
        text:               qsTr("Hybrid: %1").arg(control._stateText)
        color:              control._stateColor
        font.pointSize:     ScreenTools.smallFontPointSize
    }

    QGCLabel {
        text:               state ? qsTr("Fault %1").arg(state.faultReason) : ""
        color:              qgcPal.warningText
        font.pointSize:     ScreenTools.smallFontPointSize
        visible:            state && state.currentState === state.TransitionFault
    }

    QGCLabel {
        text:               state ? qsTr("%1%").arg(Math.round(state.positionNormalized * 100)) : ""
        color:              qgcPal.text
        font.pointSize:     ScreenTools.smallFontPointSize
        visible:            state && state.positionNormalizedValid
    }

    QGCButton {
        text:                   qsTr("Quad")
        enabled:                state && control.controller && state.canRequestTransform && !control.controller.busy && state.currentState !== state.Quad
        Layout.preferredHeight:  ScreenTools.defaultFontPixelHeight * 2
        ToolTip.text:           qsTr("Transform to quad shape")
        ToolTip.visible:        hovered

        onClicked: control.controller.requestTransform(state.TargetQuad)
    }

    QGCButton {
        text:                   qsTr("Rover")
        enabled:                state && control.controller && state.canRequestTransform && !control.controller.busy && state.currentState !== state.Rover
        Layout.preferredHeight:  ScreenTools.defaultFontPixelHeight * 2
        ToolTip.text:           qsTr("Transform to rover shape")
        ToolTip.visible:        hovered

        onClicked: control.controller.requestTransform(state.TargetRover)
    }
}
