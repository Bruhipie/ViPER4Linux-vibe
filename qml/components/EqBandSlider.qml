import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string label: "1k"
    property real level: 0.0
    signal bandMoved(real newLevel)

    width: 32
    height: 140

    Column {
        anchors.fill: parent
        spacing: 4

        // dB value top
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.level > 0 ? ("+" + root.level.toFixed(1)) : root.level.toFixed(1)
            font.pixelSize: 10
            font.family: "Monospace"
            color: root.level !== 0.0 ? "#DFD4FF" : "#6E6B80"
        }

        // Vertical Slider
        Slider {
            id: vSlider
            orientation: Qt.Vertical
            anchors.horizontalCenter: parent.horizontalCenter
            height: 96
            from: -12.0
            to: 12.0
            stepSize: 0.1
            value: root.level

            onMoved: {
                var rounded = Math.round(vSlider.value * 10.0) / 10.0
                if (root.level !== rounded) {
                    root.level = rounded
                    root.bandMoved(rounded)
                }
            }

            background: Rectangle {
                x: vSlider.leftPadding + vSlider.availableWidth / 2 - width / 2
                y: vSlider.topPadding
                width: 3
                height: vSlider.availableHeight
                radius: 1.5
                color: "#222030"

                Rectangle {
                    y: vSlider.visualPosition * parent.height
                    width: parent.width
                    height: (1.0 - vSlider.visualPosition) * parent.height
                    radius: 1.5
                    color: "#A88CFA"
                }
            }

            handle: Rectangle {
                x: vSlider.leftPadding + vSlider.availableWidth / 2 - width / 2
                y: vSlider.topPadding + vSlider.visualPosition * (vSlider.availableHeight - height)
                width: 14
                height: 10
                radius: 3
                color: vSlider.pressed ? "#FFFFFF" : "#D4C7FF"
                border.color: "#A88CFA"
                border.width: 1
            }
        }

        // Freq label bottom
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.pixelSize: 10
            color: "#8E8B9E"
        }
    }
}
