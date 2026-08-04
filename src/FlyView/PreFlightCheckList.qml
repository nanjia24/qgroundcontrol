import QtQuick
import QtQuick.Controls
import QtQml.Models
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ColumnLayout {
    id: _root

    spacing: 0.8 * ScreenTools.defaultFontPixelWidth

    property real _verticalMargin: ScreenTools.defaultFontPixelHeight / 2

    Loader {
        id:     modelContainer
        source: "qrc:/qml/QGroundControl/FlyView/DefaultChecklist.qml"

        onLoaded: {
            if (modelContainer.item && modelContainer.item.model) {
                modelContainer.item.model.reset()
            }
        }
    }

    property bool allChecksPassed:  false
    property var  vehicleCopy:      QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle

    onVehicleCopyChanged: {
        _root._resetForVehicle()
    }

    onAllChecksPassedChanged: {
        _root._syncVehicleChecklistState()
    }

    function _syncVehicleChecklistState() {
        if (_root.vehicleCopy && _root.vehicleCopy === QGroundControl.multiVehicleManager.activeVehicle) {
            _root.vehicleCopy.checkListState = _root.allChecksPassed ? Vehicle.CheckListPassed : Vehicle.CheckListFailed
        }
    }

    function _handleGroupPassedChanged(index, passed) {
        if (passed) {
            // Collapse current group
            var group = checkListRepeater.itemAt(index)
            group._checked = false
            // Expand next group
            if (index + 1 < checkListRepeater.count) {
                group = checkListRepeater.itemAt(index + 1)
                group.enabled = true
                group._checked = true
            }
        }

        // Walk the list and check if any group is failing
        var allPassed = true
        for (var i=0; i < checkListRepeater.count; i++) {
            if (!checkListRepeater.itemAt(i).passed) {
                allPassed = false
                break
            }
        }
        _root.allChecksPassed = allPassed;
    }

    //-- Pick a checklist model that matches the current airframe type (if any)
    function _updateModel() {
        var vehicle = _root.vehicleCopy

        if(!vehicle) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/DefaultChecklist.qml"
        } else if(vehicle.quadRover) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/HybridChecklist.qml"
        } else if(vehicle.multiRotor) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/MultiRotorChecklist.qml"
        } else if(vehicle.vtol) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/VTOLChecklist.qml"
        } else if(vehicle.rover) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/RoverChecklist.qml"
        } else if(vehicle.sub) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/SubChecklist.qml"
        } else if(vehicle.fixedWing) {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/FixedWingChecklist.qml"
        } else {
            modelContainer.source = "qrc:/qml/QGroundControl/FlyView/DefaultChecklist.qml"
        }
        return
    }

    function _resetForVehicle() {
        delayedGroupPassed.stop()
        _root.allChecksPassed = false
        _root._syncVehicleChecklistState()
        _root._updateModel()
        if (modelContainer.item && modelContainer.item.model) {
            modelContainer.item.model.reset()
        }
    }

    Connections {
        target: _root.vehicleCopy

        function onVehicleTypeChanged() {
            _root._resetForVehicle()
        }
    }

    Component.onCompleted: {
        _root._resetForVehicle()
    }

    onVisibleChanged: {
        if(_root.visible) {
            _root._updateModel()
        }
    }

    // We delay the updates when a group passes so the user can see all items green for a moment prior to hiding
    Timer {
        id:         delayedGroupPassed
        interval:   750

        property int index

        onTriggered: _root._handleGroupPassedChanged(delayedGroupPassed.index, true /* passed */)
    }

    function groupPassedChanged(index, passed) {
        if (passed) {
            delayedGroupPassed.index = index
            delayedGroupPassed.restart()
        } else {
            _root._handleGroupPassedChanged(index, passed)
        }
    }

    // Header/title of checklist
    RowLayout {
        Layout.fillWidth:   true
        height:             1.75 * ScreenTools.defaultFontPixelHeight
        spacing:            0

        QGCLabel {
            Layout.fillWidth:   true
            text:               _root.allChecksPassed ? qsTr("(Passed)") : qsTr("In Progress")
            font.pointSize:     ScreenTools.mediumFontPointSize
        }
        QGCButton {
            width:              1.2 * ScreenTools.defaultFontPixelHeight
            height:             1.2 * ScreenTools.defaultFontPixelHeight
            Layout.alignment:   Qt.AlignVCenter
            enabled:            !!checkListRepeater.model
            onClicked: {
                if (checkListRepeater.model) {
                    checkListRepeater.model.reset()
                }
            }

            QGCColoredImage {
                source:         "/qmlimages/MapSyncBlack.svg"
                color:          qgcPal.buttonText
                anchors.fill:   parent
            }
        }
    }

    // All check list items
    Repeater {
        id:     checkListRepeater
        model:  modelContainer.item ? modelContainer.item.model : null
    }
}
