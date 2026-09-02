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
        width: 330
        height: 380
        anchors.centerIn: parent
        radius: 12
        color: "#161424"
        border.color: "#342F4D"
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
                    color: "#C2ABFF"
                    font.pixelSize: 15
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 40 }

                Rectangle {
                    width: 22
                    height: 22
                    radius: 11
                    color: "#252236"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: "#9C98B3"
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
                color: "#28253B"
            }

            // Status Rows
            Column {
                width: parent.width
                spacing: 9

                // Row 1: App Version
                Row {
                    width: parent.width
                    Text { text: "App Version"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Text { text: "1.0.0 (Linux x86_64)"; color: "#DFD4FF"; font.pixelSize: 12; font.family: "Monospace" }
                }

                // Row 2: DSP Engine
                Row {
                    width: parent.width
                    Text { text: "DSP Engine"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Text { text: "ViPERDSP Core v" + viperState.dspVersion; color: "#DFD4FF"; font.pixelSize: 12; font.family: "Monospace" }
                }

                // Row 3: Audio Server
                Row {
                    width: parent.width
                    Text { text: "Audio Server"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 7; height: 7; radius: 3.5; color: "#34D399"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: viperState.audioServerName; color: "#34D399"; font.pixelSize: 12; font.bold: true }
                    }
                }

                // Row 4: Driver Status
                Row {
                    width: parent.width
                    Text { text: "Driver Status"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 7; height: 7; radius: 3.5; color: "#34D399"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: viperState.driverStatusText; color: "#DFD4FF"; font.pixelSize: 12 }
                    }
                }

                // Row 5: Sampling Rate
                Row {
                    width: parent.width
                    Text { text: "Sample Rate"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Text { text: viperState.currentSampleRate + " Hz"; color: "#DFD4FF"; font.pixelSize: 12; font.family: "Monospace" }
                }

                // Row 6: Active Output Device
                Row {
                    width: parent.width
                    Text { text: "Output Device"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Text {
                        text: viperState.outputDeviceName
                        color: "#DFD4FF"
                        font.pixelSize: 11
                        width: parent.width - 110
                        elide: Text.ElideMiddle
                    }
                }

                // Row 7: Processing State
                Row {
                    width: parent.width
                    Text { text: "Streaming"; color: "#7E7B92"; font.pixelSize: 12; width: 110 }
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 7; height: 7; radius: 3.5; color: viperState.isEnabled ? "#34D399" : "#F59E0B"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: viperState.isEnabled ? "Active" : "Bypassed"; color: viperState.isEnabled ? "#34D399" : "#F59E0B"; font.pixelSize: 12; font.bold: true }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#28253B"
            }

            // Buttons
            Row {
                width: parent.width
                spacing: 10

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 32
                    radius: 6
                    color: "#252236"
                    border.color: "#3A3654"

                    Text {
                        anchors.centerIn: parent
                        text: "Refresh Status"
                        color: "#DFD4FF"
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
                    color: "#463973"
                    border.color: "#A88CFA"

                    Text {
                        anchors.centerIn: parent
                        text: "Reload Engine"
                        color: "#FFFFFF"
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
