import QtQuick
import QtQuick.Controls

Item {
    id: root
    property bool checked: false
    property string title: ""

    signal toggled(bool isChecked)

    implicitWidth: title.length > 0 ? (track.width + label.implicitWidth + 8) : track.width
    implicitHeight: 22

    Row {
        anchors.fill: parent
        spacing: 8

        Rectangle {
            id: track
            width: 36
            height: 20
            radius: 10
            anchors.verticalCenter: parent.verticalCenter
            color: root.checked ? "#00D2B4" : "#262832"
            border.color: root.checked ? "#14E8C9" : "#363A46"
            border.width: 1

            Behavior on color {
                ColorAnimation { duration: 160; easing.type: Easing.OutQuad }
            }

            Rectangle {
                id: thumb
                width: 16
                height: 16
                radius: 8
                y: 2
                x: root.checked ? track.width - width - 2 : 2
                color: "#FFFFFF"

                Behavior on x {
                    NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.checked = !root.checked
                    root.toggled(root.checked)
                }
            }
        }

        Text {
            id: label
            visible: root.title.length > 0
            text: root.title
            color: root.checked ? "#F1F3F7" : "#8B92A2"
            font.pixelSize: 12
            font.weight: root.checked ? Font.Medium : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
