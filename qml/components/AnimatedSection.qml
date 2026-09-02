import QtQuick
import QtQuick.Controls

Item {
    id: root
    property bool expanded: false
    default property alias content: innerContainer.data

    width: parent.width
    clip: true

    // Height snaps instantly — the PARENT card's Behavior on height is the
    // only animation. implicitHeight must be the FULL content height
    // (not 0 / not the animated height) so the card's formula knows the
    // target size even while the card is still animating.
    height: expanded ? innerContainer.implicitHeight : 0
    implicitHeight: innerContainer.implicitHeight   // ← always the true content height

    // Only opacity fades
    opacity: expanded ? 1.0 : 0.0
    visible: expanded || opacity > 0          // hide once fully faded out
    Behavior on opacity {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    Item {
        id: innerContainer
        width: root.width
        implicitHeight: childrenRect.height
    }
}
