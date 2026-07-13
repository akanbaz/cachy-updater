"""Orphan packages and pacman cache maintenance."""

from __future__ import annotations

import re
from collections.abc import Callable
from dataclasses import dataclass, field

from cachy_updater.backend import run_cmd, which

LogFn = Callable[[str], None]
CANDIDATE_RE = re.compile(r":\s*(\d+)\s+candidate", re.IGNORECASE)


@dataclass(slots=True)
class MaintainSnapshot:
    orphans: list[str] = field(default_factory=list)
    cache_old: int = 0
    cache_uninstalled: int = 0
    warnings: list[str] = field(default_factory=list)

    @property
    def cache_total(self) -> int:
        return self.cache_old + self.cache_uninstalled


class MaintainError(RuntimeError):
    pass


def scan_maintenance(*, keep_old: int = 3, keep_uninstalled: int = 0) -> MaintainSnapshot:
    snap = MaintainSnapshot()
    proc = run_cmd(["pacman", "-Qtdq"], timeout=30)
    if proc.returncode == 0 and proc.stdout.strip():
        snap.orphans = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
    elif proc.returncode not in (0, 1):
        snap.warnings.append((proc.stderr or "pacman -Qtdq failed").strip())

    if which("paccache"):
        snap.cache_old = _paccache_candidates(["-dk", str(keep_old)])
        snap.cache_uninstalled = _paccache_candidates(
            ["-duk", str(keep_uninstalled)]
        )
    else:
        snap.warnings.append("paccache not found (install pacman-contrib).")
    return snap


def remove_orphans(*, on_line: LogFn | None = None) -> int:
    orphans = scan_maintenance().orphans
    if not orphans:
        raise MaintainError("No orphan packages to remove.")
    pkexec = which("pkexec")
    pacman = which("pacman")
    if not pkexec or not pacman:
        raise MaintainError("pkexec/pacman required to remove orphans.")
    return _stream([pkexec, pacman, "-Rns", "--noconfirm", "--", *orphans], on_line)


def clean_cache(
    *,
    keep_old: int = 3,
    keep_uninstalled: int = 0,
    on_line: LogFn | None = None,
) -> int:
    pkexec = which("pkexec")
    paccache = which("paccache")
    if not pkexec or not paccache:
        raise MaintainError("pkexec/paccache required to clean the cache.")

    code = 0
    # Keep N versions of installed packages
    code = _stream(
        [pkexec, paccache, "-rk", str(keep_old)],
        on_line=on_line,
    )
    if code != 0:
        return code
    # Remove cache for uninstalled packages (keep 0 by default)
    return _stream(
        [pkexec, paccache, "-ruk", str(keep_uninstalled)],
        on_line=on_line,
    )


def _paccache_candidates(args: list[str]) -> int:
    proc = run_cmd(["paccache", *args], timeout=60)
    text = (proc.stdout or "") + "\n" + (proc.stderr or "")
    match = CANDIDATE_RE.search(text)
    if match:
        return int(match.group(1))
    return 0


def _stream(cmd: list[str], on_line: LogFn | None) -> int:
    import subprocess

    if on_line:
        on_line("$ " + " ".join(cmd[:5]) + (" …" if len(cmd) > 5 else ""))
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
