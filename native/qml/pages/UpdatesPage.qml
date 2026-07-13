import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "../components"

ColumnLayout {
    spacing: Theme.spacing

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
        Layout.minimumHeight: 32
        spacing: Theme.spacingSmall

        QQC2.Label {
            Layout.fillWidth: true
            color: Theme.textDim
            font.family: Theme.sansFamily
            font.pixelSize: 13
            opacity: Updater.packageCount > 0 ? 1.0 : 0.0
            text: Updater.packageCount > 0
                  ? Updater.packageCount
                    + (Updater.packageCount === 1 ? " update pending" : " updates pending")
                    + "   \u00b7   " + Updater.downloadText + " download"
                    + "   \u00b7   " + Updater.sourceCount
                    + (Updater.sourceCount === 1 ? " source" : " sources")
                  : " "
        }

        QQC2.CheckBox {
            text: "Select all"
            opacity: Updater.packageCount > 0 ? 1.0 : 0.0
            enabled: Updater.packageCount > 0 && !Updater.busy
            checked: Updater.selectedCount === Updater.packageCount && Updater.packageCount > 0
            onToggled: Updater.setAllSelected(checked)
        }
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
