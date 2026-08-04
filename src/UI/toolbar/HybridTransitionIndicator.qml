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
        if (state.resetCandidateActive) {
            return qsTr("Synchronizing")
        }
        if (!state.hasValidStatus) {
            if (state.localMonotonicStatusReceiptTime < 0) {
                return qsTr("Waiting for status")
            }
            if (state.stale) {
                return qsTr("Stale")
            }
            return qsTr("Waiting for status")
        }
        return state.currentStateText
    }

    readonly property bool _showTransaction: controller && controller.transactionState !== controller.Idle
    readonly property bool _showFault: state &&
                                       (state.currentState === state.TransitionFault || state.faultReason !== 0)
    readonly property color _stateColor: _showFault || (state && (state.stale || state.resetCandidateActive)) ?
                                             qgcPal.warningText : qgcPal.text

    function _requestTransform(target) {
        if (!controller || !state) {
            return
        }
        if (!controller.requestTransform(target)) {
            QGroundControl.showMessageDialog(control,
                                             qsTr("Hybrid Transition"),
                                             controller.requestUnavailableReason || qsTr("Transition request was not accepted."))
        }
    }

    QGCPalette { id: qgcPal }

    QGCLabel {
        text:               qsTr("Hybrid: %1").arg(control._stateText)
        color:              control._stateColor
        font.pointSize:     ScreenTools.smallFontPointSize
    }

    QGCLabel {
        text:               state ? state.faultReasonText : ""
        color:              qgcPal.warningText
        font.pointSize:     ScreenTools.smallFontPointSize
        elide:              Text.ElideRight
        Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 22
        visible:            control._showFault

        HoverHandler { id: faultHover }
        ToolTip.text:       state ? state.faultReasonText : ""
        ToolTip.visible:    faultHover.hovered
    }

    QGCLabel {
        text:               controller ? controller.transactionStateText : ""
        color:              qgcPal.warningText
        font.pointSize:     ScreenTools.smallFontPointSize
        elide:              Text.ElideRight
        Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 16
        visible:            control._showTransaction

        HoverHandler { id: transactionHover }
        ToolTip.text:       controller ? controller.requestUnavailableReason : ""
        ToolTip.visible:    transactionHover.hovered
    }

    QGCLabel {
        text:               state ? qsTr("%1%").arg(Math.round(state.positionNormalized * 100)) : ""
        color:              qgcPal.text
        font.pointSize:     ScreenTools.smallFontPointSize
        visible:            state && state.positionNormalizedValid
    }

    QGCDelayButton {
        readonly property bool alreadyInTargetShape: state && state.currentState === state.Quad

        text:                   qsTr("Quad")
        enabled:                state && control.controller && control.controller.canRequestTransform && !alreadyInTargetShape
        Layout.preferredHeight:  ScreenTools.defaultFontPixelHeight * 2
        ToolTip.text:           alreadyInTargetShape ? qsTr("Vehicle is already in quad shape") :
                                (enabled ? qsTr("Hold to transform to quad shape") :
                                           (control.controller ? control.controller.requestUnavailableReason : qsTr("Hybrid control unavailable")))
        ToolTip.visible:        hovered

        onActivated: {
            if (control.state) {
                control._requestTransform(control.state.TargetQuad)
            }
        }
    }

    QGCDelayButton {
        readonly property bool alreadyInTargetShape: state && state.currentState === state.Rover

        text:                   qsTr("Rover")
        enabled:                state && control.controller && control.controller.canRequestTransform && !alreadyInTargetShape
        Layout.preferredHeight:  ScreenTools.defaultFontPixelHeight * 2
        ToolTip.text:           alreadyInTargetShape ? qsTr("Vehicle is already in rover shape") :
                                (enabled ? qsTr("Hold to transform to rover shape") :
                                           (control.controller ? control.controller.requestUnavailableReason : qsTr("Hybrid control unavailable")))
        ToolTip.visible:        hovered

        onActivated: {
            if (control.state) {
                control._requestTransform(control.state.TargetRover)
            }
        }
    }
}
