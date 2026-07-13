"""Build short, useful 'what changed' summaries for packages."""

from __future__ import annotations

import re

from cachy_updater.backend import run_cmd, which
from cachy_updater.models import PackageUpdate, UpdateSeverity, UpdateSource

CRITICAL_EXACT = {
    "linux",
    "linux-lts",
    "linux-zen",
    "linux-hardened",
    "linux-cachyos",
    "linux-cachyos-lto",
    "linux-cachyos-bore",
    "linux-firmware",
    "systemd",
    "systemd-libs",
    "glibc",
    "pacman",
    "mesa",
    "nvidia",
    "nvidia-dkms",
    "nvidia-utils",
    "filesystem",
    "bash",
    "coreutils",
}

CRITICAL_PREFIXES = (
    "linux-cachyos",
    "linux-firmware",
    "nvidia-",
    "mesa-",
    "systemd-",
    "glibc",
    "gcc-",
    "amd-ucode",
    "intel-ucode",
)

NOTICE_PREFIXES = (
    "plasma-",
    "kf6-",
    "qt6-",
    "pipewire",
    "wireplumber",
    "firefox",
    "chromium",
)

VERSION_RE = re.compile(
    r"^(?:(?P<epoch>\d+):)?(?P<main>[^-]+?)(?:-(?P<rel>.+))?$"
)


def enrich_package(pkg: PackageUpdate) -> None:
    if pkg.source != UpdateSource.FLATPAK:
        _fill_metadata(pkg)
    else:
        if not pkg.description:
            pkg.description = "Flatpak application"
        pkg.repo = "flatpak"
    pkg.severity = _classify_severity(pkg)
    pkg.summary = _build_summary(pkg)


def _fill_metadata(pkg: PackageUpdate) -> None:
    if pkg.source == UpdateSource.AUR:
        _fill_from_paru(pkg)
        return
    _fill_from_expac_or_pacman(pkg)


def _fill_from_expac_or_pacman(pkg: PackageUpdate) -> None:
    if which("expac"):
        proc = run_cmd(
            ["expac", "-S", "%r\t%d\t%m\t%G", pkg.name],
            timeout=15,
        )
        if proc.returncode == 0 and proc.stdout.strip():
            line = proc.stdout.strip().splitlines()[0]
            parts = line.split("\t")
            while len(parts) < 4:
                parts.append("")
            pkg.repo, pkg.description, size_raw, groups_raw = parts[:4]
            pkg.groups = tuple(g for g in groups_raw.split() if g)
            pkg.size_label = _format_size(size_raw)
            return

    proc = run_cmd(["pacman", "-Si", "--", pkg.name], timeout=15)
    if proc.returncode != 0:
        return
    fields = _parse_pacman_info(proc.stdout)
    pkg.repo = fields.get("Repository", pkg.repo)
    pkg.description = fields.get("Description", pkg.description)
    pkg.size_label = fields.get("Download Size", "") or fields.get(
        "Installed Size", ""
    )
    groups = fields.get("Groups", "")
    if groups and groups != "None":
        pkg.groups = tuple(groups.split())


def _fill_from_paru(pkg: PackageUpdate) -> None:
    proc = run_cmd(["paru", "-Si", "--", pkg.name], timeout=20)
    if proc.returncode != 0:
        pkg.description = pkg.description or "AUR package"
        return
    fields = _parse_pacman_info(proc.stdout)
    pkg.description = fields.get("Description", pkg.description)
    pkg.repo = "aur"
    pkg.size_label = fields.get("Download Size", "") or fields.get(
        "Installed Size", ""
    )


def _parse_pacman_info(text: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    current: str | None = None
    for raw in text.splitlines():
        if not raw.strip():
            continue
        if ":" in raw and not raw.startswith(" "):
            key, _, value = raw.partition(":")
            current = key.strip()
            fields[current] = value.strip()
        elif current and raw.startswith(" "):
            fields[current] = f"{fields[current]} {raw.strip()}".strip()
    return fields


def _classify_severity(pkg: PackageUpdate) -> UpdateSeverity:
    name = pkg.name.lower()
    if name in CRITICAL_EXACT or any(name.startswith(p) for p in CRITICAL_PREFIXES):
        return UpdateSeverity.CRITICAL
    if "base" in pkg.groups or "base-devel" in pkg.groups:
        return UpdateSeverity.IMPORTANT
    bump = _version_bump(pkg.old_version, pkg.new_version)
    if bump == "major":
        return UpdateSeverity.IMPORTANT
    if any(name.startswith(p) for p in NOTICE_PREFIXES):
        return UpdateSeverity.NOTICE
    if bump == "minor":
        return UpdateSeverity.NOTICE
    return UpdateSeverity.ROUTINE


def _build_summary(pkg: PackageUpdate) -> str:
    bits: list[str] = []
    bump = _version_bump(pkg.old_version, pkg.new_version)
    if bump == "major":
        bits.append("Major version bump — review notes if this is a core package.")
    elif bump == "minor":
        bits.append("Minor version update with likely feature or API changes.")
    elif bump == "patch":
        bits.append("Patch/release update — typically fixes and small improvements.")
    else:
        bits.append("Version update available.")

    if pkg.severity == UpdateSeverity.CRITICAL:
        bits.append("Marked important: may need a reboot or affect the running system.")

    desc = (pkg.description or "").strip()
    if desc:
        bits.append(desc if desc.endswith(".") else f"{desc}.")
    else:
        bits.append("No package description was available from the repositories.")

    if pkg.source == UpdateSource.AUR:
        bits.append("Source: AUR (built locally via paru).")
    elif pkg.source == UpdateSource.FLATPAK:
        app_id = pkg.groups[0] if pkg.groups else pkg.name
        bits.append(f"Source: Flatpak ({app_id}).")
    elif pkg.repo:
        bits.append(f"Repository: {pkg.repo}.")

    if pkg.size_label:
        bits.append(f"Size: {pkg.size_label}.")

    return " ".join(bits)


def _version_bump(old: str, new: str) -> str:
    old_p = _parse_version(old)
    new_p = _parse_version(new)
    if not old_p or not new_p:
        return "unknown"
    if old_p[0] != new_p[0]:
        return "major"
    if old_p[1] != new_p[1]:
        return "minor"
    return "patch"


def _parse_version(version: str) -> tuple[int, int, int] | None:
    match = VERSION_RE.match(version.strip())
    if not match:
        return None
    main = match.group("main")
    nums = re.findall(r"\d+", main)
    if not nums:
        return None
    major = int(nums[0])
    minor = int(nums[1]) if len(nums) > 1 else 0
    patch = int(nums[2]) if len(nums) > 2 else 0
    return major, minor, patch


def _format_size(raw: str) -> str:
    raw = (raw or "").strip()
    if not raw or not raw.isdigit():
        return raw
    size = int(raw)
    for unit in ("B", "KiB", "MiB", "GiB"):
        if size < 1024 or unit == "GiB":
            if unit == "B":
                return f"{size} {unit}"
            return f"{size:.1f} {unit}"
        size /= 1024
    return raw
