"""Application styling — Plasma-friendly, readable on dark/light themes."""

from __future__ import annotations

APP_STYLESHEET = """
QMainWindow, QDialog {
    background: palette(window);
}

QTabWidget::pane {
    border: 1px solid rgba(127, 127, 127, 0.28);
    border-radius: 12px;
    top: -1px;
    background: palette(base);
    padding: 10px;
}

QTabBar::tab {
    background: transparent;
    color: palette(window-text);
    padding: 8px 16px;
    margin-right: 4px;
    border-radius: 8px;
}

QTabBar::tab:selected {
    background: rgba(31, 163, 146, 0.20);
    font-weight: 600;
}

QTabBar::tab:hover:!selected {
    background: rgba(127, 127, 127, 0.12);
}

#HeroTitle {
    font-size: 22px;
    font-weight: 650;
    letter-spacing: -0.2px;
    color: palette(window-text);
}

#HeroSubtitle {
    color: palette(text);
    font-size: 13px;
    opacity: 0.75;
}

#SectionLabel {
    color: palette(window-text);
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 0.2px;
    padding-bottom: 2px;
}

#MutedLabel {
    color: palette(window-text);
    font-size: 12px;
}

#StatusChip {
    padding: 5px 12px;
    border-radius: 8px;
    background: rgba(127, 127, 127, 0.18);
    color: palette(window-text);
    font-size: 12px;
    font-weight: 600;
}

#StatusChip[state="ready"] {
    background: rgba(46, 180, 130, 0.28);
    color: #7decc0;
}

#StatusChip[state="busy"] {
    background: rgba(56, 160, 210, 0.28);
    color: #8fd0f0;
}

#StatusChip[state="warn"] {
    background: rgba(220, 160, 50, 0.28);
    color: #f0c878;
}

#StatusChip[state="error"] {
    background: rgba(220, 80, 80, 0.28);
    color: #f0a0a0;
}

#ContentShell {
    border: 1px solid rgba(127, 127, 127, 0.28);
    border-radius: 12px;
    background: palette(base);
}

#PaneDivider {
    background: rgba(127, 127, 127, 0.35);
    max-width: 1px;
    min-width: 1px;
}

QSplitter::handle:horizontal {
    background: rgba(127, 127, 127, 0.35);
    width: 1px;
    margin: 12px 0;
}

#PackageList {
    border: none;
    border-radius: 0;
    padding: 6px 8px;
    background: transparent;
    outline: none;
}

#PackageList::item {
    padding: 10px 12px;
    border-radius: 8px;
    margin: 2px 0;
}

#PackageList::item:selected {
    background: rgba(32, 160, 150, 0.24);
}

#PackageList::item:hover:!selected {
    background: rgba(127, 127, 127, 0.10);
}

#DetailCard {
    border: none;
    background: transparent;
    padding: 4px 4px 4px 8px;
}

#DetailName {
    font-size: 18px;
    font-weight: 650;
    color: palette(window-text);
}

#DetailMeta {
    color: palette(window-text);
    font-size: 12px;
}

#DetailSummary {
    font-size: 13px;
    line-height: 1.4;
    color: palette(window-text);
}

#EmptyHint {
    color: palette(window-text);
    font-size: 13px;
}

#LogView {
    font-family: "JetBrains Mono", "Noto Sans Mono", "Fira Code", monospace;
    font-size: 12px;
    border: 1px solid rgba(127, 127, 127, 0.28);
    border-radius: 10px;
    padding: 8px;
    background: rgba(127, 127, 127, 0.06);
    color: palette(window-text);
}

QPushButton#PrimaryButton {
    background: #1fa392;
    color: white;
    border: none;
    border-radius: 8px;
    padding: 9px 18px;
    font-weight: 600;
}

QPushButton#PrimaryButton:hover {
    background: #18b39f;
}

QPushButton#PrimaryButton:disabled {
    background: rgba(127, 127, 127, 0.28);
    color: rgba(255, 255, 255, 0.45);
}

QPushButton#GhostButton {
    border: 1px solid rgba(127, 127, 127, 0.35);
    border-radius: 8px;
    padding: 9px 16px;
    background: transparent;
    color: palette(window-text);
}

QPushButton#GhostButton:hover {
    background: rgba(127, 127, 127, 0.12);
}

QProgressBar {
    border: none;
    border-radius: 5px;
    background: rgba(127, 127, 127, 0.18);
    text-align: center;
    max-height: 8px;
}

QProgressBar::chunk {
    border-radius: 5px;
    background: #1fa392;
}

QCheckBox {
    color: palette(window-text);
    spacing: 8px;
}
"""
