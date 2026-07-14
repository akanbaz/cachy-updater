import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

Flickable {
    contentWidth: width
    contentHeight: col.implicitHeight
    clip: true

    ColumnLayout {
        id: col
        width: parent.width
        spacing: Theme.spacingLarge

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacing
                QQC2.Label { text: "General"; font.weight: Font.DemiBold; font.pixelSize: 15 }

                Banner {
                    visible: Settings.configWarning.length > 0
                    text: Settings.configWarning
                    severity: "warn"
                    closable: false
                }

                RowLayout {
                    QQC2.Label { text: "Default tab"; Layout.fillWidth: true }
                    QQC2.ComboBox {
                        model: ["Updates", "News", "Cleanup", "Firmware", "History", "Settings"]
                        currentIndex: Settings.defaultTab
                        onActivated: Settings.defaultTab = currentIndex
                    }
                }
                QQC2.CheckBox {
                    text: "Check for updates on startup"
                    checked: Settings.autoCheckOnStartup
                    onToggled: Settings.autoCheckOnStartup = checked
                }
                QQC2.CheckBox {
                    text: "Offline mode (use cached check results)"
                    checked: Settings.offlineMode
                    onToggled: Settings.offlineMode = checked
                }
                QQC2.CheckBox {
                    text: "Enable AUR checks"
                    checked: Settings.enableAur
                    onToggled: Settings.enableAur = checked
                }
                QQC2.CheckBox {
                    text: "Enable Flatpak checks"
                    checked: Settings.enableFlatpak
                    onToggled: Settings.enableFlatpak = checked
                }
                QQC2.CheckBox {
                    text: "Enable news fetches"
                    checked: Settings.enableNews
                    onToggled: Settings.enableNews = checked
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacing
                QQC2.Label { text: "Tray & notifications"; font.weight: Font.DemiBold; font.pixelSize: 15 }

                RowLayout {
                    QQC2.Label { text: "Tray check interval (minutes)"; Layout.fillWidth: true }
                    QQC2.SpinBox {
                        from: 5; to: 1440; value: Settings.trayIntervalMinutes
                        onValueModified: Settings.trayIntervalMinutes = value
                    }
                }
                QQC2.CheckBox {
                    text: "Notify only for important updates"
                    checked: Settings.notifyCriticalOnly
                    onToggled: Settings.notifyCriticalOnly = checked
                }
                QQC2.CheckBox {
                    text: "Enable nightly scheduled checks (systemd timer)"
                    checked: Settings.scheduledChecks
                    onToggled: Settings.scheduledChecks = checked
                }
                QQC2.Label {
                    text: "After enabling, run: systemctl --user enable --now org.cachyos.updater-check.timer"
                    color: Theme.textMuted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacing
                QQC2.Label { text: "Performance & safety"; font.weight: Font.DemiBold; font.pixelSize: 15 }

                RowLayout {
                    QQC2.Label { text: "Pacman max parallel downloads (0 = default)"; Layout.fillWidth: true }
                    QQC2.SpinBox {
                        from: 0; to: 20; value: Settings.maxDownloads
                        onValueModified: Settings.maxDownloads = value
                    }
                }
                RowLayout {
                    QQC2.Label { text: "AUR build concurrency (0 = default)"; Layout.fillWidth: true }
                    QQC2.SpinBox {
                        from: 0; to: 16; value: Settings.paruConcurrency
                        onValueModified: Settings.paruConcurrency = value
                    }
                }
                RowLayout {
                    QQC2.Label { text: "Cache versions to keep"; Layout.fillWidth: true }
                    QQC2.SpinBox {
                        from: 1; to: 10; value: Settings.cacheKeepVersions
                        onValueModified: Settings.cacheKeepVersions = value
                    }
                }
                QQC2.CheckBox {
                    text: "Create snapper snapshot before applying updates"
                    checked: Settings.enableSnapshots
                    enabled: Updater.snapshotsAvailable
                    onToggled: Settings.enableSnapshots = checked
                }
            }
        }

        CachyCard {
            Layout.fillWidth: true
            padding: Theme.spacing
            ColumnLayout {
                spacing: Theme.spacing
                QQC2.Label { text: "Held packages"; font.weight: Font.DemiBold; font.pixelSize: 15 }
                QQC2.Label {
                    text: Settings.holdPackages.length > 0 ? Settings.holdPackages.join(", ") : "No held packages"
                    color: Theme.textMuted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                RowLayout {
                    QQC2.TextField {
                        id: holdField
                        Layout.fillWidth: true
                        placeholderText: "Package name"
                        Keys.onEscapePressed: (event) => {
                            focus = false
                            event.accepted = true
                        }
                    }
                    QQC2.Button {
                        text: "Hold"
                        onClicked: { if (holdField.text.length > 0) { Settings.addHold(holdField.text); holdField.text = "" } }
                    }
                }
                QQC2.Button {
                    text: "Refresh held state"
                    onClicked: Updater.check()
                }
            }
        }
    }
}
