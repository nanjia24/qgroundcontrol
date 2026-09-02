import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

PX4TuningComponent {
    property real availableWidth:  parent ? parent.availableWidth : 0
    property real availableHeight: parent ? parent.availableHeight : 0

    model: ListModel {
        ListElement {
            buttonText: qsTr("Rate")
            tuningPage: "PX4TuningComponentRoverRate.qml"
        }
        ListElement {
            buttonText: qsTr("Attitude")
            tuningPage: "PX4TuningComponentRoverAttitude.qml"
        }
        ListElement {
            buttonText: qsTr("Velocity")
            tuningPage: "PX4TuningComponentRoverVelocity.qml"
        }
        ListElement {
            buttonText: qsTr("Path Tracking")
            tuningPage: "PX4TuningComponentRoverPosition.qml"
        }
    }
}
