import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string title: ""
    property bool selected: false
    property int activeCount: 0
    signal clicked()

    width: parent.width
    height: 40
    radius: 6
    color: root.selected ? "#1E2028" : (mouseArea.containsMouse ? "#181A21" : "transparent")

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    // Active indicator bar on the left
    Rectangle {
        width: 3
        height: 18
        radius: 1.5
        color: "#00D2B4"
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        visible: root.selected
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Text {
            text: root.title
            color: root.selected ? "#F1F3F7" : (mouseArea.containsMouse ? "#D1D5DB" : "#8B92A2")
            font.pixelSize: 13
            font.weight: root.selected ? Font.DemiBold : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            width: parent.width - (badge.visible ? badge.width + 6 : 0)
        }

        Rectangle {
            id: badge
            visible: root.activeCount > 0
            width: countText.implicitWidth + 8
            height: 16
            radius: 8
            color: root.selected ? "#0F3D34" : "#1B2A26"
            border.color: "#00D2B4"
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: countText
                anchors.centerIn: parent
                text: root.activeCount.toString()
                font.pixelSize: 10
                font.bold: true
                color: "#00D2B4"
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
