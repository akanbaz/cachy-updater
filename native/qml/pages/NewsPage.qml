import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.cachyos.updater

ColumnLayout {
    id: page
    spacing: Theme.spacing

    property int selectedIndex: 0

    Banner { text: News.warningText }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSmall
        QQC2.Label {
            Layout.fillWidth: true
            text: News.busy ? "Fetching news\u2026"
                            : News.count + (News.count === 1 ? " article" : " articles")
            color: Theme.textDim
            font.family: Theme.sansFamily
            font.pixelSize: 13
        }
        QQC2.Button {
            text: "Refresh"
            icon.name: "view-refresh"
            enabled: !News.busy
            onClicked: News.refresh()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Theme.spacing

        // ---- List --------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: Theme.surface
            radius: Theme.radius
            clip: true

            ListView {
                id: newsList
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                model: News.newsModel
                clip: true
                currentIndex: page.selectedIndex

                delegate: Item {
                    width: ListView.view.width
                    implicitHeight: entry.implicitHeight + Theme.spacing

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: Theme.radiusSmall
                        color: page.selectedIndex === index ? Theme.rowHover
                             : itemHover.hovered ? Theme.surfaceHover : "transparent"
                    }
                    HoverHandler { id: itemHover }
                    TapHandler { onTapped: page.selectedIndex = index }

                    ColumnLayout {
                        id: entry
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: Theme.spacingSmall
                        anchors.rightMargin: Theme.spacingSmall
                        spacing: 2

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: model.title
                            color: Theme.text
                            font.family: Theme.sansFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            spacing: Theme.spacingSmall
                            QQC2.Label {
                                text: model.source
                                color: model.source === "CachyOS" ? Theme.cyan : Theme.textMuted
                                font.family: Theme.sansFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                            QQC2.Label {
                                text: model.published
                                color: Theme.textFaint
                                font.family: Theme.sansFamily
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Theme.spacing * 2
                    visible: News.count === 0 && !News.busy
                    icon.name: "news-subscribe"
                    text: "No news"
                }
            }
        }

        // ---- Preview -----------------------------------------------
        CachyCard {
            id: previewCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: Theme.spacingLarge

            readonly property var item: News.count > 0 ? News.itemAt(page.selectedIndex) : ({})
            readonly property string curTitle: item.title || ""
            readonly property string curSummary: item.summary || ""
            readonly property string curPublished: item.published || ""
            readonly property string curSource: item.source || ""
            readonly property string curLink: item.link || ""

            QQC2.Label {
                Layout.fillWidth: true
                text: previewCard.curTitle.length > 0 ? previewCard.curTitle : "Select an article"
                color: Theme.text
                font.family: Theme.sansFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }
            RowLayout {
                spacing: Theme.spacingSmall
                visible: previewCard.curTitle.length > 0
                QQC2.Label {
                    text: previewCard.curSource
                    color: previewCard.curSource === "CachyOS" ? Theme.cyan : Theme.textMuted
                    font.family: Theme.sansFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                QQC2.Label {
                    text: previewCard.curPublished
                    color: Theme.textFaint
                    font.family: Theme.sansFamily
                    font.pixelSize: 12
                }
            }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }
            QQC2.ScrollView {
                id: summaryScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
                QQC2.Label {
                    width: summaryScroll.availableWidth
                    text: previewCard.curSummary
                    color: Theme.textDim
                    font.family: Theme.sansFamily
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }
            QQC2.Button {
                text: "Open in browser"
                icon.name: "internet-web-browser"
                enabled: previewCard.curLink.length > 0
                onClicked: Qt.openUrlExternally(previewCard.curLink)
            }
        }
    }
}
