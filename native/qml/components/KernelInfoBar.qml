import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

CachyCard {
    id: bar
    Layout.fillWidth: true
    visible: Updater.showKernelInfo
    padding: Theme.spacingSmall

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacing

        Kirigami.Icon {
            source: "cpu"
            implicitWidth: 22
            implicitHeight: 22
            color: Theme.cyan
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            QQC2.Label {
                text: "Kernel"
                color: Theme.text
                font.family: Theme.sansFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: Updater.kernelHeadline
                color: Theme.text
                font.family: Theme.sansFamily
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            QQC2.Label {
                Layout.fillWidth: true
                visible: Updater.kernelDetail.length > 0
                text: Updater.kernelDetail
                color: Theme.textMuted
                font.family: Theme.sansFamily
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            QQC2.Label {
                Layout.fillWidth: true
                visible: Updater.rebootRequired
                text: "A reboot is needed after a kernel update."
                color: Theme.textFaint
                font.family: Theme.sansFamily
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }

        QQC2.ToolButton {
            icon.name: "view-refresh"
            flat: true
            Layout.alignment: Qt.AlignTop
            onClicked: Updater.refreshKernelInfo()
            QQC2.ToolTip.text: "Refresh kernel info"
            QQC2.ToolTip.visible: hovered
        }
    }
}
