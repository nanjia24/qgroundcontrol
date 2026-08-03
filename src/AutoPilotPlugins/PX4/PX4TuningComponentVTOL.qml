import QtQuick

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id:            tuningPage
    pageComponent: pageComponent

    Component {
        id: pageComponent

        PX4TuningComponent {
            id: root

            model: tuningPages

            readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
            property var _hybridQuadRover: null

            function syncRoverPage() {
                var showRover = !!_activeVehicle
                    && _activeVehicle.miniRover
                    && _activeVehicle.parameterManager.parametersReady
                    && !!_hybridQuadRover
                    && Number(_hybridQuadRover.rawValue) === 1

                if (showRover && tuningPages.count === 1) {
                    tuningPages.append({
                        "buttonText": qsTr("Rover"),
                        "tuningPage": "PX4TuningComponentRoverAll.qml"
                    })
                } else if (!showRover && tuningPages.count > 1) {
                    tuningPages.remove(1, tuningPages.count - 1)
                }
            }

            function refreshHybridFact() {
                _hybridQuadRover = null
                if (_activeVehicle
                        && _activeVehicle.miniRover
                        && _activeVehicle.parameterManager.parametersReady) {
                    _hybridQuadRover = factPanelController.getParameterFact(-1, "HYBR_QUAD_ROV", false)
                }
                syncRoverPage()
            }

            ListModel {
                id: tuningPages

                ListElement {
                    buttonText: qsTr("Multirotor")
                    tuningPage: "PX4TuningComponentCopterAll.qml"
                }
                //ListElement {
                //    buttonText: qsTr("Fixed Wing")
                //    tuningPage: "PX4TuningComponentPlaneAll.qml"
                //}
            }

            Connections {
                target: QGroundControl.multiVehicleManager

                function onActiveVehicleChanged() {
                    root._hybridQuadRover = null
                    root.syncRoverPage()
                    Qt.callLater(root.refreshHybridFact)
                }
            }

            Connections {
                target: root._activeVehicle ? root._activeVehicle.parameterManager : null

                function onParametersReadyChanged() {
                    root.refreshHybridFact()
                }
            }

            Connections {
                target: root._activeVehicle

                function onVehicleTypeChanged() {
                    root.refreshHybridFact()
                }
            }

            Connections {
                target: root._hybridQuadRover

                function onRawValueChanged() {
                    root.syncRoverPage()
                }
            }

            Component.onCompleted: refreshHybridFact()
        }
    }
}
