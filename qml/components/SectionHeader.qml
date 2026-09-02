import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string title: ""
    property bool checked: false
    property bool expanded: false

    signal toggled(bool isChecked)

    height: 38
    color: "transparent"

    Row {
        anchors.fill: parent
        spacing: 10

        // Toggle Switch on the left
        CustomSwitch {
            id: toggle
            checked: root.checked
            anchors.verticalCenter: parent.verticalCenter
            onToggled: function(val) {
                root.checked = val
                root.toggled(val)
                if (val && !root.expanded) {
                    root.expanded = true
                }
            }
        }

        // Clickable header area to expand/collapse
        Item {
            width: root.width - toggle.width - 20
            height: parent.height
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.left: parent.left
                anchors.right: chevron.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                font.pixelSize: 14
                font.weight: root.checked ? Font.DemiBold : Font.Normal
                color: root.checked ? "#DFD4FF" : "#8A879E"
                elide: Text.ElideRight
            }

            Text {
                id: chevron
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "›"
                font.pixelSize: 18
                font.bold: true
                color: root.checked ? "#C2ABFF" : "#5A576E"
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
    }

    // Divider line
    Rectangle {
        width: parent.width
        height: 1
        color: "#1E1C2B"
        anchors.bottom: parent.bottom
    }
}
