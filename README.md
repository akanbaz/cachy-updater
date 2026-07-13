# Cachy Updater

A personal GUI system updater for CachyOS — close in spirit to `cachy-update`, focused on clarity and stability.

**Repo:** https://github.com/akanbaz/cachy-updater

## Features

- Native PyQt6 UI with per-package **change summaries**
- **Repo** (`checkupdates` / `pacman`), **AUR** (`paru`), and **Flatpak** updates
- **News** tab (Arch RSS + best-effort Cachy feeds)
- **Orphans** removal and **pacman cache** cleanup
- **System tray** with periodic checks and notifications
- Polkit (`pkexec`) for privileged actions
- Full `pacman -Syu` when all repo updates are selected (avoids partial upgrades)

## Requirements

```bash
sudo pacman -S --needed python python-pyqt6 pacman-contrib expac polkit curl
```

Optional: `paru` (AUR), `flatpak` (Flatpak apps).

## Run

```bash
cachy-updater          # window + tray
cachy-updater --tray   # tray only
```

Or from the project tree:

```bash
./run.sh
./run.sh --tray
```

## Packaging / CachyOS repos

Yes — this app is **capable** of one-click install once packaged:

```bash
sudo pacman -S cachy-updater
```

That is not automatic. CachyOS repo inclusion needs:

1. A public GitHub repo + tagged releases  
2. A proper `PKGBUILD` (see `packaging/PKGBUILD`)  
3. Review/acceptance by CachyOS packagers (usually via their PKGBUILDS repo)  
4. Ongoing maintenance (updates, fixes)

Until then you can:

- run from source (`cachy-updater` / `./run.sh`)
- or build a local package with `makepkg` after adjusting the PKGBUILD `source=`
- or publish to the AUR for `paru -S cachy-updater` one-click install
