import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ViPER4Linux 1.0
import "components"

ApplicationWindow {
    id: appWindow
    visible: true
    width: 480
    height: 980
    minimumWidth: 380
    minimumHeight: 560
    title: "ViPER4Linux"
    color: "#0A0814" // viperDeepBg

    // File Dialog for Convolver & DDC
    FileDialog {
        id: convolverDialog
        title: "Select Impulse Response (WAV / IRS)"
        nameFilters: ["Audio IR files (*.wav *.irs)", "All files (*)"]
        currentFolder: "file://" + viperState.defaultIrsFolder
        onAccepted: {
            viperState.selectConvolverKernel(convolverDialog.selectedFile.toString())
        }
    }

    FileDialog {
        id: ddcDialog
        title: "Select ViPER-DDC Profile (.vdc)"
        nameFilters: ["ViPER-DDC files (*.vdc)", "All files (*)"]
        onAccepted: {
            viperState.selectDdcProfile(ddcDialog.selectedFile.toString())
        }
    }

    // Top Header
    Rectangle {
        id: header
        width: parent.width
        height: 88
        color: "#161424"
        border.color: "#262338"
        border.width: 1
        z: 10

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            // Top Row: Title, Driver Status Pill, (i) Info, Master Toggle
            Row {
                width: parent.width
                spacing: 8

                Row {
                    spacing: 6
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: "V"
                        color: "#C2ABFF"
                        font.pixelSize: 19
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: "ViPER4Linux"
                        color: "#C2ABFF"
                        font.pixelSize: 16
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Driver Status Badge (Clickable)
                Rectangle {
                    height: 24
                    width: statusRow.implicitWidth + 14
                    radius: 12
                    color: "#211E33"
                    border.color: "#353050"
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        id: statusRow
                        anchors.centerIn: parent
                        spacing: 5

                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: viperState.isEnabled ? "#34D399" : "#F59E0B"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: viperState.audioServerName.indexOf("PipeWire") >= 0 ? "PipeWire 1.6.8" : "Active"
                            color: "#DFD4FF"
                            font.pixelSize: 11
                            font.weight: Font.Medium
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: statusModal.visible = true
                    }
                }

                // (i) Button
                Rectangle {
                    width: 22
                    height: 22
                    radius: 11
                    color: "#252236"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "i"
                        color: "#C2ABFF"
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Monospace"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: statusModal.visible = true
                    }
                }

                Item { width: Math.max(10, parent.width - 290) }

                // Master Toggle Switch
                CustomSwitch {
                    id: masterSwitch
                    checked: viperState.isEnabled
                    anchors.verticalCenter: parent.verticalCenter
                    onToggled: function(val) {
                        viperState.isEnabled = val
                    }
                }
            }

            // Bottom Row: Audio Mode Segmented Picker [ Headphone | Speaker ]
            CustomSegmentedPicker {
                width: parent.width
                height: 26
                model: ["Headphone", "Speaker"]
                currentIndex: viperState.fxType
                onSelected: function(idx) {
                    viperState.fxType = idx
                }
            }
        }
    }

    // Scrollable Main Content
    ScrollView {
        id: scrollView
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        Column {
            width: scrollView.width - 20
            x: 10
            spacing: 12
            topPadding: 10
            bottomPadding: 15

            // Output Card
            Rectangle {
                width: parent.width
                height: outCol.implicitHeight + 16
                radius: 8
                color: "#13111F"
                border.color: "#252236"

                Column {
                    id: outCol
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text {
                        text: "OUTPUT"
                        color: "#8E8B9E"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    CustomSlider {
                        width: parent.width
                        title: "Output Gain"
                        from: 1; to: 200
                        value: viperState.outputVolume
                        onValueChanged: viperState.outputVolume = value
                        displayFn: function(v) {
                            var db = v > 0 ? 20.0 * Math.log10(v / 100.0) : -99.9
                            return (db >= 0 ? "+" : "") + db.toFixed(1) + "dB"
                        }
                    }

                    CustomSlider {
                        width: parent.width
                        title: "Output Pan"
                        from: -100; to: 100
                        value: viperState.channelPan
                        onValueChanged: viperState.channelPan = value
                        displayFn: function(v) {
                            return Math.round(50 - v / 2) + ":" + Math.round(50 + v / 2)
                        }
                    }

                    CustomSlider {
                        width: parent.width
                        title: "Limiter"
                        from: 30; to: 100
                        value: viperState.limiter
                        onValueChanged: viperState.limiter = value
                        displayFn: function(v) {
                            var db = v > 0 ? 20.0 * Math.log10(v / 100.0) : -99.9
                            return db.toFixed(1) + "dB"
                        }
                    }
                }
            }

            // 1. FIR Equalizer
            SectionHeader {
                id: eqHeader
                width: parent.width
                title: "FIR Equalizer"
                checked: viperState.equalizerEnabled
                onToggled: function(v) { viperState.equalizerEnabled = v }
            }

            Column {
                visible: eqHeader.expanded
                width: parent.width
                spacing: 8

                Row {
                    width: parent.width
                    spacing: 8

                    CustomSegmentedPicker {
                        width: 160
                        height: 24
                        model: ["10", "15", "25", "31"]
                        currentIndex: viperState.equalizerBandCount === 15 ? 1 : (viperState.equalizerBandCount === 25 ? 2 : (viperState.equalizerBandCount === 31 ? 3 : 0))
                        onSelected: function(idx) {
                            var counts = [10, 15, 25, 31]
                            viperState.setEqualizerBandCount(counts[idx])
                        }
                    }

                    ComboBox {
                        id: presetBox
                        model: viperState.presetList
                        height: 24
                        width: parent.width - 240
                        currentIndex: model.indexOf(viperState.equalizerPresetName)
                        onActivated: function(index) {
                            viperState.applyEqPreset(model[index])
                        }
                    }

                    Rectangle {
                        width: 58
                        height: 24
                        radius: 4
                        color: "#252236"
                        border.color: "#353050"

                        Text {
                            anchors.centerIn: parent
                            text: "Flat"
                            color: "#DFD4FF"
                            font.pixelSize: 11
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: viperState.resetEq()
                        }
                    }
                }

                // Smooth Spline Graph
                EqGraphItem {
                    id: eqGraph
                    width: parent.width
                    height: 120
                    bands: viperState.equalizerBands
                    bandCount: viperState.equalizerBandCount
                    onBandLevelChanged: function(bIdx, bLvl) {
                        viperState.setEqBandLevel(bIdx, bLvl)
                    }
                }

                // Vertical Band Sliders
                Flickable {
                    width: parent.width
                    height: 145
                    contentWidth: bandRow.width
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Row {
                        id: bandRow
                        spacing: 2

                        Repeater {
                            model: viperState.equalizerBands.length

                            EqBandSlider {
                                level: viperState.equalizerBands[index] !== undefined ? viperState.equalizerBands[index] : 0.0
                                label: {
                                    var f10 = ["31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"]
                                    if (viperState.equalizerBandCount === 10 && index < 10) return f10[index]
                                    return (index + 1).toString()
                                }
                                onBandMoved: function(newLevel) {
                                    viperState.setEqBandLevel(index, newLevel)
                                }
                            }
                        }
                    }
                }
            }

            // 2. ViPER Bass
            SectionHeader {
                id: bassHeader
                width: parent.width
                title: "ViPER Bass"
                checked: viperState.viperBassEnabled
                onToggled: function(v) { viperState.viperBassEnabled = v }
            }

            Column {
                visible: bassHeader.expanded
                width: parent.width
                spacing: 6

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Natural", "Pure Bass+", "Subwoofer"]
                    currentIndex: viperState.viperBassMode
                    onSelected: function(idx) { viperState.viperBassMode = idx }
                }

                CustomSlider {
                    width: parent.width
                    visible: viperState.viperBassMode !== 2
                    title: "Frequency"
                    from: 20; to: 150
                    value: viperState.viperBassFrequency
                    onValueChanged: viperState.viperBassFrequency = value
                    displayFn: function(v) { return v + " Hz" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Gain"
                    from: 50; to: 500
                    stepSize: 5
                    value: viperState.viperBassGain
                    onValueChanged: viperState.viperBassGain = value
                    displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                }

                Row {
                    width: parent.width
                    Text { text: "Fade-in Anti-Pop"; color: "#8E8B9E"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Item { width: Math.max(10, parent.width - 160) }
                    CustomSwitch {
                        checked: viperState.viperBassAntiPop
                        onToggled: function(v) { viperState.viperBassAntiPop = v }
                    }
                }
            }

            // 3. ViPER Bass Mono
            SectionHeader {
                id: bassMonoHeader
                width: parent.width
                title: "ViPER Bass Mono"
                checked: viperState.viperBassMonoEnabled
                onToggled: function(v) { viperState.viperBassMonoEnabled = v }
            }

            Column {
                visible: bassMonoHeader.expanded
                width: parent.width
                spacing: 6

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Natural", "Pure Bass+", "Subwoofer"]
                    currentIndex: viperState.viperBassMonoMode
                    onSelected: function(idx) { viperState.viperBassMonoMode = idx }
                }

                CustomSlider {
                    width: parent.width
                    visible: viperState.viperBassMonoMode !== 2
                    title: "Frequency"
                    from: 20; to: 150
                    value: viperState.viperBassMonoFrequency
                    onValueChanged: viperState.viperBassMonoFrequency = value
                    displayFn: function(v) { return v + " Hz" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Gain"
                    from: 50; to: 500
                    stepSize: 5
                    value: viperState.viperBassMonoGain
                    onValueChanged: viperState.viperBassMonoGain = value
                    displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                }
            }

            // 4. ViPER Clarity
            SectionHeader {
                id: clarityHeader
                width: parent.width
                title: "ViPER Clarity"
                checked: viperState.viperClarityEnabled
                onToggled: function(v) { viperState.viperClarityEnabled = v }
            }

            Column {
                visible: clarityHeader.expanded
                width: parent.width
                spacing: 6

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Natural", "OZone+", "XHiFi"]
                    currentIndex: viperState.viperClarityMode
                    onSelected: function(idx) { viperState.viperClarityMode = idx }
                }

                CustomSlider {
                    width: parent.width
                    title: "Clarity Gain"
                    from: 0; to: 450
                    value: viperState.viperClarityGain
                    onValueChanged: viperState.viperClarityGain = value
                    displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                }
            }

            // 5. Convolver
            SectionHeader {
                id: convolverHeader
                width: parent.width
                title: "Convolver"
                checked: viperState.convolutionEnabled
                onToggled: function(v) { viperState.convolutionEnabled = v }
            }

            Column {
                visible: convolverHeader.expanded
                width: parent.width
                spacing: 6

                Row {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: parent.width - 80
                        height: 28
                        radius: 4
                        color: "#13111F"
                        border.color: "#252236"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 6
                            text: viperState.convolutionKernelPath.length > 0 ? viperState.convolutionKernelPath.split("/").pop() : "No kernel loaded"
                            color: viperState.convolutionKernelPath.length > 0 ? "#DFD4FF" : "#6E6B80"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }
                    }

                    Rectangle {
                        width: 72
                        height: 28
                        radius: 4
                        color: "#342F4D"
                        border.color: "#A88CFA"

                        Text {
                            anchors.centerIn: parent
                            text: "Load IR"
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: convolverDialog.open()
                        }
                    }
                }

                CustomSlider {
                    width: parent.width
                    title: "Cross Ch."
                    from: 0; to: 100
                    value: viperState.convolutionCrossChannel
                    onValueChanged: viperState.convolutionCrossChannel = value
                    displayFn: function(v) { return v + "%" }
                }
            }

            // 6. ViPER-DDC
            SectionHeader {
                id: ddcHeader
                width: parent.width
                title: "ViPER-DDC"
                checked: viperState.ddcEnabled
                onToggled: function(v) { viperState.ddcEnabled = v }
            }

            Column {
                visible: ddcHeader.expanded
                width: parent.width
                spacing: 6

                Row {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: parent.width - 80
                        height: 28
                        radius: 4
                        color: "#13111F"
                        border.color: "#252236"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 6
                            text: viperState.ddcFilePath.length > 0 ? viperState.ddcFilePath.split("/").pop() : "No .vdc profile loaded"
                            color: viperState.ddcFilePath.length > 0 ? "#DFD4FF" : "#6E6B80"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }
                    }

                    Rectangle {
                        width: 72
                        height: 28
                        radius: 4
                        color: "#342F4D"
                        border.color: "#A88CFA"

                        Text {
                            anchors.centerIn: parent
                            text: "Load VDC"
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ddcDialog.open()
                        }
                    }
                }
            }

            // 7. Field Surround
            SectionHeader {
                id: surroundHeader
                width: parent.width
                title: "Field Surround"
                checked: viperState.fieldSurroundEnabled
                onToggled: function(v) { viperState.fieldSurroundEnabled = v }
            }

            Column {
                visible: surroundHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Widening"
                    from: 0; to: 8
                    value: viperState.fieldSurroundWidening
                    onValueChanged: viperState.fieldSurroundWidening = value
                    displayFn: function(v) { return v.toString() }
                }

                CustomSlider {
                    width: parent.width
                    title: "Mid Image"
                    from: 0; to: 10
                    value: viperState.fieldSurroundMidImage
                    onValueChanged: viperState.fieldSurroundMidImage = value
                    displayFn: function(v) { return v.toString() }
                }

                CustomSlider {
                    width: parent.width
                    title: "Depth"
                    from: 0; to: 10
                    value: viperState.fieldSurroundDepth
                    onValueChanged: viperState.fieldSurroundDepth = value
                    displayFn: function(v) { return v.toString() }
                }
            }

            // 8. Differential Surround
            SectionHeader {
                id: diffSurrHeader
                width: parent.width
                title: "Differential Surround"
                checked: viperState.diffSurroundEnabled
                onToggled: function(v) { viperState.diffSurroundEnabled = v }
            }

            Column {
                visible: diffSurrHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Delay"
                    from: 1; to: 20
                    value: viperState.diffSurroundDelay
                    onValueChanged: viperState.diffSurroundDelay = value
                    displayFn: function(v) { return v + " ms" }
                }

                Row {
                    width: parent.width
                    Text { text: "Phase Reverse"; color: "#8E8B9E"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Item { width: Math.max(10, parent.width - 150) }
                    CustomSwitch {
                        checked: viperState.diffSurroundReverse
                        onToggled: function(v) { viperState.diffSurroundReverse = v }
                    }
                }

                CustomSlider {
                    width: parent.width
                    title: "Wet / Dry"
                    from: 0; to: 100
                    value: viperState.diffSurroundWetDryMix
                    onValueChanged: viperState.diffSurroundWetDryMix = value
                    displayFn: function(v) { return v + "%" }
                }

                CustomSlider {
                    width: parent.width
                    title: "LP Cutoff"
                    from: 0; to: 20000; stepSize: 50
                    value: viperState.diffSurroundLpCutoff
                    onValueChanged: viperState.diffSurroundLpCutoff = value
                    displayFn: function(v) { return v === 0 ? "Off" : v + " Hz" }
                }
            }

            // 9. Headphone Surround+ (VHE)
            SectionHeader {
                id: vheHeader
                width: parent.width
                title: "Headphone Surround+ (VHE)"
                checked: viperState.vheEnabled
                onToggled: function(v) { viperState.vheEnabled = v }
            }

            Column {
                visible: vheHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Quality"
                    from: 0; to: 4
                    value: viperState.vheQuality
                    onValueChanged: viperState.vheQuality = value
                    displayFn: function(v) { return "Level " + (v + 1) }
                }
            }

            // 10. Reverberation
            SectionHeader {
                id: reverbHeader
                width: parent.width
                title: "Reverberation"
                checked: viperState.reverberationEnabled
                onToggled: function(v) { viperState.reverberationEnabled = v }
            }

            Column {
                visible: reverbHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Room Size"
                    from: 0; to: 10
                    value: viperState.reverberationRoomSize
                    onValueChanged: viperState.reverberationRoomSize = value
                    displayFn: function(v) { return v.toString() }
                }

                CustomSlider {
                    width: parent.width
                    title: "Width"
                    from: 0; to: 10
                    value: viperState.reverberationRoomWidth
                    onValueChanged: viperState.reverberationRoomWidth = value
                    displayFn: function(v) { return v.toString() }
                }

                CustomSlider {
                    width: parent.width
                    title: "Dampening"
                    from: 0; to: 10
                    value: viperState.reverberationRoomDampening
                    onValueChanged: viperState.reverberationRoomDampening = value
                    displayFn: function(v) { return v.toString() }
                }

                CustomSlider {
                    width: parent.width
                    title: "Wet Signal"
                    from: 0; to: 100
                    value: viperState.reverberationWetSignal
                    onValueChanged: viperState.reverberationWetSignal = value
                    displayFn: function(v) { return v + "%" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Dry Signal"
                    from: 0; to: 100
                    value: viperState.reverberationDrySignal
                    onValueChanged: viperState.reverberationDrySignal = value
                    displayFn: function(v) { return v + "%" }
                }
            }

            // 11. Dynamic System
            SectionHeader {
                id: dynSysHeader
                width: parent.width
                title: "Dynamic System"
                checked: viperState.dynamicSystemEnabled
                onToggled: function(v) { viperState.dynamicSystemEnabled = v }
            }

            Column {
                visible: dynSysHeader.expanded
                width: parent.width
                spacing: 6

                ComboBox {
                    width: parent.width
                    model: [
                        "Extreme Headphone (v2)", "High-End Headphone (v2)",
                        "Common Headphone (v2)", "Low-End Headphone (v2)",
                        "Common Earphone (v2)", "Extreme Headphone (v1)",
                        "High-End Headphone (v1)", "Common Headphone (v1)"
                    ]
                    currentIndex: viperState.dynamicSystemDevice
                    onActivated: function(idx) { viperState.dynamicSystemDevice = idx }
                }

                CustomSlider {
                    width: parent.width
                    title: "Strength"
                    from: 0; to: 100
                    value: viperState.dynamicSystemStrength
                    onValueChanged: viperState.dynamicSystemStrength = value
                    displayFn: function(v) { return v + "%" }
                }

                CustomSlider {
                    width: parent.width
                    title: "X Low Freq"
                    from: 0; to: 2400; stepSize: 5
                    value: viperState.dsXLow
                    onValueChanged: viperState.dsXLow = value
                    displayFn: function(v) { return v + " Hz" }
                }

                CustomSlider {
                    width: parent.width
                    title: "X High Freq"
                    from: 0; to: 12000; stepSize: 20
                    value: viperState.dsXHigh
                    onValueChanged: viperState.dsXHigh = value
                    displayFn: function(v) { return v + " Hz" }
                }
            }

            // 12. Auditory Protection (CURE)
            SectionHeader {
                id: cureHeader
                width: parent.width
                title: "Auditory System Protection (CURE)"
                checked: viperState.cureEnabled
                onToggled: function(v) { viperState.cureEnabled = v }
            }

            Column {
                visible: cureHeader.expanded
                width: parent.width
                spacing: 6

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Mild", "Medium", "Strong"]
                    currentIndex: viperState.cureCrossfeedStrength
                    onSelected: function(idx) { viperState.cureCrossfeedStrength = idx }
                }
            }

            // 13. Tube Simulator (6N1J)
            SectionHeader {
                id: tubeHeader
                width: parent.width
                title: "Tube Simulator (6N1J)"
                checked: viperState.tubeSimulatorEnabled
                onToggled: function(v) { viperState.tubeSimulatorEnabled = v }
            }

            // 14. AnalogX
            SectionHeader {
                id: analogXHeader
                width: parent.width
                title: "AnalogX"
                checked: viperState.analogXEnabled
                onToggled: function(v) { viperState.analogXEnabled = v }
            }

            Column {
                visible: analogXHeader.expanded
                width: parent.width
                spacing: 6

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Mild", "Medium", "Strong"]
                    currentIndex: viperState.analogXMode
                    onSelected: function(idx) { viperState.analogXMode = idx }
                }
            }

            // 15. Spectrum Extension
            SectionHeader {
                id: vseHeader
                width: parent.width
                title: "Spectrum Extension"
                checked: viperState.spectrumExtensionEnabled
                onToggled: function(v) { viperState.spectrumExtensionEnabled = v }
            }

            Column {
                visible: vseHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Bark Freq"
                    from: 2200; to: 8200; stepSize: 50
                    value: viperState.spectrumExtensionBark
                    onValueChanged: viperState.spectrumExtensionBark = value
                    displayFn: function(v) { return v + " Hz" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Exciter"
                    from: 0; to: 100
                    value: viperState.spectrumExtensionBarkReconstruct
                    onValueChanged: viperState.spectrumExtensionBarkReconstruct = value
                    displayFn: function(v) { return v + "%" }
                }
            }

            // 16. FET Compressor
            SectionHeader {
                id: compHeader
                width: parent.width
                title: "FET Compressor"
                checked: viperState.fetCompressorEnabled
                onToggled: function(v) { viperState.fetCompressorEnabled = v }
            }

            Column {
                visible: compHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Threshold"
                    from: -48; to: 0
                    value: viperState.fetCompressorThreshold
                    onValueChanged: viperState.fetCompressorThreshold = value
                    displayFn: function(v) { return v + " dB" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Ratio"
                    from: 0; to: 200
                    value: viperState.fetCompressorRatio
                    onValueChanged: viperState.fetCompressorRatio = value
                    displayFn: function(v) { return (v / 100.0).toFixed(1) + ":1" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Gain"
                    from: 0; to: 24
                    value: viperState.fetCompressorGain
                    onValueChanged: viperState.fetCompressorGain = value
                    displayFn: function(v) { return v + " dB" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Attack"
                    from: 1; to: 100
                    value: viperState.fetCompressorAttack
                    onValueChanged: viperState.fetCompressorAttack = value
                    displayFn: function(v) { return v + " ms" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Release"
                    from: 5; to: 500
                    value: viperState.fetCompressorRelease
                    onValueChanged: viperState.fetCompressorRelease = value
                    displayFn: function(v) { return v + " ms" }
                }
            }

            // 17. Multiband Compressor
            SectionHeader {
                id: mbcHeader
                width: parent.width
                title: "Multiband Compressor"
                checked: viperState.mbcEnabled
                onToggled: function(v) { viperState.mbcEnabled = v }
            }

            Column {
                visible: mbcHeader.expanded
                width: parent.width
                spacing: 6

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Sub", "Low", "Mid", "Pres", "Air"]
                    currentIndex: viperState.mbcSelectedBand
                    onSelected: function(idx) { viperState.mbcSelectedBand = idx }
                }

                Text {
                    text: {
                        var b = viperState.mbcSelectedBand
                        var ranges = ["20 - 120 Hz", "120 - 500 Hz", "500 - 4000 Hz", "4000 - 8000 Hz", "8000+ Hz"]
                        return ranges[b]
                    }
                    color: "#8E8B9E"
                    font.pixelSize: 11
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            // 18. Stereo Imager
            SectionHeader {
                id: imagerHeader
                width: parent.width
                title: "Stereo Imager"
                checked: viperState.stereoImgEnabled
                onToggled: function(v) { viperState.stereoImgEnabled = v }
            }

            Column {
                visible: imagerHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Low Width"
                    from: 0; to: 200
                    value: viperState.stereoImgLowWidth
                    onValueChanged: viperState.stereoImgLowWidth = value
                    displayFn: function(v) { return v + "%" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Mid Width"
                    from: 0; to: 200
                    value: viperState.stereoImgMidWidth
                    onValueChanged: viperState.stereoImgMidWidth = value
                    displayFn: function(v) { return v + "%" }
                }

                CustomSlider {
                    width: parent.width
                    title: "High Width"
                    from: 0; to: 200
                    value: viperState.stereoImgHighWidth
                    onValueChanged: viperState.stereoImgHighWidth = value
                    displayFn: function(v) { return v + "%" }
                }
            }

            // 19. Playback Gain Control (AGC)
            SectionHeader {
                id: agcHeader
                width: parent.width
                title: "Playback Gain Control (AGC)"
                checked: viperState.playbackGainEnabled
                onToggled: function(v) { viperState.playbackGainEnabled = v }
            }

            Column {
                visible: agcHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Strength"
                    from: 50; to: 300
                    value: viperState.playbackGainStrength
                    onValueChanged: viperState.playbackGainStrength = value
                    displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Max Gain"
                    from: 100; to: 1000
                    value: viperState.playbackGainMaxGain
                    onValueChanged: viperState.playbackGainMaxGain = value
                    displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                }
            }

            // 20. LUFS Targeting
            SectionHeader {
                id: lufsHeader
                width: parent.width
                title: "LUFS Targeting"
                checked: viperState.lufsEnabled
                onToggled: function(v) { viperState.lufsEnabled = v }
            }

            Column {
                visible: lufsHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Target"
                    from: 80; to: 240
                    value: viperState.lufsTarget
                    onValueChanged: viperState.lufsTarget = value
                    displayFn: function(v) { return (v / -10.0).toFixed(1) + " LUFS" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Max Gain"
                    from: 0; to: 120
                    value: viperState.lufsMaxGain
                    onValueChanged: viperState.lufsMaxGain = value
                    displayFn: function(v) { return (v / 10.0).toFixed(1) + " dB" }
                }

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["Slow", "Medium", "Fast"]
                    currentIndex: viperState.lufsSpeed
                    onSelected: function(idx) { viperState.lufsSpeed = idx }
                }
            }

            // 21. Psychoacoustic Bass
            SectionHeader {
                id: psychoHeader
                width: parent.width
                title: "Psychoacoustic Bass"
                checked: viperState.psychoBassEnabled
                onToggled: function(v) { viperState.psychoBassEnabled = v }
            }

            Column {
                visible: psychoHeader.expanded
                width: parent.width
                spacing: 6

                CustomSlider {
                    width: parent.width
                    title: "Cutoff"
                    from: 60; to: 150
                    value: viperState.psychoBassCutoff
                    onValueChanged: viperState.psychoBassCutoff = value
                    displayFn: function(v) { return v + " Hz" }
                }

                CustomSlider {
                    width: parent.width
                    title: "Intensity"
                    from: 0; to: 100
                    value: viperState.psychoBassIntensity
                    onValueChanged: viperState.psychoBassIntensity = value
                    displayFn: function(v) { return v + "%" }
                }

                CustomSegmentedPicker {
                    width: parent.width
                    model: ["2nd", "3rd", "4th", "5th"]
                    currentIndex: viperState.psychoBassHarmonicOrder - 2
                    onSelected: function(idx) { viperState.psychoBassHarmonicOrder = idx + 2 }
                }
            }

            // 22. Speaker Optimization (Only in Speaker mode)
            SectionHeader {
                id: spkOptHeader
                width: parent.width
                visible: viperState.fxType === 1
                title: "Speaker Optimization"
                checked: viperState.speakerCorrectionEnabled
                onToggled: function(v) { viperState.speakerCorrectionEnabled = v }
            }

            // Presets Drawer
            Rectangle {
                width: parent.width
                height: presetCol.implicitHeight + 20
                radius: 8
                color: "#13111F"
                border.color: "#252236"

                Column {
                    id: presetCol
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: "USER PRESETS"
                        color: "#8E8B9E"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        TextField {
                            id: savePresetInput
                            placeholderText: "New preset name..."
                            height: 28
                            width: parent.width - 70
                            color: "#DFD4FF"
                        }

                        Rectangle {
                            width: 62
                            height: 28
                            radius: 4
                            color: "#342F4D"
                            border.color: "#A88CFA"

                            Text {
                                anchors.centerIn: parent
                                text: "Save"
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (savePresetInput.text.trim().length > 0) {
                                        viperState.savePreset(savePresetInput.text.trim())
                                        savePresetInput.text = ""
                                    }
                                }
                            }
                        }
                    }

                    Repeater {
                        model: viperState.userPresetFiles

                        Row {
                            width: parent.width
                            spacing: 8

                            Text {
                                text: modelData.replace(".json", "")
                                color: "#DFD4FF"
                                font.pixelSize: 12
                                width: parent.width - 100
                                elide: Text.ElideRight
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Rectangle {
                                width: 44; height: 22; radius: 3; color: "#252236"
                                Text { anchors.centerIn: parent; text: "Load"; color: "#C2ABFF"; font.pixelSize: 10 }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: viperState.loadPreset(modelData)
                                }
                            }

                            Rectangle {
                                width: 44; height: 22; radius: 3; color: "#3B1D28"
                                Text { anchors.centerIn: parent; text: "Del"; color: "#FF6B8B"; font.pixelSize: 10 }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: viperState.deletePreset(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }



    // Driver Status Modal Overlay
    DriverStatusModal {
        id: statusModal
        onClosed: statusModal.visible = false
    }
}
