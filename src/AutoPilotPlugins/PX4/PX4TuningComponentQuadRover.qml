import QtQuick

import QGroundControl
import QGroundControl.Controls

SetupPage {
    pageComponent: pageComponent

    Component {
        id: pageComponent

        PX4TuningComponent {
            model: ListModel {
                ListElement {
                    buttonText: qsTr("Multirotor")
                    tuningPage: "PX4TuningComponentCopterAll.qml"
                }
                ListElement {
                    buttonText: qsTr("Rover")
                    tuningPage: "PX4TuningComponentRoverAll.qml"
                }
            }
        }
    }
}
