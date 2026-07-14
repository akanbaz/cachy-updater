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
    readonly property int barHeight: bar.implicitHeight + Theme.spacingSmall * 2
    readonly property int expandedHeight: Math.min(220, Math.max(120, expanded ? 220 : barHeight))

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? expandedHeight : barHeight
    Layout.maximumHeight: expanded ? expandedHeight : barHeight
    Behavior on Layout.preferredHeight { NumberAnimation { duration: 0 } }
    color: Theme.deepBg
    radius: Theme.radius
    clip: true

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

    Connections {
        target: term.controller
        function onLineEmitted(text, kind) {
            term.appendLine(text, kind)
            if (kind === "error")
                term.expanded = true
        }
        function onBusyChanged() {
            if (term.controller && term.controller.busy)
                term.clear()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: term.edgeMargin
        anchors.rightMargin: term.edgeMargin
        anchors.topMargin: Theme.spacingSmall
        anchors.bottomMargin: Theme.spacingSmall
        spacing: Theme.spacingSmall

        RowLayout {
            id: bar
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            Kirigami.Icon {
                source: "utilities-terminal"
                implicitWidth: Theme.iconSmall
                implicitHeight: Theme.iconSmall
            }
            QQC2.Label {
                text: "Terminal"
                color: Theme.textDim
                font.family: Theme.sansFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
            QQC2.ToolButton {
                text: "Clear"
                flat: true
                icon.name: "edit-clear"
                font.pixelSize: 12
                onClicked: term.clear()
            }
            QQC2.ToolButton {
                text: "Copy log"
                flat: true
                icon.name: "edit-copy"
                font.pixelSize: 12
                onClicked: { area.selectAll(); area.copy(); area.deselect() }
            }
            QQC2.ToolButton {
                flat: true
                icon.name: term.expanded ? "go-down" : "go-up"
                onClicked: term.expanded = !term.expanded
            }
        }

        QQC2.ScrollView {
            visible: term.expanded
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(0, term.expandedHeight - term.barHeight)
            clip: true
            // Ensure scrollbars appear when content overflows.
            ScrollBar.horizontal.policy: QQC2.ScrollBar.AsNeeded
            ScrollBar.vertical.policy: QQC2.ScrollBar.AsNeeded

            QQC2.TextArea {
                id: area
                readOnly: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.NoWrap
                color: Theme.textDim
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle { color: "transparent" }
                // Avoid focus traps — terminal is display-only.
                activeFocusOnTab: false
            }
        }
    }
}
