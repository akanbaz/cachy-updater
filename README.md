# Cachy Updater

A fast, native **C++ / Kirigami** system updater for CachyOS — lightweight,
single-binary, and styled to the CachyOS design language.

**Repo:** https://github.com/akanbaz/cachy-updater

## Features

### Updates
- Grouped **Repo** (`checkupdates` / `pacman`), **AUR** (`paru` or `yay`), and **Flatpak** updates
- **Search & filters** (name, severity, source)
- **Package holds** persisted across sessions
- **Kernel card** with running-kernel indicator and installed-kernel list
- Per-package **severity badges**, **changelogs** (expac / AUR RPC), expandable summaries
- **Arch news gate** — acknowledge recent Arch news before repo apply
- **Reboot prompt** after kernel/systemd/firmware-class updates
- **NVIDIA + kernel warning** when both are selected
- **Mirror health check**
- **Per-source apply** (repo / AUR / Flatpak buttons in section headers)
- **Dry run / Download only / Apply**, partial-upgrade guard (`--ignore`)
- **Snapper pre-update snapshot** (when available)
- **Offline mode** with cached last-check results
- **Keyboard shortcuts**: `R` refresh, `Ctrl+A` select all, `Ctrl+Enter` apply, `Esc` cancel

### News
- Arch + CachyOS RSS feeds
- Arch news acknowledgement integrated with update safety gate

### Cleanup
- Orphan removal, configurable **paccache** retention
- **Old kernel** cleanup
- **Flatpak unused** runtimes
- **AUR cache** cleanup
- Disk-space summary

### Firmware
- **fwupd** integration (scan + update devices)

### History
- Log of apply and maintenance actions

### Settings
- Tray interval, auto-check, default tab, source toggles
- Notify-only-important, scheduled checks (systemd timer)
- Pacman parallel downloads, AUR concurrency, snapshot toggle

### Tray
- Colored status icons, progress in tooltip
- Middle-click check, **Apply all** menu action
- Rich notifications with important-update count
- CLI: `cachyos-updater --check --notify`

## Runtime dependencies

| Package | Purpose |
|---------|---------|
| `qt6-base` | Qt6 core, GUI, widgets |
| `qt6-declarative` | QML / Qt Quick |
| `kirigami` | KDE Kirigami UI framework |
| `qqc2-desktop-style` | Native Plasma widget styling |
| `breeze-icons` | System icons |
| `pacman` | Repo package management |
| `polkit` | Privileged actions via `pkexec` |

Optional (auto-detected):

| Package | Purpose |
|---------|---------|
| `paru` / `yay` | AUR updates |
| `pacman-contrib` | `checkupdates` and `paccache` |
| `expac` | Metadata and changelogs |
| `flatpak` | Flatpak updates |
| `fwupd` | Firmware tab |
| `snapper` | Pre-update btrfs snapshots |

## Build

```bash
cd native
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/cachyos-updater
```

## Install

```bash
cd native
cmake --install build
systemctl --user enable --now org.cachyos.updater-tray.service
# optional nightly checks:
systemctl --user enable --now org.cachyos.updater-check.timer
```

## Architecture

```
native/src/   SettingsController, HistoryController, FwupdController,
              UpdateController, NewsController, MaintainController, TrayController
native/qml/   Main.qml + pages (Updates, News, Cleanup, Firmware, History, Settings)
```
