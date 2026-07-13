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
        }

        Kirigami.Icon {
            source: "system-run"
            implicitWidth: 28
            implicitHeight: 28
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                spacing: Theme.spacingSmall
                QQC2.Label {
                    text: "Kernel update"
                    color: Theme.text
                    font.family: Theme.sansFamily
                    font.weight: Font.DemiBold
                    font.pixelSize: 13
                }
                Rectangle {
                    radius: Theme.radiusSmall
                    color: Theme.cyan
                    implicitWidth: hint.implicitWidth + Theme.spacingSmall
                    implicitHeight: hint.implicitHeight + 3
                    QQC2.Label {
                        id: hint
                        anchors.centerIn: parent
                        text: "REBOOT AFTER"
                        color: Theme.cyanInk
                        font.family: Theme.sansFamily
                        font.pixelSize: 9
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1
                    }
                }
            }

            QQC2.Label {
                text: model.name + "   " + model.oldVersion + "  \u2192  " + model.newVersion
                color: Theme.textDim
                font.family: Theme.monoFamily
                font.pixelSize: 12
            }
        }

        QQC2.Label {
            text: model.sizeText
            color: Theme.textDim
            font.family: Theme.sansFamily
            font.pixelSize: 12
        }
    }
}
