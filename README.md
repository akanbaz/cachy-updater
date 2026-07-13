# Cachy Updater

A fast, native **C++ / Kirigami** system updater for CachyOS — lightweight,
single-binary, and styled to the CachyOS design language.

**Repo:** https://github.com/akanbaz/cachy-updater

## Features

- Native **Kirigami / Qt Quick** UI
- Grouped **Repo** (`checkupdates` / `pacman`), **AUR** (`paru`), and **Flatpak** updates
- **Kernel updates** pulled out distinctly with a reboot hint
- Per-package **severity badges** and expandable **change summaries**
- Exact commands shown, with a collapsible **Terminal** panel (real-time output, copy log)
- **Apply / Dry Run / Download Only**, full `pacman -Syu` with a partial-upgrade guard (`--ignore`)
- **News** tab (Arch RSS + best-effort CachyOS feeds via `QNetworkAccessManager`)
- **Cleanup** tab: orphan removal (`pacman -Rns`) and cache cleanup (`paccache`)
- Background **system tray** applet (`cachyos-updater --tray`) with colored status icons
- Polkit (`pkexec`) for privileged operations

## Runtime dependencies

Required on a stock CachyOS Plasma install:

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
| `paru` | AUR updates |
| `pacman-contrib` | `checkupdates` and `paccache` |
| `expac` | Fast package metadata |
| `flatpak` | Flatpak updates |

## Build

```bash
cd native
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/cachyos-updater
```

Build requirements: `cmake`, `ninja`, `qt6-base`, `qt6-declarative`, C++20 compiler.

## Install

```bash
cd native
cmake --install build
systemctl --user enable --now org.cachyos.updater-tray.service
```

Or via the Arch package:

```bash
cd packaging && makepkg -si
```

The package install script disables the legacy `arch-update-tray` service and
enables `org.cachyos.updater-tray.service`.

QML, tray icons, and UI assets are embedded in the single ELF via the Qt
Resource System. Runtime links only against system Qt6/Kirigami libraries.

## Architecture

Thin QML/Kirigami views bind to C++ `QObject` controllers and a
`QAbstractListModel`. All external work runs through one async `ProcessRunner`
(`QProcess`) — the UI never blocks.

```
native/
  src/    ProcessRunner, UpdateController, UpdatesModel, Classifier,
          NewsController, MaintainController, TrayController, main.cpp
  qml/    Main.qml, Theme.qml, pages/, components/
  assets/ logo.svg, tray-uptodate.svg, tray-updates.svg
packaging/
  PKGBUILD, cachyos-updater.install
```

## Project layout

```
cachy-updater/
├── LICENSE
├── README.md
├── native/           # C++/QML application
└── packaging/        # Arch Linux PKGBUILD
```
