import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string title: ""
    property bool checked: false
    property bool expanded: false

    signal toggled(bool isChecked)

    height: 44

    // Toggle switch vertically centered from top and bottom
    CustomSwitch {
        id: toggle
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        checked: root.checked
        onToggled: function(val) {
            root.toggled(val)
            if (val && !root.expanded) {
                root.expanded = true
            }
        }
    }

    // Clickable header area (title + chevron)
    Item {
        anchors.left: toggle.right
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Text {
            anchors.left: parent.left
            anchors.right: chevron.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            font.pixelSize: 13
            font.weight: root.checked ? Font.DemiBold : Font.Normal
            color: root.checked ? "#F1F3F7" : "#8B92A2"
            elide: Text.ElideRight
        }

        Text {
            id: chevron
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: "›"
            font.pixelSize: 18
            font.bold: true
            color: root.checked ? "#00D2B4" : "#555A68"
            rotation: root.expanded ? 90 : 0

            Behavior on rotation {
                NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.expanded = !root.expanded
            }
        }
    }

    // Subtle divider line (only visible when expanded)
    Rectangle {
        width: parent.width
        height: 1
        color: "#22242D"
        anchors.bottom: parent.bottom
        visible: root.expanded
    }
}
