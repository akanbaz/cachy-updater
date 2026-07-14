import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

ColumnLayout {
    spacing: Theme.spacing

    RowLayout {
        Layout.fillWidth: true
        QQC2.Label {
            Layout.fillWidth: true
            text: History.count + " entries"
            color: Theme.textDim
        }
        QQC2.Button {
            text: "Refresh"
            onClicked: History.reload()
        }
        QQC2.Button {
            text: "Clear"
            onClicked: History.clear()
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: History.historyModel
        spacing: Theme.spacingSmall

        delegate: Rectangle {
            width: ListView.view.width
            radius: Theme.radius
            color: Theme.surface
            implicitHeight: row.implicitHeight + Theme.spacingSmall * 2

            RowLayout {
                id: row
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                spacing: Theme.spacingSmall

                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: model.success ? Theme.ok : Theme.errorText
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    QQC2.Label {
                        text: model.action + "  \u00b7  " + model.timestamp
                        font.weight: Font.DemiBold
                        font.pixelSize: 13
                    }
                    QQC2.Label {
                        text: model.detail
                        color: Theme.textMuted
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: History.count === 0
            icon.name: "view-list-details"
            text: "No history yet"
            explanation: "Apply or maintenance actions will appear here."
        }
    }
}
