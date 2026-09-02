import QtQuick
import QtQuick.Controls

Item {
    id: root
    property bool expanded: false
    default property alias content: innerContainer.data

    width: parent.width
    clip: true

    // Animate height and opacity smoothly
    height: expanded ? innerContainer.implicitHeight : 0
    opacity: expanded ? 1.0 : 0.0
    visible: height > 0 || opacity > 0

    Behavior on height {
        NumberAnimation {
            duration: 240
            easing.type: Easing.OutCubic
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutQuad
        }
    }

    Item {
        id: innerContainer
        width: root.width
        implicitHeight: childrenRect.height
    }
}
