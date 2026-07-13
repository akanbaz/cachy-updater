"""System tray applet for Cachy Updater."""

from __future__ import annotations

from collections.abc import Callable

from PyQt6.QtCore import QTimer
from PyQt6.QtGui import QAction, QIcon
from PyQt6.QtWidgets import QApplication, QMenu, QSystemTrayIcon

from cachy_updater import __app_name__
from cachy_updater.workers import CheckWorker, start_worker


class TrayController:
    """Owns the tray icon, periodic checks, and window show/hide."""

    def __init__(
        self,
        *,
        show_window: Callable[[], None],
        refresh_window: Callable[[], None],
        get_update_count: Callable[[], int],
        quit_app: Callable[[], None],
    ) -> None:
        self._show_window = show_window
        self._refresh_window = refresh_window
        self._get_update_count = get_update_count
        self._quit_app = quit_app
        self._thread = None
        self._worker = None

        self.tray = QSystemTrayIcon()
        icon = QIcon.fromTheme("system-software-update")
        if icon.isNull():
            icon = QIcon.fromTheme("update-none")
        self.tray.setIcon(icon)
        self.tray.setToolTip(__app_name__)
        self.tray.activated.connect(self._on_activated)

        menu = QMenu()
        open_action = QAction("Open Cachy Updater", menu)
        open_action.triggered.connect(self._show_window)
        menu.addAction(open_action)

        refresh_action = QAction("Check for updates", menu)
        refresh_action.triggered.connect(self.check_now)
        menu.addAction(refresh_action)
        menu.addSeparator()

        quit_action = QAction("Quit", menu)
        quit_action.triggered.connect(self._quit_app)
        menu.addAction(quit_action)
        self.tray.setContextMenu(menu)
        self.tray.show()

        self._timer = QTimer()
        self._timer.setInterval(30 * 60 * 1000)  # 30 minutes
        self._timer.timeout.connect(self.check_now)
        self._timer.start()

        # Quiet background check shortly after start
        QTimer.singleShot(2500, self.check_now)

    @property
    def available(self) -> bool:
        return QSystemTrayIcon.isSystemTrayAvailable()

    def update_tooltip(self, count: int | None = None) -> None:
        n = self._get_update_count() if count is None else count
        if n <= 0:
            self.tray.setToolTip(f"{__app_name__} — up to date")
            icon = QIcon.fromTheme("update-none")
            if icon.isNull():
                icon = QIcon.fromTheme("system-software-update")
            self.tray.setIcon(icon)
        else:
            self.tray.setToolTip(
                f"{__app_name__} — {n} update{'s' if n != 1 else ''} available"
            )
            icon = QIcon.fromTheme("software-update-available")
            if icon.isNull():
                icon = QIcon.fromTheme("system-software-update")
            self.tray.setIcon(icon)

    def notify_updates(self, count: int) -> None:
        self.update_tooltip(count)
        if count <= 0:
            return
        self.tray.showMessage(
            __app_name__,
            f"{count} update{'s' if count != 1 else ''} available",
            QSystemTrayIcon.MessageIcon.Information,
            6000,
        )

    def check_now(self) -> None:
        if self._thread is not None:
            return
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
        from cachy_updater.models import CheckResult

        assert isinstance(result, CheckResult)
        count = len(result.packages)
        previous = self._get_update_count()
        self.notify_updates(count)
        # Refresh open window list when counts change
        if count != previous:
            self._refresh_window()

    def _on_check_failed(self, message: str) -> None:
        self.tray.setToolTip(f"{__app_name__} — check failed")

    def _on_activated(self, reason: QSystemTrayIcon.ActivationReason) -> None:
        if reason in (
            QSystemTrayIcon.ActivationReason.Trigger,
            QSystemTrayIcon.ActivationReason.DoubleClick,
        ):
            self._show_window()
