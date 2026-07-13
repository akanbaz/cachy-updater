import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "../components"

ColumnLayout {
    spacing: Theme.spacing

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
        columns: 2
        columnSpacing: Theme.spacing
        rowSpacing: Theme.spacing

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacingSmall
                RowLayout {
                    Kirigami.Icon { source: "package-remove"; implicitWidth: 20; implicitHeight: 20 }
                    QQC2.Label {
                        text: "Orphan packages"
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }
                }
                QQC2.Label {
                    text: Maintain.orphanCount + " orphan(s)"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    color: Maintain.orphanCount > 0 ? Theme.warnText : Theme.textDim
                }
                QQC2.Button {
                    text: "Remove orphans"
                    enabled: Maintain.orphanCount > 0 && !Maintain.busy
                    onClicked: Maintain.removeOrphans()
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacingSmall
                RowLayout {
                    Kirigami.Icon { source: "edit-clear-history"; implicitWidth: 20; implicitHeight: 20 }
                    QQC2.Label {
                        text: "Package cache (keep " + Settings.cacheKeepVersions + ")"
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }
                }
                QQC2.Label {
                    text: Maintain.cacheTotal + " file(s)"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
                QQC2.Button {
                    text: "Clean cache"
                    enabled: Maintain.cacheTotal > 0 && !Maintain.busy
                    onClicked: Maintain.cleanCache()
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacingSmall
                QQC2.Label { text: "Old kernels"; font.weight: Font.DemiBold }
                QQC2.Label {
                    text: Maintain.oldKernelCount + " removable"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
                QQC2.Button {
                    text: "Remove old kernels"
                    enabled: Maintain.oldKernelCount > 0 && !Maintain.busy
                    onClicked: Maintain.removeOldKernels()
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacingSmall
                QQC2.Label { text: "Flatpak unused"; font.weight: Font.DemiBold }
                QQC2.Label {
                    text: Maintain.flatpakUnusedCount + " runtime(s)"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
                QQC2.Button {
                    text: "Clean unused"
                    enabled: Maintain.flatpakUnusedCount > 0 && !Maintain.busy
                    onClicked: Maintain.cleanFlatpakUnused()
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            padding: Theme.spacing
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    QQC2.Label { text: "AUR build cache"; font.weight: Font.DemiBold }
                    QQC2.Label { text: Maintain.aurCacheCount + " package(s) in cache" }
                }
                QQC2.Button {
                    text: "Clean AUR cache"
                    enabled: Maintain.aurCacheCount > 0 && !Maintain.busy
                    onClicked: Maintain.cleanAurCache()
                }
            }
        }
    }
}
