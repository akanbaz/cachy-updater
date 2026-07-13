"""Main application window."""

from __future__ import annotations

from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtGui import QFont, QGuiApplication
from PyQt6.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QFrame,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPlainTextEdit,
    QProgressBar,
    QPushButton,
    QSplitter,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from cachy_updater import __app_name__, __version__
from cachy_updater.backend.apply import needs_reboot
from cachy_updater.models import (
    CheckResult,
    PackageUpdate,
    UpdateSeverity,
    UpdateSource,
)
from cachy_updater.ui.care_tab import CareTab
from cachy_updater.ui.styles import APP_STYLESHEET
from cachy_updater.workers import ApplyWorker, CheckWorker, start_worker

SEVERITY_MARK = {
    UpdateSeverity.CRITICAL: "●",
    UpdateSeverity.IMPORTANT: "●",
    UpdateSeverity.NOTICE: "○",
    UpdateSeverity.ROUTINE: "·",
}


def package_key(pkg: PackageUpdate) -> str:
    if pkg.source == UpdateSource.FLATPAK and pkg.groups:
        return f"{pkg.source.value}:{pkg.groups[0]}"
    return f"{pkg.source.value}:{pkg.name}"


class MainWindow(QMainWindow):
    updates_changed = pyqtSignal(int)

    def __init__(self, *, tray_mode: bool = False) -> None:
        super().__init__()
        self.setWindowTitle(f"{__app_name__}")
        self.setMinimumSize(980, 640)
        self.resize(1100, 740)

        self.tray_mode = tray_mode
        self._packages: list[PackageUpdate] = []
        self._thread: QThread | None = None
        self._worker = None
        self._busy = False
        self._quit_on_close = not tray_mode

        self._build_ui()
        self.setStyleSheet(APP_STYLESHEET)
        self._set_status("Ready", "ready")
        self.care_tab.bootstrap()
        self.refresh()

    def _build_ui(self) -> None:
        root = QWidget()
        self.setCentralWidget(root)
        layout = QVBoxLayout(root)
        layout.setContentsMargins(20, 18, 20, 18)
        layout.setSpacing(14)

        header = QHBoxLayout()
        titles = QVBoxLayout()
        title = QLabel(__app_name__)
        title.setObjectName("HeroTitle")
        subtitle = QLabel(
            "Repo, AUR, and Flatpak updates — with clear change summaries."
        )
        subtitle.setObjectName("HeroSubtitle")
        titles.addWidget(title)
        titles.addWidget(subtitle)
        header.addLayout(titles, stretch=1)

        self.status_chip = QLabel("Ready")
        self.status_chip.setObjectName("StatusChip")
        self.status_chip.setProperty("state", "ready")
        self.status_chip.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header.addWidget(self.status_chip, alignment=Qt.AlignmentFlag.AlignTop)
        layout.addLayout(header)

        self.progress = QProgressBar()
        self.progress.setRange(0, 0)
        self.progress.setTextVisible(False)
        self.progress.hide()
        layout.addWidget(self.progress)

        self.tabs = QTabWidget()
        self.tabs.addTab(self._build_updates_tab(), "Updates")
        self.care_tab = CareTab(append_log=self._append_log)
        self.tabs.addTab(self.care_tab, "News & Cleanup")
        layout.addWidget(self.tabs, stretch=1)

        footer = QHBoxLayout()
        self.refresh_btn = QPushButton("Refresh")
        self.refresh_btn.setObjectName("GhostButton")
        self.refresh_btn.clicked.connect(self.refresh)
        footer.addWidget(self.refresh_btn)

        version = QLabel(f"v{__version__}")
        version.setObjectName("MutedLabel")
        footer.addWidget(version)
        footer.addStretch()

        self.apply_btn = QPushButton("Apply selected")
        self.apply_btn.setObjectName("PrimaryButton")
        self.apply_btn.setEnabled(False)
        self.apply_btn.clicked.connect(self.apply_selected)
        footer.addWidget(self.apply_btn)
        layout.addLayout(footer)

    def _build_updates_tab(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        layout.setContentsMargins(0, 8, 0, 0)
        layout.setSpacing(10)

        list_header = QHBoxLayout()
        self.count_label = QLabel("No updates checked yet")
        self.count_label.setObjectName("SectionLabel")
        list_header.addWidget(self.count_label)
        list_header.addStretch()
        self.select_all = QCheckBox("Select all")
        self.select_all.setChecked(True)
        self.select_all.toggled.connect(self._toggle_select_all)
        list_header.addWidget(self.select_all)
        layout.addLayout(list_header)

        shell = QFrame()
        shell.setObjectName("ContentShell")
        shell_layout = QHBoxLayout(shell)
        shell_layout.setContentsMargins(0, 0, 0, 0)
        shell_layout.setSpacing(0)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.setChildrenCollapsible(False)
        splitter.setHandleWidth(1)
        splitter.setObjectName("ContentSplitter")

        left = QWidget()
        left_layout = QVBoxLayout(left)
        left_layout.setContentsMargins(8, 8, 4, 8)
        left_layout.setSpacing(0)

        self.package_list = QListWidget()
        self.package_list.setObjectName("PackageList")
        self.package_list.setSelectionMode(
            QAbstractItemView.SelectionMode.SingleSelection
        )
        self.package_list.currentRowChanged.connect(self._on_select_row)
        self.package_list.itemChanged.connect(self._on_item_changed)
        left_layout.addWidget(self.package_list)
        splitter.addWidget(left)

        right = QFrame()
        right.setObjectName("DetailCard")
        right_layout = QVBoxLayout(right)
        right_layout.setContentsMargins(16, 14, 16, 14)
        right_layout.setSpacing(10)

        self.detail_name = QLabel("Select a package")
        self.detail_name.setObjectName("DetailName")
        self.detail_name.setWordWrap(True)
        right_layout.addWidget(self.detail_name)

        self.detail_meta = QLabel("")
        self.detail_meta.setObjectName("DetailMeta")
        self.detail_meta.setWordWrap(True)
        right_layout.addWidget(self.detail_meta)

        self.detail_summary = QLabel(
            "Change summaries appear here after a refresh."
        )
        self.detail_summary.setObjectName("DetailSummary")
        self.detail_summary.setWordWrap(True)
        self.detail_summary.setAlignment(Qt.AlignmentFlag.AlignTop)
        right_layout.addWidget(self.detail_summary, stretch=1)

        log_label = QLabel("Activity")
        log_label.setObjectName("SectionLabel")
        right_layout.addWidget(log_label)
        self.log_view = QPlainTextEdit()
        self.log_view.setObjectName("LogView")
        self.log_view.setReadOnly(True)
        self.log_view.setMaximumBlockCount(4000)
        self.log_view.setPlaceholderText("Update output will stream here…")
        right_layout.addWidget(self.log_view, stretch=1)

        splitter.addWidget(right)
        splitter.setStretchFactor(0, 5)
        splitter.setStretchFactor(1, 6)
        splitter.setSizes([420, 560])
        shell_layout.addWidget(splitter)
        layout.addWidget(shell, stretch=1)
        return page

    def update_count(self) -> int:
        return len(self._packages)

    def _set_status(self, text: str, state: str) -> None:
        self.status_chip.setText(text)
        self.status_chip.setProperty("state", state)
        self.status_chip.style().unpolish(self.status_chip)
        self.status_chip.style().polish(self.status_chip)

    def _set_busy(self, busy: bool, message: str = "Working…") -> None:
        self._busy = busy
        self.refresh_btn.setEnabled(not busy)
        self.select_all.setEnabled(not busy)
        self.package_list.setEnabled(not busy)
        if busy:
            self.apply_btn.setEnabled(False)
            self.progress.show()
            self._set_status(message, "busy")
        else:
            self.progress.hide()
            self._update_apply_enabled()

    def _cleanup_thread(self) -> None:
        if self._thread is not None:
            self._thread.quit()
            self._thread.wait(5000)
            self._thread = None
            self._worker = None

    def refresh(self) -> None:
        if self._busy:
            return
        self._set_busy(True, "Checking…")
        self._append_log("Checking for updates (repo / AUR / Flatpak)…")
        worker = CheckWorker(include_aur=True, include_flatpak=True)
        thread = start_worker(worker)
        worker.finished.connect(self._on_check_finished)
        worker.failed.connect(self._on_check_failed)
        worker.finished.connect(thread.quit)
        worker.failed.connect(thread.quit)
        thread.finished.connect(worker.deleteLater)
        thread.finished.connect(thread.deleteLater)
        thread.finished.connect(lambda: setattr(self, "_thread", None))
        self._thread = thread
        self._worker = worker
        thread.start()

    def _on_check_finished(self, result: object) -> None:
        assert isinstance(result, CheckResult)
        self._set_busy(False)
        for warning in result.warnings:
            self._append_log(f"Warning: {warning}")
        for error in result.errors:
            self._append_log(f"Error: {error}")

        if result.errors and not result.packages:
            self._set_status("Check failed", "error")
            self.count_label.setText("Could not check for updates")
            self.updates_changed.emit(0)
            QMessageBox.warning(
                self,
                "Update check failed",
                "\n".join(result.errors),
            )
            return

        self._packages = result.packages
        self._populate_list()
        self.updates_changed.emit(len(self._packages))
        if not self._packages:
            self._set_status("Up to date", "ready")
            self.count_label.setText("System is up to date")
            self.detail_name.setText("Nothing to update")
            self.detail_meta.setText("")
            self.detail_summary.setText(
                "Repo, AUR, and Flatpak sources report no pending updates."
            )
            self._append_log("No updates available.")
        else:
            state = "warn" if any(
                p.severity
                in (UpdateSeverity.CRITICAL, UpdateSeverity.IMPORTANT)
                for p in self._packages
            ) else "ready"
            self._set_status(f"{len(self._packages)} updates", state)
            self.count_label.setText(
                f"{len(self._packages)} update"
                f"{'' if len(self._packages) == 1 else 's'} available"
            )
            self._append_log(f"Found {len(self._packages)} update(s).")
            self.package_list.setCurrentRow(0)

    def _on_check_failed(self, message: str) -> None:
        self._set_busy(False)
        self._set_status("Check failed", "error")
        self._append_log(f"Error: {message}")
        self.updates_changed.emit(0)
        QMessageBox.critical(self, "Update check failed", message)

    def _populate_list(self) -> None:
        self.package_list.blockSignals(True)
        self.package_list.clear()
        for pkg in self._packages:
            item = QListWidgetItem(
                f"{SEVERITY_MARK[pkg.severity]}  {pkg.name}\n"
                f"    {pkg.source.value} · {pkg.version_delta}"
            )
            item.setFlags(
                item.flags()
                | Qt.ItemFlag.ItemIsUserCheckable
                | Qt.ItemFlag.ItemIsSelectable
                | Qt.ItemFlag.ItemIsEnabled
            )
            item.setCheckState(
                Qt.CheckState.Checked
                if pkg.selected
                else Qt.CheckState.Unchecked
            )
            item.setData(Qt.ItemDataRole.UserRole, package_key(pkg))
            font = QFont(item.font())
            if pkg.severity in (
                UpdateSeverity.CRITICAL,
                UpdateSeverity.IMPORTANT,
            ):
                font.setBold(True)
            item.setFont(font)
            self.package_list.addItem(item)
        self.package_list.blockSignals(False)
        self.select_all.blockSignals(True)
        self.select_all.setChecked(
            bool(self._packages) and all(p.selected for p in self._packages)
        )
        self.select_all.blockSignals(False)
        self._update_apply_enabled()

    def _package_by_key(self, key: str) -> PackageUpdate | None:
        for pkg in self._packages:
            if package_key(pkg) == key:
                return pkg
        return None

    def _on_select_row(self, row: int) -> None:
        if row < 0 or row >= len(self._packages):
            return
        pkg = self._packages[row]
        self.detail_name.setText(pkg.name)
        meta_bits = [
            pkg.version_delta,
            pkg.source.value.upper(),
            pkg.severity.value.capitalize(),
        ]
        if pkg.repo:
            meta_bits.insert(1, pkg.repo)
        if pkg.size_label:
            meta_bits.append(pkg.size_label)
        self.detail_meta.setText("  ·  ".join(meta_bits))
        self.detail_summary.setText(pkg.summary)

    def _on_item_changed(self, item: QListWidgetItem) -> None:
        key = item.data(Qt.ItemDataRole.UserRole)
        pkg = self._package_by_key(key)
        if not pkg:
            return
        pkg.selected = item.checkState() == Qt.CheckState.Checked
        self._update_apply_enabled()

    def _toggle_select_all(self, checked: bool) -> None:
        for pkg in self._packages:
            pkg.selected = checked
        self.package_list.blockSignals(True)
        for i in range(self.package_list.count()):
            item = self.package_list.item(i)
            item.setCheckState(
                Qt.CheckState.Checked if checked else Qt.CheckState.Unchecked
            )
        self.package_list.blockSignals(False)
        self._update_apply_enabled()

    def _selected_packages(self) -> list[PackageUpdate]:
        return [p for p in self._packages if p.selected]

    def _update_apply_enabled(self) -> None:
        self.apply_btn.setEnabled(
            (not self._busy) and bool(self._selected_packages())
        )

    def apply_selected(self) -> None:
        selected = self._selected_packages()
        if not selected or self._busy:
            return

        critical = [
            p.name
            for p in selected
            if p.severity
            in (UpdateSeverity.CRITICAL, UpdateSeverity.IMPORTANT)
        ]
        all_repo = [p for p in self._packages if p.source == UpdateSource.REPO]
        selected_repo = [p for p in selected if p.source == UpdateSource.REPO]
        partial = bool(all_repo) and set(p.name for p in selected_repo) != set(
            p.name for p in all_repo
        )

        message = (
            f"Apply {len(selected)} package update"
            f"{'' if len(selected) == 1 else 's'}?\n\n"
            "You will be asked to authenticate via Polkit when needed."
        )
        if not partial and selected_repo:
            message += (
                "\n\nRepository packages will be upgraded with "
                "`pacman -Syu` (full sync — recommended)."
            )
        if partial:
            message += (
                "\n\nWarning: you deselected some repository packages. "
                "On Arch/CachyOS this can cause a partial upgrade and "
                "break the system. Continue only if you know you need this."
            )
        if critical:
            message += (
                "\n\nImportant packages included:\n• "
                + "\n• ".join(critical[:12])
            )
            if len(critical) > 12:
                message += f"\n• …and {len(critical) - 12} more"

        confirm = QMessageBox.question(
            self,
            "Confirm updates",
            message,
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if confirm != QMessageBox.StandardButton.Yes:
            return

        self._set_busy(True, "Applying…")
        self._append_log(f"Applying {len(selected)} package(s)…")
        worker = ApplyWorker(selected, all_repo_packages=all_repo)
        thread = start_worker(worker)
        worker.line.connect(self._append_log)
        worker.finished.connect(self._on_apply_finished)
        worker.failed.connect(self._on_apply_failed)
        worker.finished.connect(thread.quit)
        worker.failed.connect(thread.quit)
        thread.finished.connect(worker.deleteLater)
        thread.finished.connect(thread.deleteLater)
        thread.finished.connect(lambda: setattr(self, "_thread", None))
        self._thread = thread
        self._worker = worker
        thread.start()

    def _on_apply_finished(self, code: int, packages: object) -> None:
        self._set_busy(False)
        assert isinstance(packages, list)
        if code == 0:
            self._set_status("Updated", "ready")
            self._append_log("Updates applied successfully.")
            if needs_reboot(packages):
                QMessageBox.information(
                    self,
                    "Reboot recommended",
                    "Core packages were updated. A reboot is recommended "
                    "so the running system matches the installed packages.",
                )
            else:
                QMessageBox.information(
                    self,
                    "Updates complete",
                    "Selected packages were updated successfully.",
                )
            self.refresh()
            self.care_tab.refresh_maintenance()
        else:
            self._set_status("Apply failed", "error")
            self._append_log(f"Apply exited with code {code}.")
            QMessageBox.warning(
                self,
                "Update failed",
                f"The package manager exited with code {code}.\n"
                "Check the activity log for details.",
            )

    def _on_apply_failed(self, message: str) -> None:
        self._set_busy(False)
        self._set_status("Apply failed", "error")
        self._append_log(f"Error: {message}")
        QMessageBox.critical(self, "Update failed", message)

    def _append_log(self, line: str) -> None:
        self.log_view.appendPlainText(line)
        bar = self.log_view.verticalScrollBar()
        bar.setValue(bar.maximum())

    def request_quit(self) -> None:
        self._quit_on_close = True
        self.close()

    def closeEvent(self, event) -> None:  # noqa: N802
        if self._busy:
            QMessageBox.warning(
                self,
                "Busy",
                "Please wait for the current operation to finish.",
            )
            event.ignore()
            return
        if self.tray_mode and not self._quit_on_close:
            event.ignore()
            self.hide()
            return
        self._cleanup_thread()
        super().closeEvent(event)


def center_on_screen(window: QMainWindow) -> None:
    screen = QGuiApplication.primaryScreen()
    if not screen:
        return
    geo = window.frameGeometry()
    geo.moveCenter(screen.availableGeometry().center())
    window.move(geo.topLeft())
