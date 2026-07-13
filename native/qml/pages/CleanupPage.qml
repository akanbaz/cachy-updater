import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "../components"

ColumnLayout {
    id: page
    spacing: Theme.spacing

    Banner { text: Maintain.warningText }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSmall
        QQC2.Label {
            Layout.fillWidth: true
            text: Maintain.scanning ? "Scanning\u2026" : "Maintenance"
            color: Theme.textDim
            font.family: Theme.sansFamily
            font.pixelSize: 13
        }
        QQC2.Button {
            text: "Rescan"
            icon.name: "view-refresh"
            enabled: !Maintain.scanning && !Maintain.busy
            onClicked: Maintain.scan()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacing

        // ---- Orphans ----------------------------------------------
        CachyCard {
            id: orphanCard
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            padding: Theme.spacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall
                Kirigami.Icon { source: "package-remove"; implicitWidth: 20; implicitHeight: 20 }
                QQC2.Label {
                    text: "Orphan packages"
                    color: Theme.text
                    font.family: Theme.sansFamily
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }
            }
            QQC2.Label {
                text: Maintain.orphanCount + (Maintain.orphanCount === 1 ? " orphan" : " orphans")
                color: Maintain.orphanCount > 0 ? Theme.warnText : Theme.textDim
                font.family: Theme.sansFamily
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: Maintain.orphanCount > 0
                      ? "Dependencies no longer required by any installed package."
                      : "No unneeded dependencies found."
                color: Theme.textMuted
                font.family: Theme.sansFamily
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            QQC2.Button {
                text: "Remove orphans"
                icon.name: "edit-delete"
                enabled: Maintain.orphanCount > 0 && !Maintain.busy
                onClicked: Maintain.removeOrphans()
            }
        }

        // ---- Cache ------------------------------------------------
        CachyCard {
            id: cacheCard
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            padding: Theme.spacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall
                Kirigami.Icon { source: "edit-clear-history"; implicitWidth: 20; implicitHeight: 20 }
                QQC2.Label {
                    text: "Package cache"
                    color: Theme.text
                    font.family: Theme.sansFamily
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }
            }
            QQC2.Label {
                text: Maintain.cacheTotal + (Maintain.cacheTotal === 1 ? " file" : " files")
                color: Maintain.cacheTotal > 0 ? Theme.cyan : Theme.textDim
                font.family: Theme.sansFamily
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: Maintain.cacheOldCount + " old \u00b7 " + Maintain.cacheUninstalledCount
                      + " uninstalled (keeps last 3 versions)"
                color: Theme.textMuted
                font.family: Theme.sansFamily
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            QQC2.Button {
                text: "Clean cache"
                icon.name: "edit-clear-all"
                enabled: Maintain.cacheTotal > 0 && !Maintain.busy
                onClicked: Maintain.cleanCache()
            }
        }
    }

    Item { Layout.fillHeight: true }

    TerminalPanel {
        controller: Maintain
    }
}
