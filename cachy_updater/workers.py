"""Background workers so the UI never blocks on pacman/network."""

from __future__ import annotations

from PyQt6.QtCore import QObject, QThread, pyqtSignal

from cachy_updater.backend.apply import UpdateApplyError, apply_updates
from cachy_updater.backend.check import check_updates
from cachy_updater.backend.maintain import (
    MaintainError,
    clean_cache,
    remove_orphans,
    scan_maintenance,
)
from cachy_updater.backend.news import fetch_news
from cachy_updater.models import PackageUpdate


class CheckWorker(QObject):
    finished = pyqtSignal(object)  # CheckResult
    failed = pyqtSignal(str)

    def __init__(
        self,
        include_aur: bool = True,
        include_flatpak: bool = True,
    ) -> None:
        super().__init__()
        self.include_aur = include_aur
        self.include_flatpak = include_flatpak

    def run(self) -> None:
        try:
            result = check_updates(
                include_aur=self.include_aur,
                include_flatpak=self.include_flatpak,
            )
            self.finished.emit(result)
        except Exception as exc:  # noqa: BLE001 — surface to UI
            self.failed.emit(str(exc))


class ApplyWorker(QObject):
    line = pyqtSignal(str)
    finished = pyqtSignal(int, object)  # exit_code, packages
    failed = pyqtSignal(str)

    def __init__(
        self,
        packages: list[PackageUpdate],
        *,
        all_repo_packages: list[PackageUpdate] | None = None,
    ) -> None:
        super().__init__()
        self.packages = packages
        self.all_repo_packages = all_repo_packages or []

    def run(self) -> None:
        try:
            code = apply_updates(
                self.packages,
                all_repo_packages=self.all_repo_packages,
                on_line=self.line.emit,
            )
            self.finished.emit(code, self.packages)
        except UpdateApplyError as exc:
            self.failed.emit(str(exc))
        except Exception as exc:  # noqa: BLE001
            self.failed.emit(str(exc))


class NewsWorker(QObject):
    finished = pyqtSignal(object)
    failed = pyqtSignal(str)

    def run(self) -> None:
        try:
            self.finished.emit(fetch_news(limit=10))
        except Exception as exc:  # noqa: BLE001
            self.failed.emit(str(exc))


class MaintainScanWorker(QObject):
    finished = pyqtSignal(object)
    failed = pyqtSignal(str)

    def run(self) -> None:
        try:
            self.finished.emit(scan_maintenance())
        except Exception as exc:  # noqa: BLE001
            self.failed.emit(str(exc))


class OrphanRemoveWorker(QObject):
    line = pyqtSignal(str)
    finished = pyqtSignal(int)
    failed = pyqtSignal(str)

    def run(self) -> None:
        try:
            self.finished.emit(remove_orphans(on_line=self.line.emit))
        except MaintainError as exc:
            self.failed.emit(str(exc))
        except Exception as exc:  # noqa: BLE001
            self.failed.emit(str(exc))


class CacheCleanWorker(QObject):
    line = pyqtSignal(str)
    finished = pyqtSignal(int)
    failed = pyqtSignal(str)

    def run(self) -> None:
        try:
            self.finished.emit(clean_cache(on_line=self.line.emit))
        except MaintainError as exc:
            self.failed.emit(str(exc))
        except Exception as exc:  # noqa: BLE001
            self.failed.emit(str(exc))


def start_worker(worker: QObject, slot_name: str = "run") -> QThread:
    thread = QThread()
    worker.moveToThread(thread)
    thread.started.connect(getattr(worker, slot_name))
    return thread
