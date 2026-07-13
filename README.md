# CachyOS Updater

A fast, native **C++ / Kirigami** system updater for CachyOS — lightweight,
single-binary, and styled to the CachyOS design language.

**Repo:** https://github.com/akanbaz/cachy-updater

## Features

- Native **Kirigami / Qt Quick** UI (no Python, no web stack)
- Grouped **Repo** (`checkupdates` / `pacman`), **AUR** (`paru`), and **Flatpak** updates
- **Kernel updates** pulled out distinctly with a reboot hint
- Per-package **severity badges** and expandable **change summaries**
- Exact commands shown, with a collapsible **Terminal** panel (real-time output, Copy Log)
- **Apply / Dry Run / Download Only**, full `pacman -Syu` with a partial-upgrade guard (`--ignore`)
- **News** tab (Arch RSS + best-effort CachyOS feeds via `QNetworkAccessManager` — no curl)
- **Cleanup** tab: orphan removal (`pacman -Rns`) and cache cleanup (`paccache`)
- Background **system tray** applet (`cachyos-updater --tray`) with periodic checks and notifications
- Polkit (`pkexec`) for privileged actions

## Runtime dependencies

Present on a stock CachyOS Plasma install:

```
qt6-base qt6-declarative kirigami qqc2-desktop-style breeze-icons pacman polkit
```

Optional (auto-detected): `paru` (AUR), `pacman-contrib` (`checkupdates`/`paccache`),
`expac` (fast metadata), `flatpak`.

## Build

```bash
cd native
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/cachyos-updater
```

Build requirements: `cmake`, `ninja`, `qt6-base`, `qt6-declarative`, a C++20 compiler.

## Install / package

```bash
cd native && cmake --install build            # or via the PKGBUILD:
cd packaging && makepkg -si                    # after adjusting source=
```

Everything (QML, icons, assets) is embedded in the single ELF via the Qt
Resource System; only the shared Qt6/Kirigami libraries already on the system
are linked at runtime. No bundled frameworks, no runtime downloads, no daemon.

## Architecture

Thin QML/Kirigami views bind to C++ `QObject` controllers and a
`QAbstractListModel`. All external work runs through one async `ProcessRunner`
(`QProcess`) — the UI never blocks and there are no worker threads.

```
native/
  src/    ProcessRunner, UpdateController, UpdatesModel, Classifier,
          NewsController, MaintainController, main.cpp
  qml/    Main.qml, Theme.qml, pages/, components/
```
