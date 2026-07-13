import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

// Warning / notice banner. Warm colors are reserved for exactly this purpose.
Rectangle {
    id: banner

    property string text: ""
    property string iconName: "dialog-warning"
    property bool dismissed: false

    Layout.fillWidth: true
    visible: text.length > 0 && !dismissed
    implicitHeight: visible ? row.implicitHeight + Theme.spacing : 0
    color: Theme.warnBg
    radius: Theme.radiusSmall

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        radius: 2
        color: Theme.warnBorder
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
            color: Theme.warnText
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: banner.text
            color: Theme.warnText
            wrapMode: Text.WordWrap
            font.family: Theme.sansFamily
        }
        QQC2.ToolButton {
            icon.name: "dialog-close"
            flat: true
            onClicked: banner.dismissed = true
            QQC2.ToolTip.text: "Dismiss"
            QQC2.ToolTip.visible: hovered
        }
    }
}
