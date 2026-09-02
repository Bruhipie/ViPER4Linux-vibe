import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string label: "1k"
    property real level: 0.0
    signal bandMoved(real newLevel)

    width: 32
    height: 140

    // Keep slider in sync with external changes (presets, Flat button, graph drag)
    onLevelChanged: {
        if (!vSlider.pressed && Math.abs(vSlider.value - root.level) > 0.01) {
            vSlider.value = root.level
        }
    }

    Column {
        anchors.fill: parent
        spacing: 4

        // dB value top
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: {
                var val = vSlider.pressed ? (Math.round(vSlider.value * 10.0) / 10.0) : root.level
                return (val > 0 ? ("+" + val.toFixed(1)) : val.toFixed(1))
            }
            font.pixelSize: 10
            font.family: "Monospace"
            color: (vSlider.pressed ? (Math.abs(vSlider.value) > 0.05) : (Math.abs(root.level) > 0.05)) ? "#00D2B4" : "#6B7280"
            font.bold: (vSlider.pressed ? (Math.abs(vSlider.value) > 0.05) : (Math.abs(root.level) > 0.05))
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
                root.bandMoved(rounded)
            }

            background: Rectangle {
                x: vSlider.leftPadding + vSlider.availableWidth / 2 - width / 2
                y: vSlider.topPadding
                width: 3
                height: vSlider.availableHeight
                radius: 1.5
                color: "#252731"

                // Center zero line indicator (0 dB)
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: Math.round(parent.height / 2)
                    width: 7
                    height: 1
                    color: "#4B5563"
                }

                // Bipolar fill from 0 dB center to current value
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    y: vSlider.visualPosition < 0.5 ? (vSlider.visualPosition * parent.height) : (parent.height / 2)
                    height: Math.abs(0.5 - vSlider.visualPosition) * parent.height
                    radius: 1.5
                    color: "#00D2B4"
                }
            }

            handle: Rectangle {
                x: vSlider.leftPadding + vSlider.availableWidth / 2 - width / 2
                y: vSlider.topPadding + vSlider.visualPosition * (vSlider.availableHeight - height)
                width: 14
                height: 10
                radius: 3
                color: vSlider.pressed ? "#FFFFFF" : (vSlider.hovered ? "#33E6CB" : "#00D2B4")
                border.color: "#111215"
                border.width: 1
            }
        }

        // Freq label bottom
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.pixelSize: 10
            color: "#8B92A2"
        }
    }
}
