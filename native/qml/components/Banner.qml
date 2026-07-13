import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

Rectangle {
    id: banner

    property string text: ""
    property string iconName: severity === "info" ? "dialog-information" : "dialog-warning"
    property string severity: "warn"
    property bool dismissed: false
    property bool closable: true

    readonly property color bgColor: severity === "info" ? Theme.cyanBg : Theme.warnBg
    readonly property color borderColor: severity === "info" ? Theme.cyan : Theme.warnBorder
    readonly property color textColor: severity === "info" ? Theme.cyan : Theme.warnText

    Layout.fillWidth: true
    visible: text.length > 0 && !dismissed
    implicitHeight: visible ? row.implicitHeight + Theme.spacing : 0
    color: bgColor
    radius: Theme.radiusSmall

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        radius: 2
        color: borderColor
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        anchors.leftMargin: Theme.spacing
        spacing: Theme.spacingSmall

        Kirigami.Icon {
            source: banner.iconName
            implicitWidth: 18
            implicitHeight: 18
            color: textColor
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: banner.text
            color: textColor
            wrapMode: Text.WordWrap
            font.family: Theme.sansFamily
        }
        QQC2.ToolButton {
            visible: closable
            icon.name: "dialog-close"
            flat: true
            onClicked: banner.dismissed = true
            QQC2.ToolTip.text: "Dismiss"
            QQC2.ToolTip.visible: hovered
        }
    }
}
