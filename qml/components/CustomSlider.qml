import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string title: ""
    property int value: 0
    property int from: 0
    property int to: 100
    property int stepSize: 1
    property var displayFn: function(val) { return val.toString() }

    height: 32
    implicitHeight: 32
    implicitWidth: 260
    opacity: enabled ? 1.0 : 0.45

    // Title Label
    Text {
        id: label
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        color: "#8E8B9E"
        font.pixelSize: 12
        width: 85
        elide: Text.ElideRight
    }

    // Editable Value Badge on the Right
    Rectangle {
        id: badge
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 62
        height: 22
        radius: 4
        color: editInput.activeFocus ? "#1C182E" : "#13111F"
        border.color: editInput.activeFocus ? "#A88CFA" : "#28253B"
        border.width: 1

        Text {
            id: displayText
            anchors.centerIn: parent
            visible: !editInput.activeFocus
            text: root.displayFn(root.value)
            color: "#DFD4FF"
            font.pixelSize: 11
            font.family: "Monospace"
            font.bold: true
        }

        TextInput {
            id: editInput
            anchors.fill: parent
            anchors.margins: 2
            visible: activeFocus
            color: "#FFFFFF"
            font.pixelSize: 11
            font.family: "Monospace"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            selectByMouse: true

            onAccepted: {
                var num = parseFloat(text)
                if (!isNaN(num)) {
                    // Check if value is dB or raw
                    var targetVal = Math.round(num)
                    if (targetVal < root.from) targetVal = root.from
                    if (targetVal > root.to) targetVal = root.to
                    root.value = targetVal
                }
                focus = false
            }

            onActiveFocusChanged: {
                if (!activeFocus) {
                    text = root.value.toString()
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: !editInput.activeFocus
            cursorShape: Qt.IBeamCursor
            onClicked: {
                editInput.text = root.value.toString()
                editInput.forceActiveFocus()
                editInput.selectAll()
            }
        }
    }

    // The Interactive Slider (Fills all space between Label and Badge)
    Slider {
        id: slider
        anchors.left: label.right
        anchors.right: badge.left
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: 24

        from: root.from
        to: root.to
        stepSize: root.stepSize
        value: root.value

        onMoved: {
            var rounded = Math.round(slider.value / root.stepSize) * root.stepSize
            if (root.value !== rounded) {
                root.value = rounded
            }
        }

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 120
            implicitHeight: 4
            width: slider.availableWidth
            height: 4
            radius: 2
            color: "#222030"

            Rectangle {
                width: Math.max(0, Math.min(parent.width, slider.visualPosition * parent.width))
                height: parent.height
                radius: 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#8E6BF5" }
                    GradientStop { position: 1.0; color: "#B99FFF" }
                }
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 14
            implicitHeight: 14
            width: 14
            height: 14
            radius: 7
            color: slider.pressed ? "#FFFFFF" : "#E4DCFF"
            border.color: "#A88CFA"
            border.width: 2

            Behavior on scale {
                NumberAnimation { duration: 100 }
            }
            scale: slider.pressed ? 1.2 : 1.0
        }
    }
}
