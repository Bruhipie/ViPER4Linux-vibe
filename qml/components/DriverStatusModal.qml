import QtQuick
import QtQuick.Controls

Item {
    id: root
    visible: false
    anchors.fill: parent
    z: 999

    signal closed()

    // Dim background
    Rectangle {
        anchors.fill: parent
        color: "#B0000000"

        MouseArea {
            anchors.fill: parent
            onClicked: root.closed()
        }
    }

    // Modal Card
    Rectangle {
        id: card
        width: 340
        height: 380
        anchors.centerIn: parent
        radius: 12
        color: "#18191E"
        border.color: "#282A33"
        border.width: 1

        MouseArea {
            anchors.fill: parent
            // Eat clicks inside modal
        }

        Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Row {
                width: parent.width
                spacing: 8

                Text {
                    text: "Engine & Driver Status"
                    color: "#F1F3F7"
                    font.pixelSize: 15
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: Math.max(10, parent.width - 230) }

                Rectangle {
                    width: 24
                    height: 24
                    radius: 12
                    color: "#22242D"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: "#9AA0AD"
                        font.pixelSize: 11
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closed()
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#252731"
            }

            // Status Rows
            Column {
                width: parent.width
                spacing: 9

                Row {
                    width: parent.width
                    Text { text: "App Version"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Text { text: "1.0.0 (Linux x86_64)"; color: "#F1F3F7"; font.pixelSize: 12; font.family: "Monospace" }
                }

                Row {
                    width: parent.width
                    Text { text: "DSP Engine"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Text { text: "ViPERDSP Core v" + viperState.dspVersion; color: "#F1F3F7"; font.pixelSize: 12; font.family: "Monospace" }
                }

                Row {
                    width: parent.width
                    Text { text: "Audio Server"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 7; height: 7; radius: 3.5; color: "#00D2B4"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: viperState.audioServerName; color: "#00D2B4"; font.pixelSize: 12; font.bold: true }
                    }
                }

                Row {
                    width: parent.width
                    Text { text: "Driver Status"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 7; height: 7; radius: 3.5; color: "#00D2B4"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: viperState.driverStatusText; color: "#F1F3F7"; font.pixelSize: 12 }
                    }
                }

                Row {
                    width: parent.width
                    Text { text: "Sample Rate"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Text { text: viperState.currentSampleRate + " Hz"; color: "#F1F3F7"; font.pixelSize: 12; font.family: "Monospace" }
                }

                Row {
                    width: parent.width
                    Text { text: "Output Device"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Text {
                        text: viperState.outputDeviceName
                        color: "#F1F3F7"
                        font.pixelSize: 11
                        width: parent.width - 110
                        elide: Text.ElideMiddle
                    }
                }

                Row {
                    width: parent.width
                    Text { text: "Streaming"; color: "#8B92A2"; font.pixelSize: 12; width: 110 }
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 7; height: 7; radius: 3.5; color: viperState.isEnabled ? "#00D2B4" : "#F59E0B"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: viperState.isEnabled ? "Active" : "Bypassed"; color: viperState.isEnabled ? "#00D2B4" : "#F59E0B"; font.pixelSize: 12; font.bold: true }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#252731"
            }

            // Buttons
            Row {
                width: parent.width
                spacing: 10

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 32
                    radius: 6
                    color: "#22242D"
                    border.color: "#313540"

                    Text {
                        anchors.centerIn: parent
                        text: "Refresh Status"
                        color: "#F1F3F7"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: viperState.refreshDriverStatus()
                    }
                }

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 32
                    radius: 6
                    color: "#163830"
                    border.color: "#00D2B4"

                    Text {
                        anchors.centerIn: parent
                        text: "Reload Engine"
                        color: "#00D2B4"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: viperState.reloadEngine()
                    }
                }
            }
        }
    }
}
