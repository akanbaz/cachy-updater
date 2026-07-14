import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

// Collapsible bottom panel showing real command output, color-coded.
Rectangle {
    id: term

    property bool expanded: false
    property var controller: Updater
    property int edgeMargin: Theme.spacingSmall
    property int maxLines: 1500
    property int lineCount: 0
    property int bodyHeight: 200

    readonly property int headerHeight: 40
    readonly property int panelHeight: expanded ? (headerHeight + bodyHeight) : headerHeight

    Layout.fillWidth: true
    Layout.preferredHeight: panelHeight
    Layout.minimumHeight: panelHeight
    Layout.maximumHeight: panelHeight
    color: Theme.deepBg
    radius: Theme.radius
    clip: true

    Behavior on Layout.preferredHeight {
        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
    }
    Behavior on Layout.minimumHeight {
        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
    }
    Behavior on Layout.maximumHeight {
        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
    }

    function escapeHtml(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }
    function colorFor(kind) {
        if (kind === "cmd") return Theme.cyan
        if (kind === "warn") return Theme.warnText
        if (kind === "error") return Theme.errorText
        if (kind === "ok") return Theme.ok
        return Theme.textDim
    }
    function clear() {
        area.text = ""
        lineCount = 0
    }
    function appendLine(text, kind) {
        if (lineCount >= maxLines) {
            clear()
            area.append('<span style="color:' + colorFor("warn")
                        + '">[earlier output trimmed]</span>')
            lineCount = 1
        }
        area.append('<span style="color:' + colorFor(kind) + '">' + escapeHtml(text) + '</span>')
        lineCount += 1
        area.cursorPosition = area.length
    }
    function toggle() {
        expanded = !expanded
    }

    Connections {
        target: term.controller
        function onLineEmitted(text, kind) {
            term.appendLine(text, kind)
            if (kind === "error")
                term.expanded = true
        }
        function onBusyChanged() {
            if (term.controller && term.controller.busy) {
                term.clear()
                // Surface the live output as soon as an action starts, instead
                // of leaving the user staring at a spinner over a collapsed panel.
                term.expanded = true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header — always visible; click anywhere (except action buttons) to toggle.
        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: term.headerHeight
            Layout.maximumHeight: term.headerHeight
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: term.toggle()
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: term.edgeMargin
                anchors.rightMargin: term.edgeMargin
                spacing: Theme.spacingSmall

                Kirigami.Icon {
                    source: term.expanded ? "go-down" : "go-up"
                    implicitWidth: Theme.iconSmall
                    implicitHeight: Theme.iconSmall
                    color: Theme.textMuted
                }
                QQC2.Label {
                    text: "Terminal"
                    color: Theme.textDim
                    font.family: Theme.sansFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                QQC2.Label {
                    visible: !term.expanded && term.lineCount > 0
                    text: term.lineCount + (term.lineCount === 1 ? " line" : " lines")
                    color: Theme.textFaint
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }

                QQC2.ToolButton {
                    text: "Clear"
                    flat: true
                    icon.name: "edit-clear"
                    font.pixelSize: 12
                    enabled: term.lineCount > 0
                    onClicked: term.clear()
                }
                QQC2.ToolButton {
                    text: "Copy log"
                    flat: true
                    icon.name: "edit-copy"
                    font.pixelSize: 12
                    enabled: term.lineCount > 0
                    onClicked: { area.selectAll(); area.copy(); area.deselect() }
                }
                QQC2.ToolButton {
                    text: term.expanded ? "Collapse" : "Expand"
                    flat: true
                    icon.name: term.expanded ? "arrow-down" : "arrow-up"
                    font.pixelSize: 12
                    onClicked: term.toggle()
                }
            }
        }

        // Body — only allocated when expanded; fills remaining panel height.
        QQC2.ScrollView {
            id: scroll
            visible: term.expanded
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: term.edgeMargin
            Layout.rightMargin: term.edgeMargin
            Layout.bottomMargin: Theme.spacingSmall
            clip: true
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AsNeeded
            QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AsNeeded

            QQC2.TextArea {
                id: area
                readOnly: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.NoWrap
                color: Theme.textDim
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle { color: "transparent" }
                activeFocusOnTab: false
            }
        }
    }
}
