import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

ColumnLayout {
    id: historyRoot
    spacing: Theme.spacing

    function relativeTime(iso) {
        if (!iso)
            return ""
        const then = new Date(iso)
        if (isNaN(then.getTime()))
            return iso
        const secs = Math.floor((Date.now() - then.getTime()) / 1000)
        if (secs < 60) return "just now"
        const mins = Math.floor(secs / 60)
        if (mins < 60) return mins + (mins === 1 ? " min ago" : " mins ago")
        const hrs = Math.floor(mins / 60)
        if (hrs < 24) return hrs + (hrs === 1 ? " hour ago" : " hours ago")
        const days = Math.floor(hrs / 24)
        if (days < 30) return days + (days === 1 ? " day ago" : " days ago")
        return Qt.formatDate(then, "yyyy-MM-dd")
    }

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

            HoverHandler { id: histHover }
            // Full ISO timestamp on hover; the row shows a friendly relative time.
            QQC2.ToolTip.text: model.timestamp
            QQC2.ToolTip.visible: histHover.hovered && model.timestamp.length > 0

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
                        text: model.action + "  \u00b7  " + historyRoot.relativeTime(model.timestamp)
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
