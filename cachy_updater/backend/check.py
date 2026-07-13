"""Detect available updates and build change summaries."""

from __future__ import annotations

import re
import shutil
import tempfile
from pathlib import Path

from cachy_updater.backend import run_cmd, which
from cachy_updater.backend.summaries import enrich_package
from cachy_updater.models import CheckResult, PackageUpdate, UpdateSource

CHECKUPDATES_TIMEOUT = 60
LINE_RE = re.compile(
    r"^(?P<name>\S+)\s+(?P<old>\S+)\s+->\s+(?P<new>\S+)\s*$"
)


def check_updates(
    *,
    include_aur: bool = True,
    include_flatpak: bool = True,
) -> CheckResult:
    result = CheckResult()

    if not which("checkupdates"):
        result.errors.append(
            "checkupdates not found. Install pacman-contrib."
        )
        return result

    try:
        packages, warnings = _check_repo_packages()
        result.packages.extend(packages)
        result.warnings.extend(warnings)
    except TimeoutError:
        result.errors.append("Timed out while checking repository updates.")
        return result
    except OSError as exc:
        result.errors.append(f"Failed to check repository updates: {exc}")
        return result

    if include_aur and which("paru"):
        try:
            aur_pkgs, aur_warnings = _check_aur_packages()
            result.packages.extend(aur_pkgs)
            result.warnings.extend(aur_warnings)
        except TimeoutError:
            result.warnings.append("Timed out while checking AUR updates.")
        except OSError as exc:
            result.warnings.append(f"AUR check failed: {exc}")
    elif include_aur:
        result.warnings.append("paru not found — skipping AUR updates.")

    if include_flatpak and which("flatpak"):
        try:
            fp_pkgs, fp_warnings = _check_flatpak_packages()
            result.packages.extend(fp_pkgs)
            result.warnings.extend(fp_warnings)
        except TimeoutError:
            result.warnings.append("Timed out while checking Flatpak updates.")
        except OSError as exc:
            result.warnings.append(f"Flatpak check failed: {exc}")
    elif include_flatpak:
        result.warnings.append("flatpak not found — skipping Flatpak updates.")

    for pkg in result.packages:
        enrich_package(pkg)

    result.packages.sort(key=lambda p: (p.severity.value, p.source.value, p.name))
    return result


def _check_repo_packages() -> tuple[list[PackageUpdate], list[str]]:
    warnings: list[str] = []
    tmp = Path(tempfile.mkdtemp(prefix="cachy-updater-db-"))
    try:
        env = {"CHECKUPDATES_DB": str(tmp)}
        proc = run_cmd(
            ["checkupdates", "--nocolor"],
            timeout=CHECKUPDATES_TIMEOUT,
            env=env,
        )
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    # checkupdates: 0 = updates, 2 = none, other = error
    if proc.returncode == 2 or (
        proc.returncode == 0 and not proc.stdout.strip()
    ):
        return [], warnings

    if proc.returncode not in (0, 2):
        err = (proc.stderr or proc.stdout or "unknown error").strip()
        if "404" in err or "failed" in err.lower():
            warnings.append(f"Mirror/network issue while checking updates: {err}")
        elif err:
            warnings.append(f"checkupdates reported: {err}")
        # Still try to parse stdout if any lines look valid

    packages: list[PackageUpdate] = []
    for raw in proc.stdout.splitlines():
        line = raw.strip()
        if not line:
            continue
        match = LINE_RE.match(line)
        if not match:
            continue
        packages.append(
            PackageUpdate(
                name=match.group("name"),
                old_version=match.group("old"),
                new_version=match.group("new"),
                source=UpdateSource.REPO,
            )
        )
    return packages, warnings


def _check_aur_packages() -> tuple[list[PackageUpdate], list[str]]:
    warnings: list[str] = []
    proc = run_cmd(
        ["paru", "--color", "never", "-Qua"],
        timeout=CHECKUPDATES_TIMEOUT,
    )
    if proc.returncode not in (0, 1):
        err = (proc.stderr or proc.stdout or "").strip()
        if err:
            warnings.append(f"paru reported: {err}")
        return [], warnings

    packages: list[PackageUpdate] = []
    for raw in proc.stdout.splitlines():
        line = raw.strip()
        if not line or "[ignored]" in line:
            continue
        parts = line.split()
        # paru -Qua: name old -> new  OR  name old -> new [repo]
        if len(parts) >= 4 and parts[2] == "->":
            packages.append(
                PackageUpdate(
                    name=parts[0],
                    old_version=parts[1],
                    new_version=parts[3],
                    source=UpdateSource.AUR,
                    repo="aur",
                )
            )
        elif len(parts) >= 2:
            packages.append(
                PackageUpdate(
                    name=parts[0],
                    old_version=parts[1] if len(parts) > 1 else "",
                    new_version=parts[-1],
                    source=UpdateSource.AUR,
                    repo="aur",
                )
            )
    return packages, warnings


def _check_flatpak_packages() -> tuple[list[PackageUpdate], list[str]]:
    warnings: list[str] = []
    # Refresh appstream metadata (best effort)
    meta = run_cmd(
        ["flatpak", "update", "--appstream"],
        timeout=CHECKUPDATES_TIMEOUT,
    )
    if meta.returncode not in (0, 1):
        err = (meta.stderr or meta.stdout or "").strip()
        if err:
            warnings.append(f"flatpak appstream refresh: {err[:160]}")

    proc = run_cmd(
        [
            "flatpak",
            "remote-ls",
            "--updates",
            "--columns=application,name,version",
        ],
        timeout=CHECKUPDATES_TIMEOUT,
    )
    if proc.returncode not in (0, 1):
        err = (proc.stderr or proc.stdout or "").strip()
        if err:
            warnings.append(f"flatpak reported: {err[:160]}")
        return [], warnings

    # Installed versions for nicer deltas
    installed: dict[str, str] = {}
    inst = run_cmd(
        ["flatpak", "list", "--columns=application,version"],
        timeout=30,
    )
    if inst.returncode == 0:
        for raw in inst.stdout.splitlines():
            parts = raw.split("\t") if "\t" in raw else raw.split()
            if len(parts) >= 2:
                installed[parts[0].strip()] = parts[1].strip()

    packages: list[PackageUpdate] = []
    for raw in proc.stdout.splitlines():
        line = raw.strip()
        if not line:
            continue
        parts = line.split("\t") if "\t" in line else re.split(r"\s{2,}|\t", line)
        if len(parts) < 2:
            continue
        app_id = parts[0].strip()
        # columns: application, name, version — name may contain spaces if split wrong
        if len(parts) >= 3:
            display = parts[1].strip() or app_id
            new_ver = parts[-1].strip()
        else:
            display = app_id
            new_ver = parts[1].strip()
        old_ver = installed.get(app_id, "")
        packages.append(
            PackageUpdate(
                name=display,
                old_version=old_ver,
                new_version=new_ver,
                source=UpdateSource.FLATPAK,
                repo="flatpak",
                description=f"Flatpak application ({app_id})",
            )
        )
        # Stash app id in description prefix already; also keep id as selectable key
        packages[-1].description = f"Flatpak: {app_id}"
        # Reuse groups tuple to carry flatpak app id without model change
        packages[-1].groups = (app_id,)
    return packages, warnings
