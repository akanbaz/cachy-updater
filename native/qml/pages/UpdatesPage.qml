import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater
import "../components"

ColumnLayout {
    id: page
    spacing: Theme.spacing

    Banner {
        text: Updater.warningText
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
                  + (Updater.packageCount === 1 ? " package available" : " packages available")
                  + "   \u00b7   total download " + Updater.downloadText
                  + "   \u00b7   " + Updater.sourceCount
                  + (Updater.sourceCount === 1 ? " source" : " sources")
        }

        QQC2.CheckBox {
            text: "Select all"
            checked: Updater.selectedCount === Updater.packageCount && Updater.packageCount > 0
            onToggled: Updater.setAllSelected(checked)
        }
    }

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
                : "Press Refresh to check for package updates."
        }
    }

    TerminalPanel {
        id: terminal
    }
}
