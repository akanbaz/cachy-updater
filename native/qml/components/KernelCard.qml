import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

// Kernel updates are pulled out distinctly: deep blue, reboot hint.
// Uses the injected `model` context (Repeater over the kernel proxy model).
Rectangle {
    id: kcard

    Layout.fillWidth: true
    radius: Theme.radius
    color: Theme.kernelDeep
    implicitHeight: row.implicitHeight + Theme.spacing * 2

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        QQC2.CheckBox {
            checked: model.selected
            onToggled: model.selected = checked
            Layout.alignment: Qt.AlignTop
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                QQC2.Label {
                    text: model.name
                    color: Theme.text
                    font.family: Theme.sansFamily
                    font.weight: Font.DemiBold
                    font.pixelSize: 14
                }
                QQC2.Label {
                    text: model.oldVersion + "  \u2192  "
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                }
                QQC2.Label {
                    text: model.newVersion
                    color: Theme.cyan
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    Layout.leftMargin: -Theme.spacingSmall
                }

                Rectangle {
                    visible: model.runningKernel
                    radius: Theme.radiusSmall
                    color: Theme.ok
                    implicitWidth: runLbl.implicitWidth + 6
                    implicitHeight: runLbl.implicitHeight + 2
                    QQC2.Label {
                        id: runLbl
                        anchors.centerIn: parent
                        text: "RUNNING"
                        color: Theme.bg
                        font.pixelSize: 8
                        font.weight: Font.DemiBold
                    }
                }

                Rectangle {
                    radius: Theme.radiusSmall
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.cyan
                    implicitWidth: hint.implicitWidth + Theme.spacingSmall
                    implicitHeight: hint.implicitHeight + 3
                    QQC2.Label {
                        id: hint
                        anchors.centerIn: parent
                        text: "KERNEL"
                        color: Theme.cyan
                        font.family: Theme.sansFamily
                        font.pixelSize: 9
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1
                    }
                }

                Item { Layout.fillWidth: true }

                QQC2.Label {
                    text: model.sizeText + "  \u00b7  reboot required"
                    color: Theme.textDim
                    font.family: Theme.sansFamily
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignTop
                }
            }

            QQC2.Label {
                visible: model.summary.length > 0
                Layout.fillWidth: true
                text: model.summary
                color: Theme.textDim
                wrapMode: Text.WordWrap
                font.family: Theme.sansFamily
                font.pixelSize: 12
            }
        }
    }
}
