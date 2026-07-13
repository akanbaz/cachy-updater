"""Shared data models."""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum


class UpdateSource(str, Enum):
    REPO = "repo"
    AUR = "aur"
    FLATPAK = "flatpak"


class UpdateSeverity(str, Enum):
    ROUTINE = "routine"
    NOTICE = "notice"
    IMPORTANT = "important"
    CRITICAL = "critical"


@dataclass(slots=True)
class PackageUpdate:
    name: str
    old_version: str
    new_version: str
    source: UpdateSource = UpdateSource.REPO
    description: str = ""
    repo: str = ""
    summary: str = ""
    severity: UpdateSeverity = UpdateSeverity.ROUTINE
    size_label: str = ""
    selected: bool = True
    groups: tuple[str, ...] = field(default_factory=tuple)

    @property
    def version_delta(self) -> str:
        if self.old_version and self.new_version:
            return f"{self.old_version} → {self.new_version}"
        return self.new_version or self.old_version or "—"


@dataclass(slots=True)
class CheckResult:
    packages: list[PackageUpdate] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not self.errors
