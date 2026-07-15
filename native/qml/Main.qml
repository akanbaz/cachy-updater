import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

Kirigami.ApplicationWindow {
    id: root

    title: "Cachy Updater"
    width: 1024
    height: 760
    minimumWidth: 900
    minimumHeight: 600
    color: Theme.bg

    property int currentTab: StartTab
    readonly property var tabs: [
        { name: "Updates", icon: "org.cachyos.updater-tray",
          subtitle: "Repo, AUR, and Flatpak \u2014 with clear change summaries." },
        { name: "News", icon: "news-subscribe",
          subtitle: "Arch and CachyOS announcements." },
        { name: "Cleanup", icon: "edit-clear-all",
          subtitle: "Orphans, cache, kernels, Flatpak, and AUR maintenance." },
        { name: "Firmware", icon: "cpu",
          subtitle: "Device firmware via fwupd." },
        { name: "History", icon: "view-list-details",
          subtitle: "Recent apply and maintenance actions." },
        { name: "Settings", icon: "configure",
          subtitle: "Tray, sources, holds, and performance options." }
    ]

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    property bool newsLoaded: false
    property bool cleanupLoaded: false
    property bool firmwareLoaded: false

    function loadTab() {
        if (currentTab === 1 && !newsLoaded && Settings.enableNews && !Settings.offlineMode) {
            newsLoaded = true; News.refresh()
        } else if (currentTab === 2 && !cleanupLoaded) {
            cleanupLoaded = true; Maintain.scan()
        } else if (currentTab === 3 && !firmwareLoaded) {
            firmwareLoaded = true; Firmware.scan()
        }
    }

    onCurrentTabChanged: loadTab()
    Component.onCompleted: loadTab()

    function statusColor() {
        switch (Updater.statusState) {
        case "ready": return Theme.cyan
        case "uptodate": case "done": return Theme.ok
        case "error": return Theme.errorText
        case "checking": case "applying": return Theme.textDim
        default: return Theme.textMuted
        }
    }

    Shortcut { sequence: "R"; onActivated: if (!Updater.busy) Updater.check() }
    Shortcut {
        sequence: "Ctrl+A"
        onActivated: {
            // Don't hijack select-all while typing in a text field.
            const f = root.activeFocusItem
            if (f && (f instanceof TextInput || f instanceof TextEdit
                      || (f.text !== undefined && f.cursorPosition !== undefined)))
                return
            Updater.setAllSelected(true)
        }
    }
    Shortcut { sequence: "Ctrl+Return"; onActivated: {
        if (!Updater.busy && Updater.selectedCount > 0) {
            confirmDialog.applySource = ""
            confirmDialog.plannedCmds = Updater.plannedCommands()
            confirmDialog.open()
        }
    } }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            const f = root.activeFocusItem
            if (f && (f instanceof TextInput || f instanceof TextEdit
                      || (f.text !== undefined && f.cursorPosition !== undefined))) {
                root.forceActiveFocus()
                return
            }
            // Guard against interrupting a running privileged transaction.
            if (Updater.busy || Maintain.busy || Firmware.busy) {
                cancelDialog.open()
                return
            }
            Updater.cancel()
            Maintain.cancel()
            Firmware.cancel()
        }
    }

    pageStack.initialPage: Kirigami.Page {
        padding: 0

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                color: Theme.bg
                implicitHeight: headerCol.implicitHeight + Theme.spacing

                ColumnLayout {
                    id: headerCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.rightMargin: Theme.spacingLarge
                    spacing: 2

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing

                        Kirigami.Icon {
                            source: root.tabs[root.currentTab].icon
                            implicitWidth: Theme.iconLarge
                            implicitHeight: Theme.iconLarge
                            color: Theme.cyan
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 4
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSmall

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: root.tabs[root.currentTab].name
                                    color: Theme.text
                                    font.family: Theme.sansFamily
                                    font.pixelSize: 26
                                    font.weight: Font.Bold
                                }

                                RowLayout {
                                    visible: root.currentTab === 0
                                    spacing: 6

                                    Item {
                                        implicitWidth: 18
                                        implicitHeight: 18

                                        Rectangle {
                                            anchors.centerIn: parent
                                            visible: !Updater.busy
                                            width: 8
                                            height: 8
                                            radius: 4
                                            color: root.statusColor()
                                        }

                                        QQC2.BusyIndicator {
                                            anchors.centerIn: parent
                                            running: Updater.busy
                                            visible: Updater.busy
                                            implicitWidth: 18
                                            implicitHeight: 18
                                        }
                                    }

                                    QQC2.Label {
                                        text: Updater.statusText
                                        color: Theme.cyan
                                        font.family: Theme.sansFamily
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: root.tabs[root.currentTab].subtitle
                                color: Theme.textMuted
                                font.family: Theme.sansFamily
                                font.pixelSize: 13
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                color: Theme.bg
                implicitHeight: 46

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.border
                }

                Flickable {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLarge
                    contentWidth: tabRow.width
                    clip: true

                    RowLayout {
                        id: tabRow
                        spacing: Theme.spacingLarge

                        Repeater {
                            model: root.tabs
                            delegate: QQC2.AbstractButton {
                                id: tabButton
                                required property int index
                                required property var modelData
                                readonly property bool active: root.currentTab === index
                                implicitHeight: 40
                                implicitWidth: tabContent.implicitWidth
                                onClicked: root.currentTab = index

                                contentItem: RowLayout {
                                    id: tabContent
                                    spacing: 4

                                    Kirigami.Icon {
                                        source: modelData.icon
                                        implicitWidth: 16
                                        implicitHeight: 16
                                        color: tabButton.active ? Theme.cyan : Theme.textMuted
                                    }
                                    QQC2.Label {
                                        text: modelData.name
                                        color: tabButton.active ? Theme.text : Theme.textMuted
                                        font.family: Theme.sansFamily
                                        font.pixelSize: 14
                                        font.weight: tabButton.active ? Font.DemiBold : Font.Normal
                                    }
                                    Rectangle {
                                        visible: tabButton.index === 0
                                        radius: 9
                                        width: 28
                                        implicitHeight: 18
                                        color: tabButton.active ? Theme.cyan : Theme.surfaceHover
                                        opacity: Updater.packageCount > 0 ? 1.0 : 0.0
                                        QQC2.Label {
                                            id: countLabel
                                            anchors.centerIn: parent
                                            text: Updater.packageCount > 0 ? Updater.packageCount : "0"
                                            color: tabButton.active ? Theme.cyanInk : Theme.textDim
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                        }
                                    }
                                }

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 2
                                    color: Theme.cyan
                                    visible: tabButton.active
                                }
                            }
                        }
                    }
                }
            }

            Item {
                visible: root.currentTab === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0

                UpdatesPage {
                    id: updatesPageTab
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: updatesBottomChrome.top
                    anchors.topMargin: Theme.spacingSmall
                    anchors.leftMargin: Theme.spacingSmall
                    anchors.rightMargin: Theme.spacingSmall
                }

                ColumnLayout {
                    id: updatesBottomChrome
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: parent.width
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        color: Theme.deepBg
                        visible: Updater.busy
                        implicitHeight: visible ? progressRow.implicitHeight + Theme.spacingSmall : 0

                        RowLayout {
                            id: progressRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: Theme.spacing
                            anchors.rightMargin: Theme.spacing
                            spacing: Theme.spacingSmall

                            QQC2.Label {
                                text: Updater.statusState === "checking" ? "Checking…" : Updater.stage
                                color: Theme.cyan
                                font.family: Theme.sansFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                Layout.minimumWidth: 80
                            }
                            QQC2.ProgressBar {
                                Layout.fillWidth: true
                                // Check has no meaningful fraction; show it as motion.
                                indeterminate: Updater.progress <= 0
                                from: 0; to: 1
                                value: Updater.progress
                            }
                            QQC2.Label {
                                visible: Updater.progress > 0
                                text: Math.round(Updater.progress * 100) + "%"
                                color: Theme.textMuted
                                font.family: Theme.monoFamily
                                font.pixelSize: 12
                                Layout.minimumWidth: 38
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    TerminalPanel {
                        controller: Updater
                        radius: 0
                        edgeMargin: Theme.spacingSmall
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Rectangle {
                        Layout.fillWidth: true
                        color: Theme.deepBg
                        implicitHeight: footerRow.implicitHeight + Theme.spacingSmall

                        RowLayout {
                            id: footerRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: Theme.spacing
                            anchors.rightMargin: Theme.spacing
                            spacing: Theme.spacingSmall

                            QQC2.Button {
                                text: "Refresh"
                                icon.name: "view-refresh"
                                enabled: !Updater.busy
                                onClicked: Updater.check()
                                QQC2.ToolTip.text: "Refresh (R)"
                                QQC2.ToolTip.visible: hovered
                            }

                            QQC2.Button {
                                text: "Cancel"
                                icon.name: "process-stop"
                                visible: Updater.busy
                                onClicked: cancelDialog.open()
                                QQC2.ToolTip.text: "Stop the current operation (Esc)"
                                QQC2.ToolTip.visible: hovered
                            }

                            QQC2.Label {
                                text: "v" + AppVersion
                                      + (Updater.lastChecked.length > 0 ? "   \u00b7   last checked " + Updater.lastChecked : "")
                                color: Theme.textFaint
                                font.family: Theme.monoFamily
                                font.pixelSize: 12
                            }

                            Item { Layout.fillWidth: true }

                            QQC2.Label {
                                Layout.minimumWidth: 72
                                horizontalAlignment: Text.AlignRight
                                text: Updater.selectedCount + " selected"
                                color: Theme.textMuted
                                font.pixelSize: 12
                            }

                            QQC2.ToolButton {
                                icon.name: "overflow-menu"
                                flat: true
                                enabled: !Updater.busy && Updater.selectedCount > 0
                                onClicked: moreMenu.open()
                                QQC2.Menu {
                                    id: moreMenu
                                    y: -height
                                    QQC2.MenuItem { text: "Dry Run"; onTriggered: Updater.dryRun() }
                                    QQC2.MenuItem { text: "Download Only"; onTriggered: Updater.downloadOnly() }
                                }
                            }

                            QQC2.Button {
                                id: applyButton
                                Layout.minimumWidth: 168
                                text: "Apply " + Updater.selectedCount + " update" + (Updater.selectedCount === 1 ? "" : "s")
                                enabled: !Updater.busy && Updater.selectedCount > 0 && !Updater.archNewsBlocked
                                onClicked: {
                                    confirmDialog.applySource = ""
                                    confirmDialog.plannedCmds = Updater.plannedCommands()
                                    confirmDialog.open()
                                }
                                QQC2.ToolTip.text: "Apply selected updates (Ctrl+Enter)"
                                QQC2.ToolTip.visible: hovered

                                contentItem: QQC2.Label {
                                    text: applyButton.text
                                    color: applyButton.enabled ? Theme.cyanInk : Theme.textFaint
                                    font.weight: Font.DemiBold
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: Theme.spacing
                                    rightPadding: Theme.spacing
                                }
                                background: Rectangle {
                                    radius: Theme.radiusSmall
                                    implicitHeight: 32
                                    implicitWidth: Math.max(130, applyButton.contentItem.implicitWidth)
                                    color: !applyButton.enabled ? Theme.surfaceHover
                                         : applyButton.pressed ? Theme.cyanPressed
                                         : applyButton.hovered ? Theme.cyanHover : Theme.cyan
                                }
                            }
                        }
                    }
                }
            }

            Item {
                visible: root.currentTab === 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0

                CleanupPage {
                    id: cleanupPageTab
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: cleanupBottomChrome.top
                    anchors.topMargin: Theme.spacingSmall
                    anchors.leftMargin: Theme.spacingSmall
                    anchors.rightMargin: Theme.spacingSmall
                }

                ColumnLayout {
                    id: cleanupBottomChrome
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: parent.width
                    spacing: 0

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    TerminalPanel {
                        controller: Maintain
                        radius: 0
                        edgeMargin: Theme.spacingSmall
                    }
                }
            }

            Item {
                visible: root.currentTab === 1 || root.currentTab === 3
                          || root.currentTab === 4 || root.currentTab === 5
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSmall
                    currentIndex: root.currentTab === 1 ? 0
                                  : root.currentTab === 3 ? 1
                                  : root.currentTab === 4 ? 2
                                  : 3

                    NewsPage {}
                    FirmwarePage {}
                    HistoryPage {}
                    SettingsPage {}
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: confirmDialog
        title: applySource === "" ? "Apply updates?"
             : applySource === "aur" ? "Update AUR packages?"
             : applySource === "flatpak" ? "Update Flatpaks?"
             : "Update repo packages?"
        preferredWidth: Kirigami.Units.gridUnit * 30
        standardButtons: Kirigami.Dialog.NoButton
        property string plannedCmds: ""
        // "" = apply everything selected; otherwise a single source (repo/aur/flatpak).
        property string applySource: ""

        onOpened: plannedCmds = applySource === ""
                  ? Updater.plannedCommands()
                  : Updater.sourceCommandFor(applySource)
        onClosed: applySource = ""

        customFooterActions: [
            Kirigami.Action { text: "Cancel"; onTriggered: confirmDialog.close() },
            Kirigami.Action {
                text: "Apply"
                onTriggered: {
                    const src = confirmDialog.applySource
                    confirmDialog.close()
                    if (src === "")
                        Updater.apply()
                    else
                        Updater.applySource(src)
                }
            }
        ]

        ColumnLayout {
            width: parent ? parent.width : implicitWidth
            spacing: Theme.spacingSmall

            QQC2.Label {
                Layout.fillWidth: true
                text: "The following commands will run:"
                color: Theme.textDim
                font.family: Theme.sansFamily
            }

            QQC2.TextArea {
                id: cmdPreview
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(56, Math.min(180, contentHeight + Theme.spacing))
                readOnly: true
                text: confirmDialog.plannedCmds.length > 0
                      ? confirmDialog.plannedCmds
                      : "No commands could be built for the current selection."
                color: Theme.cyan
                font.family: Theme.monoFamily
                font.pixelSize: 12
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                background: Rectangle {
                    color: Theme.deepBg
                    radius: Theme.radiusSmall
                    border.width: 1
                    border.color: Theme.border
                }
            }

            Banner {
                Layout.fillWidth: true
                visible: confirmDialog.applySource !== "flatpak" && Updater.partialUpgradeWarning
                text: "Partial upgrade: held repo packages will be skipped. This can fail "
                      + "on version-locked packages (gcc, glibc, pipewire…)."
                severity: "warn"
                closable: false
            }

            Banner {
                Layout.fillWidth: true
                visible: Updater.rebootRequired
                text: "A reboot will be required afterward."
                severity: "info"
                closable: false
            }
        }
    }

    Kirigami.PromptDialog {
        id: rebootDialog
        title: "Reboot required"
        subtitle: "Important system packages were updated."
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action { text: "Later"; onTriggered: rebootDialog.close() },
            Kirigami.Action { text: "Reboot now"; onTriggered: { rebootDialog.close(); Updater.reboot() } }
        ]
    }

    Kirigami.PromptDialog {
        id: cancelDialog
        title: "Stop the current operation?"
        subtitle: "A package operation is running. Interrupting it partway can "
                  + "leave the transaction incomplete."
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action { text: "Keep running"; onTriggered: cancelDialog.close() },
            Kirigami.Action {
                text: "Stop"
                onTriggered: {
                    cancelDialog.close()
                    Updater.cancel()
                    Maintain.cancel()
                    Firmware.cancel()
                }
            }
        ]
    }

    Connections {
        target: Updater
        function onApplyFinished(ok) {
            if (ok && Updater.rebootRequired)
                rebootDialog.open()
        }
        function onSourceApplyRequested(source) {
            confirmDialog.applySource = source
            confirmDialog.open()
        }
    }
}
