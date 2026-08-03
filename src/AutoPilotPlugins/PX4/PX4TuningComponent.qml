import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id:         root
    spacing:    ScreenTools.defaultFontPixelWidth / 4

    property var model
    property alias factPanelController: controller

    property real _availableHeight: availableHeight
    property real _availableWidth:  availableWidth

    FactPanelController {
        id:         controller
    }

    QGCTabBar {
        id: tabBar

        Repeater {
            model: root.model
            QGCTabButton {
                text: buttonText
            }
        }
    }

    Loader {
        id:     loader
        source: {
            if (!root.model || root.model.count === 0) {
                return ""
            }
            var pageIndex = tabBar.currentIndex >= 0 && tabBar.currentIndex < root.model.count ? tabBar.currentIndex : 0
            return root.model.get(pageIndex).tuningPage
        }

        property bool useAutoTuning:    true
        property real availableWidth:   _availableWidth
        property real availableHeight:  _availableHeight - loader.y
    }
}
