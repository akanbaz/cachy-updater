import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

ColumnLayout {
    id: updatesRoot
    spacing: Theme.spacingSmall
    width: parent ? parent.width : implicitWidth

    ColumnLayout {
        id: chromeColumn
        Layout.fillWidth: true
        spacing: Theme.spacingSmall

    Banner { text: Updater.warningText }

    Banner {
        visible: Updater.archNewsBlocked
        text: Updater.archGateText
        severity: "warn"
        closable: false
    }

    RowLayout {
        visible: Updater.archNewsBlocked
        QQC2.Button {
            text: "I've read the Arch news"
            onClicked: { News.acknowledgeArchGate(Settings); Updater.acknowledgeArchNews() }
        }
    }

    Banner {
        visible: Updater.nvidiaKernelWarning
        text: "NVIDIA and kernel updates are selected together \u2014 DKMS will rebuild on reboot."
        severity: "warn"
    }

    Banner {
        visible: Updater.partialUpgradeWarning
        text: "Some repo updates are deselected or held \u2014 this is a partial upgrade. "
              + "Version-locked packages (gcc, glibc, pipewire, p11-kit\u2026) must upgrade "
              + "together, so pacman may fail. Prefer Apply all repo updates, or hold a whole locked group."
        severity: "warn"
        closable: false
    }

    RowLayout {
        visible: Updater.partialUpgradeWarning
        QQC2.Button {
            text: "Select all updates"
            icon.name: "package-install"
            onClicked: Updater.setAllSelected(true)
        }
    }

    Banner {
        visible: Updater.rebootRequired
        text: "A reboot is required after these updates finish."
        severity: "info"
    }

    RowLayout {
        visible: Updater.rebootRequired
        QQC2.Button {
            text: "Reboot now"
            icon.name: "system-reboot"
            onClicked: Updater.reboot()
        }
    }

    KernelInfoBar {}

    Repeater {
        model: Updater.kernelModel
        delegate: KernelCard {}
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSmall
        visible: Updater.packageCount > 0

        QQC2.Label {
            Layout.fillWidth: true
            color: Theme.textDim
            font.family: Theme.sansFamily
            font.pixelSize: 13
            text: Updater.packageCount
                  + (Updater.packageCount === 1 ? " update pending" : " updates pending")
                  + "   \u00b7   " + Updater.downloadText + " download"
                  + "   \u00b7   " + Updater.sourceCount
                  + (Updater.sourceCount === 1 ? " source" : " sources")
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSmall

        Kirigami.SearchField {
            id: searchField
            Layout.fillWidth: true
            Layout.minimumWidth: 140
            opacity: Updater.packageCount > 0 ? 1.0 : 0.0
            enabled: Updater.packageCount > 0 && !Updater.busy
            placeholderText: "Search packages…"
            text: Updater.searchText
            onTextChanged: Updater.setSearchText(text)
            Keys.onEscapePressed: (event) => {
                if (text.length > 0) {
                    clear()
                    event.accepted = true
                } else {
                    focus = false
                    event.accepted = true
                }
            }
        }

        QQC2.ComboBox {
            Layout.preferredWidth: 148
            opacity: Updater.packageCount > 0 ? 1.0 : 0.0
            enabled: Updater.packageCount > 0 && !Updater.busy
            model: ["By source", "Important first", "Largest first", "Name A\u2013Z"]
            currentIndex: Updater.sortMode
            onActivated: Updater.setSortMode(currentIndex)
        }

        QQC2.Button {
            text: "Important only"
            flat: true
            opacity: Updater.packageCount > 0 ? 1.0 : 0.0
            enabled: Updater.packageCount > 0 && !Updater.busy
            onClicked: Updater.selectImportant()
            QQC2.ToolTip.text: "Select only important and critical updates"
            QQC2.ToolTip.visible: hovered
        }

        QQC2.CheckBox {
            text: "Select all"
            opacity: Updater.packageCount > 0 ? 1.0 : 0.0
            enabled: Updater.packageCount > 0 && !Updater.busy
            checked: Updater.selectedCount === Updater.packageCount && Updater.packageCount > 0
            onToggled: Updater.setAllSelected(checked)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSmall
        visible: Updater.packageCount > 0
                 && (Updater.searchText.length > 0
                     || Updater.minSeverity > 0
                     || Updater.sourceFilter.length > 0)

        QQC2.Label {
            Layout.fillWidth: true
            text: "Showing " + list.count + " of " + Updater.packageCount + " packages"
            color: Theme.textMuted
            font.family: Theme.sansFamily
            font.pixelSize: 12
        }
        QQC2.Button {
            text: "Clear filters"
            flat: true
            icon.name: "edit-clear"
            onClicked: {
                Updater.searchText = ""
                Updater.minSeverity = 0
                Updater.sourceFilter = ""
            }
        }
    }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: Updater.packageCount === 0 && !Updater.busy ? 140 : 80

        ListView {
            id: list
            anchors.fill: parent
            // Reserve a dedicated strip for the scrollbar so it never overlays rows.
            anchors.rightMargin: scrollBar.policy !== QQC2.ScrollBar.AlwaysOff
                                 && (contentHeight > height) ? scrollBar.implicitWidth : 0
            clip: true
            spacing: 0
            model: Updater.updatesModel
            boundsBehavior: Flickable.StopAtBounds

            section.property: "source"
            section.criteria: ViewSection.FullString
            section.delegate: SectionHeader {}

            delegate: PackageDelegate {}

            QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                id: scrollBar
                parent: list.parent
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                policy: list.contentHeight > list.height
                        ? QQC2.ScrollBar.AlwaysOn : QQC2.ScrollBar.AlwaysOff
            }

            Kirigami.PlaceholderMessage {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: Theme.spacingLarge * 2
                width: parent.width - Theme.spacingLarge * 2
                visible: Updater.packageCount === 0 && !Updater.busy
                icon.name: Updater.statusState === "uptodate" ? "org.cachyos.updater" : "org.cachyos.updater-tray-updates"
                text: Updater.statusState === "uptodate" ? "System is up to date" : "No updates yet"
                explanation: Updater.statusState === "uptodate"
                    ? "Everything is current. Last checked " + Updater.lastChecked + "."
                    : Settings.offlineMode ? "Offline mode \u2014 showing cached results."
                    : "Press Refresh to check for package updates."
            }
        }
    }
}
