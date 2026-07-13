import QtQuick
import QtQuick.Layouts
import org.cachyos.updater

// Flat, borderless surface following the CachyOS design language.
Rectangle {
    id: card

    default property alias content: holder.data
    property real padding: Theme.spacing
    property alias spacing: holder.spacing

    color: Theme.surface
    radius: Theme.radius

    implicitWidth: holder.implicitWidth + padding * 2
    implicitHeight: holder.implicitHeight + padding * 2

    ColumnLayout {
        id: holder
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: card.padding
        spacing: Theme.spacingSmall
    }
}
