import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "../components"

ColumnLayout {
    id: cleanupRoot
    spacing: Theme.spacingSmall
    width: parent ? parent.width : implicitWidth

    Banner { text: Maintain.warningText }
    Banner {
        visible: Maintain.diskSummary.length > 0
        text: Maintain.diskSummary
        severity: "info"
        closable: false
    }

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

    GridLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        columns: 2
        columnSpacing: Theme.spacingSmall
        rowSpacing: Theme.spacingSmall

        component CleanCard: CachyCard {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            padding: Theme.spacingSmall

            property string title
            property string iconName: ""
            property string valueText
            property color valueColor: Theme.textDim
            property string actionText
            property bool actionEnabled: false
            property var action

            ColumnLayout {
                spacing: 4
                RowLayout {
                    spacing: Theme.spacingSmall
                    Kirigami.Icon {
                        visible: iconName.length > 0
                        source: iconName
                        implicitWidth: 18
                        implicitHeight: 18
                    }
                    QQC2.Label {
                        text: title
                        font.weight: Font.DemiBold
                        font.pixelSize: 13
                        Layout.fillWidth: true
                    }
                }
                QQC2.Label {
                    text: valueText
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: valueColor
                }
                QQC2.Button {
                    text: actionText
                    enabled: actionEnabled && !Maintain.busy
                    onClicked: if (action) action()
                }
            }
        }

        CleanCard {
            title: "Orphan packages"
            iconName: "package-remove"
            valueText: Maintain.orphanCount + " orphan(s)"
            valueColor: Maintain.orphanCount > 0 ? Theme.warnText : Theme.textDim
            actionText: "Remove orphans"
            actionEnabled: Maintain.orphanCount > 0
            action: function() { Maintain.removeOrphans() }
        }

        CleanCard {
            title: "Package cache (keep " + Settings.cacheKeepVersions + ")"
            iconName: "edit-clear-history"
            valueText: Maintain.cacheTotal + " file(s)"
            actionText: "Clean cache"
            actionEnabled: Maintain.cacheTotal > 0
            action: function() { Maintain.cleanCache() }
        }

        CleanCard {
            title: "Old kernels"
            valueText: Maintain.oldKernelCount + " removable"
            valueColor: Maintain.oldKernelCount > 0 ? Theme.text : Theme.textDim
            actionText: "Remove old kernels"
            actionEnabled: Maintain.oldKernelCount > 0
            action: function() { Maintain.removeOldKernels() }
        }

        CleanCard {
            title: "Flatpak unused"
            valueText: Maintain.flatpakUnusedCount + " runtime(s)"
            actionText: "Clean unused"
            actionEnabled: Maintain.flatpakUnusedCount > 0
            action: function() { Maintain.cleanFlatpakUnused() }
        }

        CleanCard {
            title: "AUR build cache"
            valueText: Maintain.aurCacheCount + " package(s) in cache"
            valueColor: Maintain.aurCacheCount > 0 ? Theme.text : Theme.textDim
            actionText: "Clean AUR cache"
            actionEnabled: Maintain.aurCacheCount > 0
            action: function() { Maintain.cleanAurCache() }
        }
    }
}
