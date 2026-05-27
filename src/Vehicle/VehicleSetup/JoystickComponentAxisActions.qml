import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    required property var joystick

    GridLayout {
        columns: 4
        columnSpacing: ScreenTools.defaultFontPixelWidth
        rowSpacing: ScreenTools.defaultFontPixelHeight / 4

        QGCLabel { text: qsTr("Axis") }
        QGCLabel { text: qsTr("Low") }
        QGCLabel { text: qsTr("Center") }
        QGCLabel { text: qsTr("High") }

        Repeater {
            model: joystick ? joystick.axisCount : 0

            RowLayout {
                id: axisActionRow
                Layout.columnSpan: 4
                spacing: ScreenTools.defaultFontPixelWidth

                readonly property int axisIndex: index

                function refresh() {
                    lowActionCombo.selectAssignedAction()
                    midActionCombo.selectAssignedAction()
                    highActionCombo.selectAssignedAction()
                }

                QGCLabel {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                    text: qsTr("Axis %1").arg(axisActionRow.axisIndex + 1)
                }

                QGCComboBox {
                    id: lowActionCombo
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 20
                    model: joystick.assignableActionTitles
                    sizeToContents: true

                    function selectAssignedAction() {
                        let actionIndex = find(joystick.getAxisAction(axisActionRow.axisIndex, 0))
                        currentIndex = actionIndex < 0 ? 0 : actionIndex
                    }

                    Component.onCompleted: selectAssignedAction()
                    onActivated: (index) => joystick.setAxisAction(axisActionRow.axisIndex, 0, textAt(index))
                }

                QGCComboBox {
                    id: midActionCombo
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 20
                    model: joystick.assignableActionTitles
                    sizeToContents: true

                    function selectAssignedAction() {
                        let actionIndex = find(joystick.getAxisAction(axisActionRow.axisIndex, 1))
                        currentIndex = actionIndex < 0 ? 0 : actionIndex
                    }

                    Component.onCompleted: selectAssignedAction()
                    onActivated: (index) => joystick.setAxisAction(axisActionRow.axisIndex, 1, textAt(index))
                }

                QGCComboBox {
                    id: highActionCombo
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 20
                    model: joystick.assignableActionTitles
                    sizeToContents: true

                    function selectAssignedAction() {
                        let actionIndex = find(joystick.getAxisAction(axisActionRow.axisIndex, 2))
                        currentIndex = actionIndex < 0 ? 0 : actionIndex
                    }

                    Component.onCompleted: selectAssignedAction()
                    onActivated: (index) => joystick.setAxisAction(axisActionRow.axisIndex, 2, textAt(index))
                }

                Connections {
                    target: joystick
                    function onAxisActionsChanged() { axisActionRow.refresh() }
                    function onAssignableActionsChanged() { axisActionRow.refresh() }
                }
            }
        }
    }
}
