"""Lightweight tests for summary helpers (no network)."""

from cachy_updater.backend.summaries import _version_bump, enrich_package
from cachy_updater.models import PackageUpdate, UpdateSeverity, UpdateSource


def test_version_bump_major():
    assert _version_bump("1.2.3-1", "2.0.0-1") == "major"


def test_version_bump_patch():
    assert _version_bump("6.11.1-1", "6.11.2-1") == "patch"


def test_critical_kernel_severity():
    pkg = PackageUpdate(
        name="linux-cachyos",
        old_version="6.14.1-1",
        new_version="6.14.2-1",
        source=UpdateSource.REPO,
        description="The Linux CachyOS kernel",
        repo="cachyos",
    )
    enrich_package(pkg)
    assert pkg.severity == UpdateSeverity.CRITICAL
    assert "kernel" in pkg.summary.lower() or "important" in pkg.summary.lower()
    assert "Linux CachyOS" in pkg.summary or "linux" in pkg.summary.lower()
