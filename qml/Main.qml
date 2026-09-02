import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ViPER4Linux 1.0
import "components"

ApplicationWindow {
    id: appWindow
    visible: true
    width: 860
    height: 600
    minimumWidth: 720
    minimumHeight: 480
    title: "ViPER4Linux"
    color: "#0F1013"

    property int currentCategory: viperState.currentCategory
    onCurrentCategoryChanged: {
        if (viperState.currentCategory !== currentCategory) {
            viperState.currentCategory = currentCategory
        }
    }

    Connections {
        target: viperState
        function onCurrentCategoryChanged() {
            if (appWindow.currentCategory !== viperState.currentCategory) {
                appWindow.currentCategory = viperState.currentCategory
            }
        }
    }

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

    // Top Header (Sleek, Compact, No Manual Headphone/Speaker switch)
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 50
        color: "#141519"
        border.color: "#22242D"
        border.width: 1
        z: 10

        Row {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Branding Logo
            Row {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    width: 26
                    height: 26
                    radius: 6
                    color: "#17332B"
                    border.color: "#00D2B4"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "V"
                        color: "#00D2B4"
                        font.pixelSize: 15
                        font.bold: true
                    }
                }

                Text {
                    text: "ViPER4Linux"
                    color: "#F1F3F7"
                    font.pixelSize: 15
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Driver Status Pill
            Rectangle {
                height: 24
                width: statusRow.implicitWidth + 16
                radius: 12
                color: "#1A1C22"
                border.color: "#282A34"
                anchors.verticalCenter: parent.verticalCenter

                Row {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 6; height: 6; radius: 3
                        color: viperState.isEnabled ? "#00D2B4" : "#F59E0B"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: viperState.audioServerName.indexOf("PipeWire") >= 0 ? "PipeWire 1.6.8" : "Active"
                        color: "#9AA0AD"
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

            // (i) Info Button
            Rectangle {
                width: 24
                height: 24
                radius: 12
                color: "#1F2128"
                border.color: "#2A2D37"
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: "i"
                    color: "#00D2B4"
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

            // Spacer
            Item {
                width: Math.max(10, parent.width - 450)
                height: parent.height
            }

            // Master Toggle Switch
            Row {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: viperState.isEnabled ? "MASTER ON" : "BYPASSED"
                    color: viperState.isEnabled ? "#00D2B4" : "#6B7280"
                    font.pixelSize: 11
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                CustomSwitch {
                    id: masterSwitch
                    checked: viperState.isEnabled
                    anchors.verticalCenter: parent.verticalCenter
                    onToggled: function(val) {
                        viperState.isEnabled = val
                    }
                }
            }
        }
    }

    // Main Body: 2-Column Layout (Sidebar + Content)
    Item {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        // Left Navigation Sidebar
        Rectangle {
            id: sidebar
            width: 210
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            color: "#141519"
            border.color: "#22242D"
            border.width: 1

            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                Text {
                    text: "CATEGORIES"
                    font.pixelSize: 10
                    font.bold: true
                    color: "#5C6170"
                    leftPadding: 8
                    bottomPadding: 6
                    topPadding: 4
                }

                SidebarItem {
                    title: "Master & EQ"
                    selected: appWindow.currentCategory === 0
                    activeCount: (viperState.equalizerEnabled ? 1 : 0)
                    onClicked: appWindow.currentCategory = 0
                }

                SidebarItem {
                    title: "Bass & Clarity"
                    selected: appWindow.currentCategory === 1
                    activeCount: (viperState.viperBassEnabled ? 1 : 0) + (viperState.viperBassMonoEnabled ? 1 : 0) + (viperState.psychoBassEnabled ? 1 : 0) + (viperState.viperClarityEnabled ? 1 : 0)
                    onClicked: appWindow.currentCategory = 1
                }

                SidebarItem {
                    title: "Spatial & Reverb"
                    selected: appWindow.currentCategory === 2
                    activeCount: (viperState.fieldSurroundEnabled ? 1 : 0) + (viperState.diffSurroundEnabled ? 1 : 0) + (viperState.vheEnabled ? 1 : 0) + (viperState.reverberationEnabled ? 1 : 0) + (viperState.stereoImgEnabled ? 1 : 0)
                    onClicked: appWindow.currentCategory = 2
                }

                SidebarItem {
                    title: "Impulse & Correction"
                    selected: appWindow.currentCategory === 3
                    activeCount: (viperState.convolutionEnabled ? 1 : 0) + (viperState.ddcEnabled ? 1 : 0) + (viperState.speakerCorrectionEnabled ? 1 : 0) + (viperState.cureEnabled ? 1 : 0)
                    onClicked: appWindow.currentCategory = 3
                }

                SidebarItem {
                    title: "Dynamics & Color"
                    selected: appWindow.currentCategory === 4
                    activeCount: (viperState.dynamicSystemEnabled ? 1 : 0) + (viperState.tubeSimulatorEnabled ? 1 : 0) + (viperState.analogXEnabled ? 1 : 0) + (viperState.spectrumExtensionEnabled ? 1 : 0) + (viperState.fetCompressorEnabled ? 1 : 0) + (viperState.playbackGainEnabled ? 1 : 0) + (viperState.lufsEnabled ? 1 : 0)
                    onClicked: appWindow.currentCategory = 4
                }
            }
        }

        // Right Content Area (ScrollView hosting active category's cards)
        ScrollView {
            id: contentScroll
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: sidebar.right
            anchors.right: parent.right
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Column {
                width: contentScroll.width - 24
                x: 12
                topPadding: 12
                bottomPadding: 20
                spacing: 12

                // ==========================================
                // CATEGORY 0: MASTER & EQ
                // ==========================================
                Column {
                    width: parent.width
                    spacing: 12
                    visible: appWindow.currentCategory === 0

                    // Master Output Card
                    Rectangle {
                        width: parent.width
                        height: outCol.implicitHeight + 20
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1

                        Column {
                            id: outCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "Master Output"
                                font.pixelSize: 13
                                font.bold: true
                                color: "#F1F3F7"
                            }

                            CustomSlider {
                                width: parent.width
                                title: "Output Gain"
                                from: 0; to: 200
                                value: viperState.outputVolume
                                onValueChanged: viperState.outputVolume = value
                                displayFn: function(v) {
                                    if (v === 100) return "0.0 dB"
                                    if (v <= 0) return "-∞ dB"
                                    var db = 20.0 * Math.log10(v / 100.0)
                                    return (db > 0 ? "+" : "") + db.toFixed(1) + " dB"
                                }
                            }

                            CustomSlider {
                                width: parent.width
                                title: "Output Pan"
                                from: -100; to: 100
                                value: viperState.channelPan
                                onValueChanged: viperState.channelPan = value
                                displayFn: function(v) {
                                    if (v === 0) return "Center"
                                    return (v < 0 ? "L " + Math.abs(v) : "R " + v) + "%"
                                }
                            }

                            CustomSlider {
                                width: parent.width
                                title: "Limiter Thresh"
                                from: 0; to: 100
                                value: viperState.limiter
                                onValueChanged: viperState.limiter = value
                                displayFn: function(v) {
                                    if (v === 100) return "0.0 dB"
                                    if (v <= 0) return "-∞ dB"
                                    var db = 20.0 * Math.log10(v / 100.0)
                                    return db.toFixed(1) + " dB"
                                }
                            }
                        }
                    }

                    // FIR Equalizer Card
                    Rectangle {
                        width: parent.width
                        height: eqHeader.expanded ? (eqHeader.height + eqContent.implicitHeight + 20) : eqHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1

                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: eqHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "FIR Equalizer"
                                checked: viperState.equalizerEnabled
                                onToggled: function(v) { viperState.equalizerEnabled = v }
                            }

                        AnimatedSection {
                            id: eqContent
                            anchors.top: eqHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: eqHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 8

                                    Row {
                                        width: parent.width
                                        spacing: 10

                                        CustomSegmentedPicker {
                                            width: 190
                                            height: 26
                                            model: ["10 Bands", "15 Bands", "31 Bands"]
                                            currentIndex: viperState.equalizerBandCount === 31 ? 2 : (viperState.equalizerBandCount === 15 ? 1 : 0)
                                            onSelected: function(idx) {
                                                if (idx === 0) viperState.setEqualizerBandCount(10)
                                                else if (idx === 1) viperState.setEqualizerBandCount(15)
                                                else if (idx === 2) viperState.setEqualizerBandCount(31)
                                            }
                                        }

                                        ComboBox {
                                            id: presetCombo
                                            model: viperState.equalizerPresetNames
                                            width: 130
                                            height: 26
                                            currentIndex: model ? model.indexOf(viperState.equalizerPresetName) : 0
                                            onActivated: function(index) {
                                                viperState.applyEqPreset(currentText)
                                            }
                                        }

                                        Button {
                                            text: "Flat"
                                            height: 26
                                            width: 50
                                            onClicked: viperState.resetEq()
                                        }
                                    }

                                    EqGraphItem {
                                        id: eqGraph
                                        width: parent.width
                                        height: 90
                                        bands: viperState.equalizerBands
                                        bandCount: viperState.equalizerBandCount
                                        onBandLevelChanged: function(band, level) {
                                            viperState.setEqBandLevel(band, level)
                                        }
                                    }

                                    Flickable {
                                        width: parent.width
                                        height: 145
                                        contentWidth: eqSlidersRow.width
                                        clip: true

                                        Row {
                                            id: eqSlidersRow
                                            spacing: 4
                                            Repeater {
                                                model: viperState.equalizerBandLabels.length
                                                EqBandSlider {
                                                    label: viperState.equalizerBandLabels[index]
                                                    level: viperState.equalizerBands[index]
                                                    onBandMoved: function(newLevel) {
                                                        viperState.setEqBandLevel(index, newLevel)
                                                        eqGraph.update()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                    }
                }

                // ==========================================
                // CATEGORY 1: BASS & CLARITY
                // ==========================================
                Column {
                    width: parent.width
                    spacing: 12
                    visible: appWindow.currentCategory === 1

                    // ViPER Bass Card
                    Rectangle {
                        width: parent.width
                        height: bassHeader.expanded ? (bassHeader.height + bassContent.implicitHeight + 20) : bassHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: bassHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "ViPER Bass"
                                checked: viperState.viperBassEnabled
                                onToggled: function(v) { viperState.viperBassEnabled = v }
                            }

                        AnimatedSection {
                            id: bassContent
                            anchors.top: bassHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: bassHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSegmentedPicker {
                                        width: parent.width
                                        height: 26
                                        model: ["Natural Bass", "Pure Bass+", "Subwoofer"]
                                        currentIndex: viperState.viperBassMode
                                        onSelected: function(idx) { viperState.viperBassMode = idx }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Frequency"
                                        from: 20; to: 150
                                        value: viperState.viperBassFrequency
                                        onValueChanged: viperState.viperBassFrequency = value
                                        displayFn: function(v) { return v + " Hz" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Bass Gain"
                                        from: 50; to: 500
                                        stepSize: 5
                                        value: viperState.viperBassGain
                                        onValueChanged: viperState.viperBassGain = value
                                        displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                                    }
                                }
                            }
                    }

                    // ViPER Bass Mono Card
                    Rectangle {
                        width: parent.width
                        height: bassMonoHeader.expanded ? (bassMonoHeader.height + bassMonoContent.implicitHeight + 20) : bassMonoHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: bassMonoHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "ViPER Bass Mono"
                                checked: viperState.viperBassMonoEnabled
                                onToggled: function(v) { viperState.viperBassMonoEnabled = v }
                            }

                        AnimatedSection {
                            id: bassMonoContent
                            anchors.top: bassMonoHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: bassMonoHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSegmentedPicker {
                                        width: parent.width
                                        height: 26
                                        model: ["Natural", "Pure+", "Subwoofer"]
                                        currentIndex: viperState.viperBassMonoMode
                                        onSelected: function(idx) { viperState.viperBassMonoMode = idx }
                                    }

                                    CustomSlider {
                                        width: parent.width
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
                            }
                    }

                    // Psychoacoustic Bass Card
                    Rectangle {
                        width: parent.width
                        height: psychoBassHeader.expanded ? (psychoBassHeader.height + psychoBassContent.implicitHeight + 20) : psychoBassHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: psychoBassHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Psychoacoustic Bass"
                                checked: viperState.psychoBassEnabled
                                onToggled: function(v) { viperState.psychoBassEnabled = v }
                            }

                        AnimatedSection {
                            id: psychoBassContent
                            anchors.top: psychoBassHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: psychoBassHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Cutoff"
                                        from: 30; to: 200
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

                                    CustomSlider {
                                        width: parent.width
                                        title: "Harmonic Order"
                                        from: 2; to: 8
                                        value: viperState.psychoBassHarmonicOrder
                                        onValueChanged: viperState.psychoBassHarmonicOrder = value
                                        displayFn: function(v) { return "Order " + v }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Original Level"
                                        from: 0; to: 100
                                        value: viperState.psychoBassOriginalLevel
                                        onValueChanged: viperState.psychoBassOriginalLevel = value
                                        displayFn: function(v) { return v + "%" }
                                    }
                                }
                            }
                    }

                    // ViPER Clarity Card
                    Rectangle {
                        width: parent.width
                        height: clarityHeader.expanded ? (clarityHeader.height + clarityContent.implicitHeight + 20) : clarityHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: clarityHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "ViPER Clarity"
                                checked: viperState.viperClarityEnabled
                                onToggled: function(v) { viperState.viperClarityEnabled = v }
                            }

                        AnimatedSection {
                            id: clarityContent
                            anchors.top: clarityHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: clarityHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSegmentedPicker {
                                        width: parent.width
                                        height: 26
                                        model: ["Natural", "Ozone+", "XHiFi"]
                                        currentIndex: viperState.viperClarityMode
                                        onSelected: function(idx) { viperState.viperClarityMode = idx }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Clarity Gain"
                                        from: 50; to: 450
                                        stepSize: 5
                                        value: viperState.viperClarityGain
                                        onValueChanged: viperState.viperClarityGain = value
                                        displayFn: function(v) { return (v / 100.0).toFixed(1) + "x" }
                                    }
                                }
                            }
                    }
                }

                // ==========================================
                // CATEGORY 2: SPATIAL & REVERB
                // ==========================================
                Column {
                    width: parent.width
                    spacing: 12
                    visible: appWindow.currentCategory === 2

                    // Field Surround Card
                    Rectangle {
                        width: parent.width
                        height: fieldSurroundHeader.expanded ? (fieldSurroundHeader.height + fieldSurroundContent.implicitHeight + 20) : fieldSurroundHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: fieldSurroundHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Field Surround"
                                checked: viperState.fieldSurroundEnabled
                                onToggled: function(v) { viperState.fieldSurroundEnabled = v }
                            }

                        AnimatedSection {
                            id: fieldSurroundContent
                            anchors.top: fieldSurroundHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: fieldSurroundHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Widening"
                                        from: 0; to: 100
                                        value: viperState.fieldSurroundWidening
                                        onValueChanged: viperState.fieldSurroundWidening = value
                                        displayFn: function(v) { return v + "%" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Mid Image"
                                        from: 0; to: 100
                                        value: viperState.fieldSurroundMidImage
                                        onValueChanged: viperState.fieldSurroundMidImage = value
                                        displayFn: function(v) { return v + "%" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Depth"
                                        from: 0; to: 10
                                        value: viperState.fieldSurroundDepth
                                        onValueChanged: viperState.fieldSurroundDepth = value
                                        displayFn: function(v) { return "Level " + v }
                                    }
                                }
                            }
                    }

                    // Differential Surround Card
                    Rectangle {
                        width: parent.width
                        height: diffSurroundHeader.expanded ? (diffSurroundHeader.height + diffSurroundContent.implicitHeight + 20) : diffSurroundHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: diffSurroundHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Differential Surround"
                                checked: viperState.diffSurroundEnabled
                                onToggled: function(v) { viperState.diffSurroundEnabled = v }
                            }

                        AnimatedSection {
                            id: diffSurroundContent
                            anchors.top: diffSurroundHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: diffSurroundHeader.expanded

                                Column {
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
                                }
                            }
                    }

                    // Headphone Surround+ (VHE) Card
                    Rectangle {
                        width: parent.width
                        height: vheHeader.expanded ? (vheHeader.height + vheContent.implicitHeight + 20) : vheHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: vheHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Headphone Surround+ (VHE)"
                                checked: viperState.vheEnabled
                                onToggled: function(v) { viperState.vheEnabled = v }
                            }

                        AnimatedSection {
                            id: vheContent
                            anchors.top: vheHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: vheHeader.expanded

                                Column {
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
                            }
                    }

                    // Reverberation Card
                    Rectangle {
                        width: parent.width
                        height: reverbHeader.expanded ? (reverbHeader.height + reverbContent.implicitHeight + 20) : reverbHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: reverbHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Reverberation"
                                checked: viperState.reverberationEnabled
                                onToggled: function(v) { viperState.reverberationEnabled = v }
                            }

                        AnimatedSection {
                            id: reverbContent
                            anchors.top: reverbHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: reverbHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Room Size"
                                        from: 0; to: 100
                                        value: viperState.reverberationRoomSize
                                        onValueChanged: viperState.reverberationRoomSize = value
                                        displayFn: function(v) { return v + "%" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Width"
                                        from: 0; to: 100
                                        value: viperState.reverberationRoomWidth
                                        onValueChanged: viperState.reverberationRoomWidth = value
                                        displayFn: function(v) { return v + "%" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Damp"
                                        from: 0; to: 100
                                        value: viperState.reverberationRoomDampening
                                        onValueChanged: viperState.reverberationRoomDampening = value
                                        displayFn: function(v) { return v + "%" }
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
                            }
                    }

                    // Stereo Imager Card
                    Rectangle {
                        width: parent.width
                        height: stereoImgHeader.expanded ? (stereoImgHeader.height + stereoImgContent.implicitHeight + 20) : stereoImgHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: stereoImgHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Stereo Imager"
                                checked: viperState.stereoImgEnabled
                                onToggled: function(v) { viperState.stereoImgEnabled = v }
                            }

                        AnimatedSection {
                            id: stereoImgContent
                            anchors.top: stereoImgHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: stereoImgHeader.expanded

                                Column {
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

                                    CustomSlider {
                                        width: parent.width
                                        title: "Low Cross"
                                        from: 100; to: 1000
                                        stepSize: 10
                                        value: viperState.stereoImgLowCrossover
                                        onValueChanged: viperState.stereoImgLowCrossover = value
                                        displayFn: function(v) { return v + " Hz" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "High Cross"
                                        from: 1000; to: 10000
                                        stepSize: 100
                                        value: viperState.stereoImgHighCrossover
                                        onValueChanged: viperState.stereoImgHighCrossover = value
                                        displayFn: function(v) { return v + " Hz" }
                                    }
                                }
                            }
                    }
                }

                // ==========================================
                // CATEGORY 3: IMPULSE & CORRECTION
                // ==========================================
                Column {
                    width: parent.width
                    spacing: 12
                    visible: appWindow.currentCategory === 3

                    // Convolver Card
                    Rectangle {
                        width: parent.width
                        height: convolverHeader.expanded ? (convolverHeader.height + convolverContent.implicitHeight + 20) : convolverHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: convolverHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Convolver"
                                checked: viperState.convolutionEnabled
                                onToggled: function(v) { viperState.convolutionEnabled = v }
                            }

                        AnimatedSection {
                            id: convolverContent
                            anchors.top: convolverHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: convolverHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    Row {
                                        width: parent.width
                                        spacing: 8

                                        Rectangle {
                                            width: parent.width - 88
                                            height: 28
                                            radius: 4
                                            color: "#141519"
                                            border.color: "#282A33"

                                            Text {
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                text: viperState.convolutionKernelPath.length > 0 ? viperState.convolutionKernelPath.split("/").pop() : "No kernel loaded"
                                                color: viperState.convolutionKernelPath.length > 0 ? "#00D2B4" : "#6B7280"
                                                font.pixelSize: 11
                                                elide: Text.ElideMiddle
                                            }
                                        }

                                        Rectangle {
                                            width: 80
                                            height: 28
                                            radius: 4
                                            color: "#1B2F2A"
                                            border.color: "#00D2B4"

                                            Text {
                                                anchors.centerIn: parent
                                                text: "Load IR"
                                                color: "#00D2B4"
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
                            }
                    }

                    // ViPER-DDC Card
                    Rectangle {
                        width: parent.width
                        height: ddcHeader.expanded ? (ddcHeader.height + ddcContent.implicitHeight + 20) : ddcHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: ddcHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "ViPER-DDC"
                                checked: viperState.ddcEnabled
                                onToggled: function(v) { viperState.ddcEnabled = v }
                            }

                        AnimatedSection {
                            id: ddcContent
                            anchors.top: ddcHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: ddcHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    Row {
                                        width: parent.width
                                        spacing: 8

                                        Rectangle {
                                            width: parent.width - 88
                                            height: 28
                                            radius: 4
                                            color: "#141519"
                                            border.color: "#282A33"

                                            Text {
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                text: viperState.ddcFilePath.length > 0 ? viperState.ddcFilePath.split("/").pop() : "No .vdc profile loaded"
                                                color: viperState.ddcFilePath.length > 0 ? "#00D2B4" : "#6B7280"
                                                font.pixelSize: 11
                                                elide: Text.ElideMiddle
                                            }
                                        }

                                        Rectangle {
                                            width: 80
                                            height: 28
                                            radius: 4
                                            color: "#1B2F2A"
                                            border.color: "#00D2B4"

                                            Text {
                                                anchors.centerIn: parent
                                                text: "Select VDC"
                                                color: "#00D2B4"
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
                            }
                    }

                    // Speaker Correction Card
                    Rectangle {
                        width: parent.width
                        height: spkCorrHeader.expanded ? (spkCorrHeader.height + spkCorrContent.implicitHeight + 20) : spkCorrHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: spkCorrHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Speaker Correction"
                                checked: viperState.speakerCorrectionEnabled
                                onToggled: function(v) { viperState.speakerCorrectionEnabled = v }
                            }

                        AnimatedSection {
                            id: spkCorrContent
                            anchors.top: spkCorrHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: spkCorrHeader.expanded

                                Text {
                                    text: "Enables hardware speaker acoustic frequency correction."
                                    font.pixelSize: 12
                                    color: "#9AA0AD"
                                }
                            }
                    }

                    // Cure Card
                    Rectangle {
                        width: parent.width
                        height: cureHeader.expanded ? (cureHeader.height + cureContent.implicitHeight + 20) : cureHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: cureHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Cure (Hearing Protection)"
                                checked: viperState.cureEnabled
                                onToggled: function(v) { viperState.cureEnabled = v }
                            }

                        AnimatedSection {
                            id: cureContent
                            anchors.top: cureHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: cureHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Level"
                                        from: 0; to: 2
                                        value: viperState.cureCrossfeedStrength
                                        onValueChanged: viperState.cureCrossfeedStrength = value
                                        displayFn: function(v) { return ["Slight", "Moderate", "Extreme"][v] || "" }
                                    }
                                }
                            }
                    }
                }

                // ==========================================
                // CATEGORY 4: DYNAMICS & COLOR
                // ==========================================
                Column {
                    width: parent.width
                    spacing: 12
                    visible: appWindow.currentCategory === 4

                    // Dynamic System Card
                    Rectangle {
                        width: parent.width
                        height: dsHeader.expanded ? (dsHeader.height + dsContent.implicitHeight + 20) : dsHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: dsHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Dynamic System"
                                checked: viperState.dynamicSystemEnabled
                                onToggled: function(v) { viperState.dynamicSystemEnabled = v }
                            }

                        AnimatedSection {
                            id: dsContent
                            anchors.top: dsHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: dsHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSegmentedPicker {
                                        width: parent.width
                                        height: 26
                                        model: ["Headphones", "Earphones", "High-End", "In-Ear"]
                                        currentIndex: viperState.dynamicSystemDevice
                                        onSelected: function(idx) { viperState.dynamicSystemDevice = idx }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Dynamic Bass"
                                        from: 0; to: 100
                                        value: viperState.dynamicSystemStrength
                                        onValueChanged: viperState.dynamicSystemStrength = value
                                        displayFn: function(v) { return v + "%" }
                                    }
                                }
                            }
                    }

                    // Tube Simulator Card
                    Rectangle {
                        width: parent.width
                        height: tubeHeader.expanded ? (tubeHeader.height + tubeContent.implicitHeight + 20) : tubeHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: tubeHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Tube Simulator (6J1 Vacuum Tube)"
                                checked: viperState.tubeSimulatorEnabled
                                onToggled: function(v) { viperState.tubeSimulatorEnabled = v }
                            }

                        AnimatedSection {
                            id: tubeContent
                            anchors.top: tubeHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: tubeHeader.expanded

                                Text {
                                    text: "Simulates warm harmonic vacuum tube characteristics (6J1 class-A)."
                                    font.pixelSize: 12
                                    color: "#9AA0AD"
                                }
                            }
                    }

                    // AnalogX Card
                    Rectangle {
                        width: parent.width
                        height: analogXHeader.expanded ? (analogXHeader.height + analogXContent.implicitHeight + 20) : analogXHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: analogXHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "AnalogX (Harmonic Enhancement)"
                                checked: viperState.analogXEnabled
                                onToggled: function(v) { viperState.analogXEnabled = v }
                            }

                        AnimatedSection {
                            id: analogXContent
                            anchors.top: analogXHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: analogXHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSegmentedPicker {
                                        width: parent.width
                                        height: 26
                                        model: ["Subtle", "Moderate", "Heavy"]
                                        currentIndex: viperState.analogXMode
                                        onSelected: function(idx) { viperState.analogXMode = idx }
                                    }
                                }
                            }
                    }

                    // Spectrum Extension Card
                    Rectangle {
                        width: parent.width
                        height: specExtHeader.expanded ? (specExtHeader.height + specExtContent.implicitHeight + 20) : specExtHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: specExtHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Spectrum Extension"
                                checked: viperState.spectrumExtensionEnabled
                                onToggled: function(v) { viperState.spectrumExtensionEnabled = v }
                            }

                        AnimatedSection {
                            id: specExtContent
                            anchors.top: specExtHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: specExtHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Exciter"
                                        from: 0; to: 100
                                        value: viperState.spectrumExtensionBark
                                        onValueChanged: viperState.spectrumExtensionBark = value
                                        displayFn: function(v) { return (v / 100.0).toFixed(2) }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Ref. Frequency"
                                        from: 0; to: 100
                                        value: viperState.spectrumExtensionBarkReconstruct
                                        onValueChanged: viperState.spectrumExtensionBarkReconstruct = value
                                        displayFn: function(v) { return v + "%" }
                                    }
                                }
                            }
                    }

                    // FET Compressor Card
                    Rectangle {
                        width: parent.width
                        height: fetHeader.expanded ? (fetHeader.height + fetContent.implicitHeight + 20) : fetHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: fetHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "FET Compressor"
                                checked: viperState.fetCompressorEnabled
                                onToggled: function(v) { viperState.fetCompressorEnabled = v }
                            }

                        AnimatedSection {
                            id: fetContent
                            anchors.top: fetHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: fetHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Threshold"
                                        from: -60; to: 0
                                        value: viperState.fetCompressorThreshold
                                        onValueChanged: viperState.fetCompressorThreshold = value
                                        displayFn: function(v) { return v + " dB" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Ratio"
                                        from: 1; to: 20
                                        value: viperState.fetCompressorRatio
                                        onValueChanged: viperState.fetCompressorRatio = value
                                        displayFn: function(v) { return v + ":1" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Knee Width"
                                        from: 0; to: 24
                                        value: viperState.fetCompressorKnee
                                        onValueChanged: viperState.fetCompressorKnee = value
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
                                        from: 10; to: 1000
                                        stepSize: 10
                                        value: viperState.fetCompressorRelease
                                        onValueChanged: viperState.fetCompressorRelease = value
                                        displayFn: function(v) { return v + " ms" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Gain Makeup"
                                        from: -20; to: 20
                                        value: viperState.fetCompressorGain
                                        onValueChanged: viperState.fetCompressorGain = value
                                        displayFn: function(v) { return (v > 0 ? "+" : "") + v + " dB" }
                                    }

                                    Row {
                                        spacing: 20
                                        CustomSwitch {
                                            title: "Auto Gain"
                                            checked: viperState.fetCompressorAutoGain
                                            onToggled: function(v) { viperState.fetCompressorAutoGain = v }
                                        }
                                        CustomSwitch {
                                            title: "No Clip"
                                            checked: viperState.fetCompressorNoClip
                                            onToggled: function(v) { viperState.fetCompressorNoClip = v }
                                        }
                                    }
                                }
                            }
                    }

                    // Playback Gain Control Card
                    Rectangle {
                        width: parent.width
                        height: playbackGainHeader.expanded ? (playbackGainHeader.height + playbackGainContent.implicitHeight + 20) : playbackGainHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: playbackGainHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Playback Gain Control"
                                checked: viperState.playbackGainEnabled
                                onToggled: function(v) { viperState.playbackGainEnabled = v }
                            }

                        AnimatedSection {
                            id: playbackGainContent
                            anchors.top: playbackGainHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: playbackGainHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSlider {
                                        width: parent.width
                                        title: "Max Gain"
                                        from: 1; to: 10
                                        value: viperState.playbackGainMaxGain
                                        onValueChanged: viperState.playbackGainMaxGain = value
                                        displayFn: function(v) { return v + "x" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Strength"
                                        from: 0; to: 100
                                        value: viperState.playbackGainStrength
                                        onValueChanged: viperState.playbackGainStrength = value
                                        displayFn: function(v) { return v + "%" }
                                    }

                                    CustomSlider {
                                        width: parent.width
                                        title: "Max Thresh"
                                        from: 0; to: 100
                                        value: viperState.playbackGainOutputThreshold
                                        onValueChanged: viperState.playbackGainOutputThreshold = value
                                        displayFn: function(v) { return (v / 10.0).toFixed(1) + " dB" }
                                    }
                                }
                            }
                    }

                    // Auditory System Protection (LUFS) Card
                    Rectangle {
                        width: parent.width
                        height: lufsHeader.expanded ? (lufsHeader.height + lufsContent.implicitHeight + 20) : lufsHeader.height
                        radius: 8
                        color: "#18191E"
                        border.color: "#252731"
                        border.width: 1
                        clip: true
                        Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }


                        SectionHeader {
                            id: lufsHeader
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                                title: "Auditory System Protection (LUFS)"
                                checked: viperState.lufsEnabled
                                onToggled: function(v) { viperState.lufsEnabled = v }
                            }

                        AnimatedSection {
                            id: lufsContent
                            anchors.top: lufsHeader.bottom
                            anchors.topMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            expanded: lufsHeader.expanded

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    CustomSegmentedPicker {
                                        width: parent.width
                                        height: 26
                                        model: ["Fast", "Medium", "Slow"]
                                        currentIndex: viperState.lufsSpeed
                                        onSelected: function(idx) { viperState.lufsSpeed = idx }
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
