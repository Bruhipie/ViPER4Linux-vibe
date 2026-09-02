import QtQuick
import QtQuick.Controls

Item {
    id: root
    property bool expanded: false
    default property alias content: innerContainer.data

    width: parent.width
    clip: true

    // Height is set IMMEDIATELY (no Behavior here) so the PARENT card's
    // Behavior on height drives the single smooth expand/collapse animation.
    height: expanded ? innerContainer.implicitHeight : 0

    // Only opacity fades in/out
    opacity: expanded ? 1.0 : 0.0
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
