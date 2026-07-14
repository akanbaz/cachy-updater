#!/usr/bin/env bash
# Build, install to ~/.local, and restart the tray so one binary feeds GUI + tray.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/native/build"
PREFIX="${HOME}/.local"
CMAKE="${ROOT}/.buildenv/bin/cmake"

if [[ ! -x "${CMAKE}" ]]; then
  CMAKE="$(command -v cmake)"
fi

mkdir -p "${BUILD}"
if [[ ! -f "${BUILD}/CMakeCache.txt" ]]; then
  "${CMAKE}" -S "${ROOT}/native" -B "${BUILD}" -DCMAKE_BUILD_TYPE=Release
fi

"${CMAKE}" --build "${BUILD}" -j"$(nproc)"
"${CMAKE}" --install "${BUILD}" --prefix "${PREFIX}"

# Desktop launchers: pin absolute path so Plasma never picks /usr/bin by accident.
for f in org.cachyos.updater.desktop org.cachyos.updater-tray.desktop; do
  dest="${PREFIX}/share/applications/${f}"
  if [[ -f "${dest}" ]]; then
    sed -i "s|^Exec=cachyos-updater|Exec=${PREFIX}/bin/cachyos-updater|" "${dest}"
    sed -i "s|^TryExec=.*|TryExec=${PREFIX}/bin/cachyos-updater|" "${dest}"
  fi
done
update-desktop-database "${PREFIX}/share/applications" 2>/dev/null || true

# User systemd override: prefer ~/.local/bin and point at this install.
mkdir -p "${HOME}/.config/systemd/user"
cat > "${HOME}/.config/systemd/user/org.cachyos.updater-tray.service" <<EOF
[Unit]
Description=Cachy Updater systray applet
After=graphical-session.target

[Service]
Environment=PATH=${PREFIX}/bin:/usr/local/bin:/usr/bin
ExecStart=${PREFIX}/bin/cachyos-updater --tray
Restart=on-failure

[Install]
WantedBy=graphical-session.target
EOF

if [[ -f "${PREFIX}/share/systemd/user/org.cachyos.updater-check.service" ]]; then
  cp "${PREFIX}/share/systemd/user/org.cachyos.updater-check.service" \
     "${HOME}/.config/systemd/user/org.cachyos.updater-check.service"
  sed -i "s|ExecStart=.*|ExecStart=${PREFIX}/bin/cachyos-updater --check --notify|" \
    "${HOME}/.config/systemd/user/org.cachyos.updater-check.service"
  if ! grep -q '^Environment=' "${HOME}/.config/systemd/user/org.cachyos.updater-check.service"; then
    sed -i "/^\\[Service\\]/a Environment=PATH=${PREFIX}/bin:/usr/local/bin:/usr/bin" \
      "${HOME}/.config/systemd/user/org.cachyos.updater-check.service"
  fi
fi

systemctl --user daemon-reload
systemctl --user enable --now org.cachyos.updater-tray.service
systemctl --user restart org.cachyos.updater-tray.service

echo
echo "Installed: ${PREFIX}/bin/cachyos-updater"
echo "Tray restarted from that binary."
echo "Launch GUI: ${PREFIX}/bin/cachyos-updater"
