"""Application entrypoints."""

from __future__ import annotations

import sys

from PyQt6.QtWidgets import QApplication, QSystemTrayIcon

from cachy_updater import __app_id__, __app_name__
from cachy_updater.ui.main_window import MainWindow, center_on_screen
from cachy_updater.ui.tray import TrayController


def main(argv: list[str] | None = None) -> int:
    args = list(argv if argv is not None else sys.argv)
    start_hidden = "--tray" in args
    args = [a for a in args if a != "--tray"]

    app = QApplication(args)
    # Internal name must stay stable for WM_CLASS / portal matching.
    app.setApplicationName("cachy-updater")
    app.setApplicationDisplayName(__app_name__)
    app.setOrganizationName("CachyOS")
    # Must match ~/.local/share/applications/<id>.desktop basename.
    app.setDesktopFileName(__app_id__)
    app.setQuitOnLastWindowClosed(False)

    tray_ok = QSystemTrayIcon.isSystemTrayAvailable()
    window = MainWindow(tray_mode=tray_ok)
    center_on_screen(window)

    tray: TrayController | None = None
    if tray_ok:
        tray = TrayController(
            show_window=lambda: (window.show(), window.raise_(), window.activateWindow()),
            refresh_window=window.refresh,
            get_update_count=window.update_count,
            quit_app=lambda: (window.request_quit(), app.quit()),
        )
        window.updates_changed.connect(tray.update_tooltip)
        window.updates_changed.connect(
            lambda n: tray.notify_updates(n) if n > 0 else None
        )

    if start_hidden and tray_ok:
        # Stay in tray only
        pass
    else:
        window.show()

    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
