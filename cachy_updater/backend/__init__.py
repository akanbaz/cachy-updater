"""Backend helpers: timeouts, subprocess, locking."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

DEFAULT_TIMEOUT = 45


def run_cmd(
    args: list[str],
    *,
    timeout: float = DEFAULT_TIMEOUT,
    env: dict[str, str] | None = None,
    input_text: str | None = None,
) -> subprocess.CompletedProcess[str]:
    merged = os.environ.copy()
    if env:
        merged.update(env)
    return subprocess.run(
        args,
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
        env=merged,
        input=input_text,
    )


def state_dir() -> Path:
    base = os.environ.get("XDG_STATE_HOME")
    if base:
        path = Path(base) / "cachy-updater"
    else:
        path = Path.home() / ".local" / "state" / "cachy-updater"
    path.mkdir(parents=True, exist_ok=True)
    return path


def runtime_dir() -> Path:
    base = os.environ.get("XDG_RUNTIME_DIR")
    if base:
        path = Path(base) / "cachy-updater"
    else:
        path = Path("/tmp") / f"cachy-updater-{os.getuid()}"
    path.mkdir(parents=True, exist_ok=True)
    return path


def which(cmd: str) -> str | None:
    from shutil import which as _which

    return _which(cmd)
