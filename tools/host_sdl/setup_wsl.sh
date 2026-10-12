#!/usr/bin/env bash
# One-time setup of the host-sdl toolchain inside WSL2 (Ubuntu).
# Installs build deps + SDL2 (needs sudo) and ESPHome into a virtualenv.
#
# Usage: tools/host_sdl/setup_wsl.sh [esphome-version]
#   Default version matches the ESPHome CLI used for the real devices.
set -euo pipefail

VERSION="${1:-2026.9.1}"
VENV="${ESPHOME_VENV:-$HOME/.venvs/esphome}"

need=()
for pkg in build-essential python3-venv libsdl2-dev libssl-dev pkg-config curl; do
  dpkg -s "$pkg" >/dev/null 2>&1 || need+=("$pkg")
done
if (( ${#need[@]} )); then
  echo "Installing: ${need[*]}"
  sudo apt-get update
  sudo apt-get install -y "${need[@]}"
fi

if [[ ! -x "$VENV/bin/python" ]]; then
  python3 -m venv "$VENV"
fi
"$VENV/bin/pip" install -q --upgrade pip wheel
"$VENV/bin/pip" install -q "esphome==$VERSION"
"$VENV/bin/esphome" version

