"""Apply selected package updates via Polkit (pkexec)."""

from __future__ import annotations

import fcntl
import os
from collections.abc import Callable

from cachy_updater.backend import runtime_dir, which
from cachy_updater.models import PackageUpdate, UpdateSource

LogFn = Callable[[str], None]


class UpdateApplyError(RuntimeError):
    pass


def apply_updates(
    packages: list[PackageUpdate],
    *,
    all_repo_packages: list[PackageUpdate] | None = None,
    on_line: LogFn | None = None,
) -> int:
    if not packages:
        raise UpdateApplyError("No packages selected.")

    lock_path = runtime_dir() / "apply.lock"
    lock_fd = os.open(lock_path, os.O_CREAT | os.O_RDWR, 0o644)
    try:
        fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError as exc:
        os.close(lock_fd)
        raise UpdateApplyError(
            "Another Cachy Updater apply is already running."
        ) from exc

    try:
        repo_pkgs = [p.name for p in packages if p.source == UpdateSource.REPO]
        aur_pkgs = [p.name for p in packages if p.source == UpdateSource.AUR]
        flatpak_pkgs = [
            (p.groups[0] if p.groups else p.name)
            for p in packages
            if p.source == UpdateSource.FLATPAK
        ]
        all_repo = [
            p.name
            for p in (all_repo_packages or [])
            if p.source == UpdateSource.REPO
        ]
        # Full sync upgrade when every pending repo package is selected —
        # avoids Arch partial-upgrade breakage.
        full_repo_upgrade = bool(repo_pkgs) and set(repo_pkgs) == set(all_repo)

        exit_code = 0
        if repo_pkgs:
            exit_code = _apply_repo(
                repo_pkgs,
                full_upgrade=full_repo_upgrade,
                on_line=on_line,
            )
            if exit_code != 0:
                return exit_code
        if aur_pkgs:
            exit_code = _apply_aur(aur_pkgs, on_line=on_line)
            if exit_code != 0:
                return exit_code
        if flatpak_pkgs:
            exit_code = _apply_flatpak(flatpak_pkgs, on_line=on_line)
        return exit_code
    finally:
        fcntl.flock(lock_fd, fcntl.LOCK_UN)
        os.close(lock_fd)


def _apply_repo(
    names: list[str],
    *,
    full_upgrade: bool,
    on_line: LogFn | None,
) -> int:
    pkexec = which("pkexec")
    pacman = which("pacman")
    if not pacman:
        raise UpdateApplyError("pacman not found.")
    if not pkexec:
        raise UpdateApplyError(
            "pkexec not found. Install polkit to apply updates from the GUI."
        )

    runtime_dir().joinpath("selected-packages.txt").write_text(
        "\n".join(names) + "\n",
        encoding="utf-8",
    )

    if full_upgrade:
        cmd = [pkexec, pacman, "-Syu", "--noconfirm"]
    else:
        # Explicit subset — caller must have warned about partial upgrades.
        cmd = [pkexec, pacman, "-S", "--noconfirm", "--needed", "--", *names]
    return _stream(cmd, on_line=on_line)


def _apply_aur(names: list[str], *, on_line: LogFn | None) -> int:
    paru = which("paru")
    if not paru:
        raise UpdateApplyError("paru not found for AUR updates.")
    # paru handles its own privilege elevation for package install.
    cmd = [paru, "-S", "--noconfirm", "--needed", "--", *names]
    return _stream(cmd, on_line=on_line)


def _apply_flatpak(app_ids: list[str], *, on_line: LogFn | None) -> int:
    flatpak = which("flatpak")
    if not flatpak:
        raise UpdateApplyError("flatpak not found.")
    # Update selected apps; Flatpak prompts Polkit itself when needed.
    cmd = [flatpak, "update", "-y", "--", *app_ids]
    return _stream(cmd, on_line=on_line)


def _stream(cmd: list[str], *, on_line: LogFn | None) -> int:
    import subprocess

    if on_line:
        on_line("$ " + " ".join(cmd[:4]) + (" …" if len(cmd) > 4 else ""))

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    assert proc.stdout is not None
    for line in proc.stdout:
        if on_line:
            on_line(line.rstrip("\n"))
    return proc.wait()


def needs_reboot(packages: list[PackageUpdate]) -> bool:
    return any(
        p.name.startswith("linux")
        or p.name in {"linux-firmware", "systemd", "glibc"}
        for p in packages
        if p.source == UpdateSource.REPO
    )


def sync_databases(*, on_line: LogFn | None = None) -> int:
    """Optional explicit -Sy before checks (usually checkupdates handles this)."""
    pkexec = which("pkexec")
    pacman = which("pacman")
    if not pkexec or not pacman:
        raise UpdateApplyError("pkexec/pacman required to sync databases.")
    return _stream([pkexec, pacman, "-Sy"], on_line=on_line)
