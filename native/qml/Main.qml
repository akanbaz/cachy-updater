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
        { name: "Updates", icon: "update-high" },
        { name: "News", icon: "news-subscribe" },
        { name: "Cleanup", icon: "edit-clear-all" }
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
                color: Theme.deepBg
                implicitHeight: headerRow.implicitHeight + Theme.spacingLarge

                RowLayout {
                    id: headerRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.rightMargin: Theme.spacingLarge
                    spacing: Theme.spacing

                    Image {
                        source: Qt.resolvedUrl("assets/logo.svg")
                        sourceSize.width: 36
                        sourceSize.height: 36
                    }

                    ColumnLayout {
                        spacing: 0
                        QQC2.Label {
                            text: "CachyOS Updater"
                            color: Theme.text
                            font.family: Theme.sansFamily
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }
                        QQC2.Label {
                            text: "Fast. Clean. Native."
                            color: Theme.cyan
                            font.family: Theme.sansFamily
                            font.pixelSize: 12
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: root.statusColor()
                        opacity: Updater.busy ? 0.5 : 1.0
                    }
                    QQC2.Label {
                        text: Updater.statusText
                        color: Theme.textDim
                        font.family: Theme.sansFamily
                        font.pixelSize: 13
                    }
                    QQC2.BusyIndicator {
                        visible: Updater.busy
                        running: Updater.busy
                        implicitWidth: 18
                        implicitHeight: 18
                    }
                }
            }

            // ---- Nav tabs ----------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                color: Theme.bg
                implicitHeight: 52

                RowLayout {
                    id: tabRow
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.spacingLarge
                    spacing: Theme.spacingSmall

                    Repeater {
                        model: root.tabs
                        delegate: QQC2.AbstractButton {
                            id: tabButton
                            required property int index
                            required property var modelData
                            readonly property bool active: root.currentTab === index
                            hoverEnabled: true
                            implicitHeight: 34
                            implicitWidth: tabContent.implicitWidth + Theme.spacing * 2
                            onClicked: root.currentTab = index

                            background: Rectangle {
                                radius: Theme.radius
                                color: tabButton.active ? Theme.surfaceHover
                                     : tabButton.hovered ? Theme.rowHover
                                     : "transparent"
                            }

                            contentItem: RowLayout {
                                id: tabContent
                                spacing: 6
                                Kirigami.Icon {
                                    source: modelData.icon
                                    implicitWidth: 16; implicitHeight: 16
                                    color: tabButton.active ? Theme.cyan : Theme.textMuted
                                    opacity: tabButton.active ? 1.0 : 0.7
                                }
                                QQC2.Label {
                                    text: modelData.name
                                    color: tabButton.active ? Theme.cyan : Theme.textDim
                                    font.family: Theme.sansFamily
                                    font.pixelSize: 13
                                    font.weight: tabButton.active ? Font.DemiBold : Font.Normal
                                }
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

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

                    Item { Layout.fillWidth: true }

                    QQC2.Label {
                        text: Updater.selectedCount + " of " + Updater.packageCount + " selected"
                        color: Theme.textMuted
                        font.family: Theme.sansFamily
                        font.pixelSize: 12
                    }

                    QQC2.Button {
                        text: "Dry Run"
                        flat: true
                        enabled: !Updater.busy && Updater.selectedCount > 0
                        onClicked: Updater.dryRun()
                    }
                    QQC2.Button {
                        text: "Download Only"
                        flat: true
                        enabled: !Updater.busy && Updater.selectedCount > 0
                        onClicked: Updater.downloadOnly()
                    }

                    QQC2.Button {
                        id: applyButton
                        text: "Apply Updates"
                        enabled: !Updater.busy && Updater.selectedCount > 0
                        onClicked: confirmDialog.open()

                        contentItem: QQC2.Label {
                            text: applyButton.text
                            color: applyButton.enabled ? Theme.cyanInk : Theme.textFaint
                            font.family: Theme.sansFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: Theme.radiusSmall
                            implicitHeight: 32
                            implicitWidth: 130
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
