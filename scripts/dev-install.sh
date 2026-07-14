#!/usr/bin/env bash
# One real binary (native/build/cachyos-updater); ~/.local/bin and optionally
# /usr/bin are only short_cuts (symlinks) to it. Rebuild = tray + GUI update.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/native/build"
PREFIX="${HOME}/.local"
BIN_NAME="cachyos-updater"
REAL_BIN="${BUILD}/${BIN_NAME}"
LOCAL_BIN="${PREFIX}/bin/${BIN_NAME}"
CMAKE="${ROOT}/.buildenv/bin/cmake"
LINK_SYSTEM="${LINK_SYSTEM:-1}"

if [[ ! -x "${CMAKE}" ]]; then
  CMAKE="$(command -v cmake)"
fi

mkdir -p "${BUILD}" "${PREFIX}/bin"
if [[ ! -f "${BUILD}/CMakeCache.txt" ]]; then
  "${CMAKE}" -S "${ROOT}/native" -B "${BUILD}" -DCMAKE_BUILD_TYPE=Release
fi

"${CMAKE}" --build "${BUILD}" -j"$(nproc)"
"${CMAKE}" --install "${BUILD}" --prefix "${PREFIX}"

# cmake --install copies a second binary — replace it with a shortcut to the build.
ln -sfn "${REAL_BIN}" "${LOCAL_BIN}"

# Desktop launchers: always open the local shortcut.
{
  sed -i \
    -e "s|^Exec=.*|Exec=${LOCAL_BIN}|" \
    -e "s|^TryExec=.*|TryExec=${LOCAL_BIN}|" \
    "${PREFIX}/share/applications/org.cachyos.updater.desktop"
  sed -i \
    -e "s|^Exec=.*|Exec=${LOCAL_BIN} --tray|" \
    -e "s|^TryExec=.*|TryExec=${LOCAL_BIN}|" \
    "${PREFIX}/share/applications/org.cachyos.updater-tray.desktop"
} 2>/dev/null || true
update-desktop-database "${PREFIX}/share/applications" 2>/dev/null || true

# Optional: /usr/bin becomes a shortcut to the same binary (needs sudo once).
if [[ "${LINK_SYSTEM}" == "1" ]]; then
  target="$(readlink -f "${REAL_BIN}")"
  current="$(readlink -f "/usr/bin/${BIN_NAME}" 2>/dev/null || true)"
  if [[ "${current}" != "${target}" ]]; then
    if sudo -n true 2>/dev/null; then
      sudo ln -sfn "${LOCAL_BIN}" "/usr/bin/${BIN_NAME}"
      echo "System shortcut: /usr/bin/${BIN_NAME} -> ${LOCAL_BIN}"
    else
      echo
      echo "One-time (makes /usr/bin a shortcut too — needs your password):"
      echo "  sudo ln -sfn ${LOCAL_BIN} /usr/bin/${BIN_NAME}"
    fi
  fi
fi

# User systemd: point at the local shortcut (resolves to the build binary).
mkdir -p "${HOME}/.config/systemd/user"
cat > "${HOME}/.config/systemd/user/org.cachyos.updater-tray.service" <<EOF
[Unit]
Description=Cachy Updater systray applet
After=graphical-session.target

[Service]
ExecStart=${LOCAL_BIN} --tray
Restart=on-failure

[Install]
WantedBy=graphical-session.target
EOF

cat > "${HOME}/.config/systemd/user/org.cachyos.updater-check.service" <<EOF
[Unit]
Description=CachyOS Updater scheduled check

[Service]
Type=oneshot
ExecStart=${LOCAL_BIN} --check --notify
EOF

# Refresh Plasma icon lookup: replace stale /usr app icon when possible.
if sudo -n true 2>/dev/null; then
  sudo install -Dm644 "${ROOT}/native/assets/logo.svg" \
    /usr/share/icons/hicolor/scalable/apps/org.cachyos.updater.svg
  sudo install -Dm644 "${ROOT}/native/assets/tray-uptodate.svg" \
    /usr/share/icons/hicolor/scalable/status/org.cachyos.updater-tray.svg
  sudo install -Dm644 "${ROOT}/native/assets/tray-updates.svg" \
    /usr/share/icons/hicolor/scalable/status/org.cachyos.updater-tray-updates.svg
  sudo gtk-update-icon-cache -f /usr/share/icons/hicolor 2>/dev/null || true
elif [[ -f /usr/share/icons/hicolor/scalable/apps/org.cachyos.updater.svg ]]; then
  echo
  echo "Panel still showing the old icon? Update the system icon once:"
  echo "  sudo install -Dm644 ${ROOT}/native/assets/logo.svg /usr/share/icons/hicolor/scalable/apps/org.cachyos.updater.svg"
  echo "  sudo gtk-update-icon-cache -f /usr/share/icons/hicolor"
fi

gtk-update-icon-cache -f "${PREFIX}/share/icons/hicolor" 2>/dev/null || true
rm -f "${HOME}/.cache/icon-cache.kcache" 2>/dev/null || true

systemctl --user daemon-reload
systemctl --user enable --now org.cachyos.updater-tray.service
systemctl --user restart org.cachyos.updater-tray.service

echo
echo "Real binary:  ${REAL_BIN}"
echo "Local shortcut: ${LOCAL_BIN} -> $(readlink -f "${LOCAL_BIN}")"
if [[ -L /usr/bin/${BIN_NAME} ]]; then
  echo "System shortcut: /usr/bin/${BIN_NAME} -> $(readlink /usr/bin/${BIN_NAME})"
fi
echo "Tray restarted. Rebuilds update the same binary for GUI + tray."
echo "If the panel icon is still old, log out/in once or run:"
echo "  kbuildsycoca6 --noincremental 2>/dev/null || true"
