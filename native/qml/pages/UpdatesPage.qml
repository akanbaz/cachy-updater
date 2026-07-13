import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "../components"

ColumnLayout {
    spacing: Theme.spacing

    Banner { text: Updater.warningText }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSmall

        QQC2.TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Search packages\u2026"
            text: Updater.searchText
            onTextChanged: Updater.searchText = text
        }

        QQC2.ComboBox {
            id: severityFilter
            model: ["All severities", "Notice+", "Notable+", "Important only"]
            onActivated: Updater.minSeverity = index
        }

        QQC2.ComboBox {
            id: sourceFilter
            model: ["All sources", "repo", "aur", "flatpak"]
            onActivated: Updater.sourceFilter = index === 0 ? "" : model[index]
        }

        QQC2.Button {
            text: "Clear"
            onClicked: Updater.clearFilters()
        }
    }

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

    RowLayout {
        Layout.fillWidth: true
        visible: Updater.packageCount > 0
        spacing: Theme.spacingSmall

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
                  + (Updater.mirrorStatus.length > 0 ? "   \u00b7   mirrors: " + Updater.mirrorStatus : "")
        }

        QQC2.Button {
            text: "Mirror check"
            icon.name: "network-wireless"
            enabled: !Updater.busy
            onClicked: Updater.checkMirrorHealth()
        }

        QQC2.CheckBox {
            text: "Select all"
            checked: Updater.selectedCount === Updater.packageCount && Updater.packageCount > 0
            onToggled: Updater.setAllSelected(checked)
        }
    }

    KernelInfoBar {}

    Repeater {
        model: Updater.kernelModel
        delegate: KernelCard {}
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 0
        model: Updater.updatesModel
        boundsBehavior: Flickable.StopAtBounds

        section.property: "source"
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {}

        delegate: PackageDelegate {}

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Theme.spacingLarge * 4
            visible: Updater.packageCount === 0 && !Updater.busy
            icon.name: Updater.statusState === "uptodate" ? "checkmark" : "system-software-update"
            text: Updater.statusState === "uptodate" ? "System is up to date" : "No updates yet"
            explanation: Updater.statusState === "uptodate"
                ? "Everything is current. Last checked " + Updater.lastChecked + "."
                : Settings.offlineMode ? "Offline mode \u2014 showing cached results."
                : "Press Refresh to check for package updates."
        }
    }
}
