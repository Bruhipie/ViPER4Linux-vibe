import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var model: []
    property int currentIndex: 0
    signal selected(int index)

    implicitHeight: 28
    implicitWidth: 240

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 6
        color: "#141519"
        border.color: "#282A33"
        border.width: 1

        // Active indicator pill
        Rectangle {
            id: indicator
            width: root.model.length > 0 ? (bg.width / root.model.length) - 4 : 0
            height: bg.height - 4
            y: 2
            x: root.model.length > 0 ? (root.currentIndex * (bg.width / root.model.length)) + 2 : 2
            radius: 5
            color: "#1B2F2A"
            border.color: "#00D2B4"
            border.width: 1

            Behavior on x {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }
        }

        Row {
            anchors.fill: parent

            Repeater {
                model: root.model

                Item {
                    id: segItem
                    width: bg.width / root.model.length
                    height: bg.height

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 11
                        font.weight: root.currentIndex === index ? Font.DemiBold : Font.Normal
                        color: root.currentIndex === index ? "#00D2B4" : "#8B92A2"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentIndex = index
                            root.selected(index)
                        }
                    }
                }
            }
        }
    }
}
