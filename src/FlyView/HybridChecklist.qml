import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {
    property var model: listModel

    PreFlightCheckModel {
        id: listModel

        PreFlightCheckGroup {
            name: qsTr("Quad-Rover Initial Checks")

            PreFlightHybridStateCheck {
            }

            PreFlightCheckButton {
                name:       qsTr("Hardware")
                manualText: qsTr("Battery, propellers, wheels, and transform mechanism secured?")
            }

            PreFlightBatteryCheck {
                failurePercent:              40
                allowFailurePercentOverride: false
            }

            PreFlightSensorsHealthCheck {
            }

            PreFlightGPSCheck {
                failureSatCount:       9
                allowOverrideSatCount: true
            }

            PreFlightRCCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Please arm the vehicle here")

            PreFlightCheckButton {
                name:       qsTr("Mission")
                manualText: qsTr("Mission route and commanded shape sequence verified?")
            }

            PreFlightSoundCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Last preparations before operation")

            PreFlightCheckButton {
                name:       qsTr("Payload")
                manualText: qsTr("Configured, secured, and started?")
            }

            PreFlightCheckButton {
                name:       qsTr("Wind & weather")
                manualText: qsTr("Suitable for both flight and ground operation?")
            }

            PreFlightCheckButton {
                name:       qsTr("Mission area")
                manualText: qsTr("Flight path, ground path, and transform area clear of obstacles and people?")
            }
        }
    }
}
