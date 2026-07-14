import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

ColumnLayout {
    spacing: Theme.spacing

    Banner { text: Firmware.warningText }

    RowLayout {
        Layout.fillWidth: true
        QQC2.Label {
            Layout.fillWidth: true
            text: Firmware.available ? Firmware.count + " firmware update(s)" : "fwupdmgr not installed"
            color: Theme.textDim
        }
        QQC2.Button {
            text: "Scan"
            icon.name: "view-refresh"
            enabled: !Firmware.busy
            onClicked: Firmware.scan()
        }
        QQC2.Button {
            text: "Update all"
            enabled: !Firmware.busy && Firmware.count > 0
            onClicked: Firmware.updateAll()
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: Firmware.firmwareModel
        spacing: Theme.spacingSmall

        delegate: CachyCard {
            width: ListView.view.width
            padding: Theme.spacing
            ColumnLayout {
                spacing: 4
                QQC2.Label {
                    text: model.name
                    font.weight: Font.DemiBold
                    font.pixelSize: 14
                }
                QQC2.Label {
                    text: model.version + "  \u2192  " + model.newVersion
                    font.family: Theme.monoFamily
                    color: Theme.cyan
                    font.pixelSize: 12
                }
                QQC2.Button {
                    text: "Update"
                    enabled: !Firmware.busy
                    onClicked: Firmware.updateDevice(model.deviceId)
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: Firmware.count === 0 && !Firmware.busy
            icon.name: "cpu"
            text: "No firmware updates"
            explanation: Firmware.available ? "Scan to check device firmware." : "Install fwupd to enable firmware updates."
        }
    }
}
