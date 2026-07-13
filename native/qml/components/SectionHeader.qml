import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.cachyos.updater

Item {
    id: header
    required property string section

    width: ListView.view ? ListView.view.width : implicitWidth
    implicitHeight: row.implicitHeight + Theme.spacingSmall

    function labelText() {
        if (section === "aur") return "AUR"
        if (section === "flatpak") return "FLATPAK"
        return "REPO"
    }
    function pillColor() {
        if (section === "aur") return Theme.kernel
        if (section === "flatpak") return "#3B3550"
        return Theme.cyan
    }
    function pillInk() {
        return section === "repo" ? Theme.cyanInk : Theme.text
    }

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.spacingSmall
        spacing: Theme.spacingSmall

        Rectangle {
            radius: Theme.radiusSmall
            color: header.pillColor()
            implicitWidth: pill.implicitWidth + Theme.spacingSmall * 2
            implicitHeight: pill.implicitHeight + 4
            QQC2.Label {
                id: pill
                anchors.centerIn: parent
                text: header.labelText()
                color: header.pillInk()
                font.family: Theme.sansFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }

        QQC2.Label {
            text: Updater.sourceCommandFor(header.section)
            color: Theme.textMuted
            font.family: Theme.monoFamily
            font.pixelSize: 12
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        QQC2.Label {
            text: {
                const n = Updater.sourceCountFor(header.section)
                const sz = Updater.sourceSizeTextFor(header.section)
                return n + (n === 1 ? " package" : " packages") + "  \u00b7  " + sz
            }
            color: Theme.textDim
            font.family: Theme.sansFamily
            font.pixelSize: 12
        }

        QQC2.Button {
            text: "Update"
            flat: true
            enabled: !Updater.busy && Updater.sourceCountFor(header.section) > 0
            onClicked: {
                if (header.section === "repo") Updater.applyRepo()
                else if (header.section === "aur") Updater.applyAur()
                else Updater.applyFlatpak()
            }
        }
    }
}
