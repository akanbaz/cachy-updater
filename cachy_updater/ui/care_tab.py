"""News + orphans/cache maintenance tab."""

from __future__ import annotations

from PyQt6.QtCore import Qt, QThread, QUrl
from PyQt6.QtGui import QDesktopServices
from PyQt6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMessageBox,
    QPlainTextEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from cachy_updater.backend.maintain import MaintainSnapshot
from cachy_updater.backend.news import NewsItem, NewsResult
from cachy_updater.workers import (
    CacheCleanWorker,
    MaintainScanWorker,
    NewsWorker,
    OrphanRemoveWorker,
    start_worker,
)


class CareTab(QWidget):
    """Combined news + cleanup surface."""

    def __init__(self, *, append_log) -> None:
        super().__init__()
        self._append_log = append_log
        self._news: list[NewsItem] = []
        self._snap = MaintainSnapshot()
        self._thread: QThread | None = None
        self._worker = None
        self._busy = False
        self._build()

    def _build(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 8, 0, 0)
        root.setSpacing(12)

        news_card = QFrame()
        news_card.setObjectName("DetailCard")
        news_layout = QVBoxLayout(news_card)
        news_header = QHBoxLayout()
        news_title = QLabel("News")
        news_title.setObjectName("DetailName")
        news_header.addWidget(news_title)
        news_header.addStretch()
        self.news_refresh_btn = QPushButton("Refresh news")
        self.news_refresh_btn.setObjectName("GhostButton")
        self.news_refresh_btn.clicked.connect(self.refresh_news)
        news_header.addWidget(self.news_refresh_btn)
        news_layout.addLayout(news_header)

        hint = QLabel("Read before updating — especially recent Arch posts.")
        hint.setObjectName("MutedLabel")
        news_layout.addWidget(hint)

        self.news_list = QListWidget()
        self.news_list.setObjectName("PackageList")
        self.news_list.itemActivated.connect(self._open_news_item)
        self.news_list.currentRowChanged.connect(self._on_news_row)
        news_layout.addWidget(self.news_list)

        self.news_summary = QLabel("Select a headline for a short preview.")
        self.news_summary.setObjectName("DetailSummary")
        self.news_summary.setWordWrap(True)
        news_layout.addWidget(self.news_summary)
        root.addWidget(news_card, stretch=3)

        care_card = QFrame()
        care_card.setObjectName("DetailCard")
        care_layout = QVBoxLayout(care_card)
        care_title = QLabel("Cleanup")
        care_title.setObjectName("DetailName")
        care_layout.addWidget(care_title)

        self.orphan_label = QLabel("Orphans: scanning…")
        self.orphan_label.setObjectName("DetailSummary")
        self.orphan_label.setWordWrap(True)
        care_layout.addWidget(self.orphan_label)

        self.cache_label = QLabel("Cache: scanning…")
        self.cache_label.setObjectName("DetailSummary")
        self.cache_label.setWordWrap(True)
        care_layout.addWidget(self.cache_label)

        actions = QHBoxLayout()
        self.scan_btn = QPushButton("Rescan")
        self.scan_btn.setObjectName("GhostButton")
        self.scan_btn.clicked.connect(self.refresh_maintenance)
        actions.addWidget(self.scan_btn)

        self.orphan_btn = QPushButton("Remove orphans")
        self.orphan_btn.setObjectName("GhostButton")
        self.orphan_btn.clicked.connect(self.remove_orphans)
        actions.addWidget(self.orphan_btn)

        self.cache_btn = QPushButton("Clean package cache")
        self.cache_btn.setObjectName("PrimaryButton")
        self.cache_btn.clicked.connect(self.clean_cache)
        actions.addWidget(self.cache_btn)
        actions.addStretch()
        care_layout.addLayout(actions)

        self.care_log = QPlainTextEdit()
        self.care_log.setObjectName("LogView")
        self.care_log.setReadOnly(True)
        self.care_log.setMaximumBlockCount(2000)
        self.care_log.setPlaceholderText("Cleanup output…")
        self.care_log.setMaximumHeight(140)
        care_layout.addWidget(self.care_log)
        root.addWidget(care_card, stretch=2)

    def bootstrap(self) -> None:
        self.refresh_news()
        self.refresh_maintenance()

    def _set_busy(self, busy: bool) -> None:
        self._busy = busy
        for btn in (
            self.news_refresh_btn,
            self.scan_btn,
            self.orphan_btn,
            self.cache_btn,
        ):
            btn.setEnabled(not busy)

    def _start(self, worker, on_finished, on_failed, line_signal=False) -> None:
        if self._busy:
            return
        self._set_busy(True)
        thread = start_worker(worker)
        if line_signal:
            worker.line.connect(self._care_log_line)
        worker.finished.connect(on_finished)
        worker.failed.connect(on_failed)
        worker.finished.connect(thread.quit)
        worker.failed.connect(thread.quit)
        thread.finished.connect(worker.deleteLater)
        thread.finished.connect(thread.deleteLater)
        thread.finished.connect(lambda: setattr(self, "_thread", None))
        self._thread = thread
        self._worker = worker
        thread.start()

    def refresh_news(self) -> None:
        self._start(NewsWorker(), self._on_news, self._on_fail)

    def refresh_maintenance(self) -> None:
        self._start(MaintainScanWorker(), self._on_scan, self._on_fail)

    def _on_news(self, result: object) -> None:
        self._set_busy(False)
        assert isinstance(result, NewsResult)
        for w in result.warnings:
            self._append_log(f"News: {w}")
        self._news = result.items
        self.news_list.clear()
        for item in self._news:
            label = f"[{item.source}] {item.title}"
            if item.published:
                label = f"{label}  ·  {item.published}"
            row = QListWidgetItem(label)
            row.setData(Qt.ItemDataRole.UserRole, item.link)
            self.news_list.addItem(row)
        if self._news:
            self.news_list.setCurrentRow(0)
        else:
            self.news_summary.setText("No news items available right now.")

    def _on_news_row(self, row: int) -> None:
        if row < 0 or row >= len(self._news):
            return
        item = self._news[row]
        preview = item.summary or "Open the full article in your browser."
        self.news_summary.setText(preview)

    def _open_news_item(self, item: QListWidgetItem) -> None:
        link = item.data(Qt.ItemDataRole.UserRole)
        if link:
            QDesktopServices.openUrl(QUrl(link))

    def _on_scan(self, snap: object) -> None:
        self._set_busy(False)
        assert isinstance(snap, MaintainSnapshot)
        self._snap = snap
        for w in snap.warnings:
            self._care_log_line(f"Warning: {w}")
        if snap.orphans:
            shown = ", ".join(snap.orphans[:8])
            more = f" (+{len(snap.orphans) - 8} more)" if len(snap.orphans) > 8 else ""
            self.orphan_label.setText(
                f"Orphans: {len(snap.orphans)} — {shown}{more}"
            )
        else:
            self.orphan_label.setText("Orphans: none found")
        self.cache_label.setText(
            f"Cache: {snap.cache_old} old + {snap.cache_uninstalled} "
            f"uninstalled package(s) can be cleaned "
            f"(keeps 3 installed versions)."
        )
        self.orphan_btn.setEnabled(bool(snap.orphans))
        self.cache_btn.setEnabled(snap.cache_total > 0)

    def remove_orphans(self) -> None:
        if not self._snap.orphans:
            return
        confirm = QMessageBox.question(
            self,
            "Remove orphans",
            f"Remove {len(self._snap.orphans)} orphan package(s) with "
            "`pacman -Rns`?\n\nPolkit authentication will be required.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if confirm != QMessageBox.StandardButton.Yes:
            return
        self._care_log_line("Removing orphans…")
        self._start(
            OrphanRemoveWorker(),
            self._on_action_done,
            self._on_fail,
            line_signal=True,
        )

    def clean_cache(self) -> None:
        if self._snap.cache_total <= 0:
            return
        confirm = QMessageBox.question(
            self,
            "Clean cache",
            "Clean old/uninstalled packages from the pacman cache?\n"
            "Keeps the 3 newest versions of installed packages.\n\n"
            "Polkit authentication will be required.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if confirm != QMessageBox.StandardButton.Yes:
            return
        self._care_log_line("Cleaning package cache…")
        self._start(
            CacheCleanWorker(),
            self._on_action_done,
            self._on_fail,
            line_signal=True,
        )

    def _on_action_done(self, code: int) -> None:
        self._set_busy(False)
        if code == 0:
            self._care_log_line("Done.")
            self.refresh_maintenance()
        else:
            QMessageBox.warning(
                self,
                "Cleanup failed",
                f"Command exited with code {code}. See the log for details.",
            )

    def _on_fail(self, message: str) -> None:
        self._set_busy(False)
        self._care_log_line(f"Error: {message}")
        QMessageBox.warning(self, "Care tab", message)

    def _care_log_line(self, line: str) -> None:
        self.care_log.appendPlainText(line)
        self._append_log(line)
        bar = self.care_log.verticalScrollBar()
        bar.setValue(bar.maximum())
