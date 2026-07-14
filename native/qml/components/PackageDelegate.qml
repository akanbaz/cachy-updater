import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

// One package row: checkbox, name, version transition, severity badge, size,
// and an expandable change-summary. Uses the injected `model` context so the
// checkbox writes back through the model (QAbstractListModel::setData).
Item {
    id: delegate

    property bool expanded: false

    width: ListView.view ? ListView.view.width : implicitWidth
    implicitHeight: col.implicitHeight + 4

    function badgeBg() {
        if (model.severity >= 3) return Theme.warnBg
        return Qt.rgba(1, 1, 1, 0.06)
    }
    function badgeText() {
        if (model.severity >= 3) return Theme.warnText
        if (model.severity === 2) return Theme.textDim
        return Theme.textMuted
    }

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        radius: Theme.radiusSmall
        color: hover.hovered ? Theme.rowHover : "transparent"
    }

    HoverHandler { id: hover }

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingSmall
            Layout.rightMargin: Theme.spacingSmall
            spacing: Theme.spacingSmall

            QQC2.CheckBox {
                checked: model.selected
                enabled: !model.held
                onToggled: model.selected = checked
            }

            QQC2.Label {
                text: model.name
                color: model.held ? Theme.textMuted : Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            Kirigami.Icon {
                visible: model.lockedGroup
                source: "link"
                implicitWidth: 13
                implicitHeight: 13
                color: Theme.textMuted
                HoverHandler { id: linkHover }
                QQC2.ToolTip.text: "Version-locked — updates together with its related packages"
                QQC2.ToolTip.visible: linkHover.hovered
            }

            QQC2.Label {
                text: model.oldVersion + "  \u2192  "
                color: Theme.textMuted
                font.family: Theme.monoFamily
                font.pixelSize: 12
            }
            QQC2.Label {
                text: model.newVersion
                color: Theme.cyan
                font.family: Theme.monoFamily
                font.pixelSize: 12
                Layout.leftMargin: -Theme.spacingSmall
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Rectangle {
                visible: model.severityLabel.length > 0
                radius: Theme.radiusSmall
                color: delegate.badgeBg()
                implicitWidth: badge.implicitWidth + Theme.spacingSmall * 1.5
                implicitHeight: badge.implicitHeight + 4
                QQC2.Label {
                    id: badge
                    anchors.centerIn: parent
                    text: model.severityLabel
                    color: delegate.badgeText()
                    font.family: Theme.sansFamily
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }
            }

            QQC2.Label {
                text: model.sizeText
                color: Theme.textDim
                font.family: Theme.sansFamily
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                Layout.minimumWidth: 64
            }

            QQC2.Label {
                visible: model.source === "flatpak" && model.flatpakKind.length > 0
                text: model.flatpakKind
                color: Theme.textFaint
                font.pixelSize: 10
            }

            QQC2.ToolButton {
                flat: true
                icon.name: model.held ? "object-unlocked" : "object-locked"
                onClicked: {
                    if (model.held)
                        Updater.unholdPackage(model.name)
                    else
                        Updater.holdPackage(model.name)
                }
                QQC2.ToolTip.text: model.held ? "Unhold package" : "Hold package"
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ToolButton {
                flat: true
                icon.name: delegate.expanded ? "go-down" : "go-next"
                icon.width: 16
                icon.height: 16
                implicitWidth: 26
                implicitHeight: 26
                onClicked: delegate.expanded = !delegate.expanded
            }
        }

        QQC2.Label {
            visible: delegate.expanded
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingLarge + Theme.spacingSmall
            Layout.rightMargin: Theme.spacingSmall
            Layout.bottomMargin: Theme.spacingSmall
            text: model.changelog.length > 0 ? model.changelog : model.summary
            color: Theme.textDim
            wrapMode: Text.WordWrap
            font.family: Theme.sansFamily
            font.pixelSize: 12
        }
    }
}
