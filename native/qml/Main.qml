import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "pages"
import "components"

Kirigami.ApplicationWindow {
    id: root

    title: "CachyOS Updater"
    width: 1024
    height: 760
    minimumWidth: 900
    minimumHeight: 600
    color: Theme.bg

    property int currentTab: StartTab
    readonly property var tabs: [
        { name: "Updates", subtitle: "Repo, AUR, and Flatpak \u2014 with clear change summaries." },
        { name: "News", subtitle: "Arch and CachyOS announcements." },
        { name: "Cleanup", subtitle: "Remove orphaned packages and clear the package cache." }
    ]

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    property bool newsLoaded: false
    property bool cleanupLoaded: false

    function loadTab() {
        if (currentTab === 1 && !newsLoaded) { newsLoaded = true; News.refresh() }
        else if (currentTab === 2 && !cleanupLoaded) { cleanupLoaded = true; Maintain.scan() }
    }

    onCurrentTabChanged: loadTab()
    Component.onCompleted: { Updater.check(); loadTab() }

    function statusColor() {
        switch (Updater.statusState) {
        case "ready": return Theme.cyan
        case "uptodate": case "done": return Theme.ok
        case "error": return Theme.errorText
        case "checking": case "applying": return Theme.textDim
        default: return Theme.textMuted
        }
    }

    pageStack.initialPage: Kirigami.Page {
        padding: 0

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ---- Header ------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                color: Theme.bg
                implicitHeight: headerRow.implicitHeight + Theme.spacingLarge * 1.5

                RowLayout {
                    id: headerRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.rightMargin: Theme.spacingLarge
                    spacing: Theme.spacing

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        QQC2.Label {
                            text: root.tabs[root.currentTab].name
                            color: Theme.text
                            font.family: Theme.sansFamily
                            font.pixelSize: 26
                            font.weight: Font.Bold
                        }
                        QQC2.Label {
                            text: root.tabs[root.currentTab].subtitle
                            color: Theme.textMuted
                            font.family: Theme.sansFamily
                            font.pixelSize: 13
                        }
                    }

                    Rectangle {
                        visible: root.currentTab === 0
                        width: 8; height: 8; radius: 4
                        color: root.statusColor()
                        opacity: Updater.busy ? 0.5 : 1.0
                        Layout.alignment: Qt.AlignVCenter
                    }
                    QQC2.Label {
                        visible: root.currentTab === 0
                        text: Updater.statusText
                        color: Theme.cyan
                        font.family: Theme.sansFamily
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        Layout.alignment: Qt.AlignVCenter
                    }
                    QQC2.BusyIndicator {
                        visible: Updater.busy
                        running: Updater.busy
                        implicitWidth: 18
                        implicitHeight: 18
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            // ---- Nav tabs ----------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                color: Theme.bg
                implicitHeight: 46

                // Full-width baseline the active underline sits on.
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.border
                }

                RowLayout {
                    id: tabRow
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: Theme.spacingLarge
                    spacing: Theme.spacingLarge

                    Repeater {
                        model: root.tabs
                        delegate: QQC2.AbstractButton {
                            id: tabButton
                            required property int index
                            required property var modelData
                            readonly property bool active: root.currentTab === index
                            hoverEnabled: true
                            implicitHeight: 40
                            implicitWidth: tabContent.implicitWidth
                            onClicked: root.currentTab = index

                            contentItem: RowLayout {
                                id: tabContent
                                spacing: Theme.spacingSmall

                                QQC2.Label {
                                    text: modelData.name
                                    color: tabButton.active ? Theme.text
                                         : tabButton.hovered ? Theme.textDim : Theme.textMuted
                                    font.family: Theme.sansFamily
                                    font.pixelSize: 14
                                    font.weight: tabButton.active ? Font.DemiBold : Font.Normal
                                }

                                // Count badge (Updates tab only).
                                Rectangle {
                                    visible: tabButton.index === 0 && Updater.packageCount > 0
                                    radius: height / 2
                                    color: tabButton.active ? Theme.cyan : Theme.surfaceHover
                                    implicitHeight: 18
                                    implicitWidth: Math.max(18, countLabel.implicitWidth + Theme.spacingSmall)
                                    QQC2.Label {
                                        id: countLabel
                                        anchors.centerIn: parent
                                        text: Updater.packageCount
                                        color: tabButton.active ? Theme.cyanInk : Theme.textDim
                                        font.family: Theme.sansFamily
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            // Active underline indicator.
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 2
                                radius: 1
                                color: Theme.cyan
                                visible: tabButton.active
                            }
                        }
                    }
                }
            }

            // ---- Content -----------------------------------------------
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Theme.spacingLarge
                currentIndex: root.currentTab

                UpdatesPage {}
                NewsPage {}
                CleanupPage {}
            }

            // ---- Footer actions (Updates only) -------------------------
            Rectangle {
                Layout.fillWidth: true
                visible: root.currentTab === 0
                color: Theme.deepBg
                implicitHeight: footerRow.implicitHeight + Theme.spacing

                RowLayout {
                    id: footerRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.rightMargin: Theme.spacingLarge
                    spacing: Theme.spacingSmall

                    QQC2.Button {
                        text: "Refresh"
                        icon.name: "view-refresh"
                        enabled: !Updater.busy
                        onClicked: Updater.check()
                    }

                    QQC2.Label {
                        text: "v" + AppVersion
                              + (Updater.lastChecked.length > 0
                                 ? "   \u00b7   last checked " + Updater.lastChecked : "")
                        color: Theme.textFaint
                        font.family: Theme.monoFamily
                        font.pixelSize: 12
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.Label {
                        text: Updater.selectedCount + " selected"
                        color: Theme.textMuted
                        font.family: Theme.sansFamily
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
                            QQC2.MenuItem {
                                text: "Dry Run"
                                icon.name: "system-run"
                                onTriggered: Updater.dryRun()
                            }
                            QQC2.MenuItem {
                                text: "Download Only"
                                icon.name: "download"
                                onTriggered: Updater.downloadOnly()
                            }
                        }
                    }

                    QQC2.Button {
                        id: applyButton
                        text: "Apply " + Updater.selectedCount + " update" + (Updater.selectedCount === 1 ? "" : "s")
                        enabled: !Updater.busy && Updater.selectedCount > 0
                        onClicked: confirmDialog.open()

                        contentItem: QQC2.Label {
                            id: applyLabel
                            text: applyButton.text
                            color: applyButton.enabled ? Theme.cyanInk : Theme.textFaint
                            font.family: Theme.sansFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: Theme.spacing
                            rightPadding: Theme.spacing
                        }
                        background: Rectangle {
                            radius: Theme.radiusSmall
                            implicitHeight: 32
                            implicitWidth: Math.max(130, applyLabel.implicitWidth)
                            color: !applyButton.enabled ? Theme.surfaceHover
                                 : applyButton.pressed ? Theme.cyanPressed
                                 : applyButton.hovered ? Theme.cyanHover
                                 : Theme.cyan
                        }
                    }
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: confirmDialog
        title: "Apply updates?"
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action {
                text: "Cancel"
                icon.name: "dialog-cancel"
                onTriggered: confirmDialog.close()
            },
            Kirigami.Action {
                text: "Apply"
                icon.name: "dialog-ok-apply"
                onTriggered: { confirmDialog.close(); Updater.apply() }
            }
        ]

        ColumnLayout {
            spacing: Theme.spacingSmall
            QQC2.Label {
                text: "The following commands will run:"
                color: Theme.textDim
                font.family: Theme.sansFamily
            }
            Rectangle {
                Layout.fillWidth: true
                color: Theme.deepBg
                radius: Theme.radiusSmall
                implicitHeight: cmds.implicitHeight + Theme.spacing
                QQC2.Label {
                    id: cmds
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSmall
                    text: Updater.plannedCommands()
                    color: Theme.cyan
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    wrapMode: Text.WrapAnywhere
                }
            }
            QQC2.Label {
                text: "You may be prompted for your password."
                color: Theme.textMuted
                font.family: Theme.sansFamily
                font.pixelSize: 12
            }
        }
    }
}
