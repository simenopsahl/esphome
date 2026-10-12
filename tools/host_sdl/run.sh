#!/usr/bin/env bash
# Build / run the virtual host-sdl ESPHome device (Linux / WSL2).
#
# Usage: tools/host_sdl/run.sh <config|compile|run|clean> [layout] [extra esphome args...]
#   layout = name of a gui/<layout>.yaml file, e.g. 240x320, 480x320, 128x128
#
# Environment:
#   ESPHOME_VENV      ESPHome virtualenv (default: ~/.venvs/esphome)
#   ESPHOME_DATA_DIR  build/cache dir (default: ~/.esphome-host, kept off the network share)
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV="${ESPHOME_VENV:-$HOME/.venvs/esphome}"
export ESPHOME_DATA_DIR="${ESPHOME_DATA_DIR:-$HOME/.esphome-host}"
CONFIG="host-sdl.yaml"

cmd="${1:-run}"
layout="${2:-}"
shift $(( $# > 2 ? 2 : $# ))

if [[ ! -x "$VENV/bin/esphome" ]]; then
  echo "ESPHome not found in $VENV — run tools/host_sdl/setup_wsl.sh first" >&2
  exit 1
fi

args=()
if [[ -n "$layout" ]]; then
  file="$REPO/gui/$layout.yaml"
  if [[ ! -f "$file" ]]; then
    echo "Unknown layout '$layout' (no $file)" >&2
    exit 1
  fi
  # Layout files declare their pixel size in the &page_styles block
  width="$(grep -m1 -E '^\s*screen_width:' "$file" | grep -oE '[0-9]+')"
  height="$(grep -m1 -E '^\s*screen_height:' "$file" | grep -oE '[0-9]+')"
  if [[ -z "$width" || -z "$height" ]]; then
    echo "Could not read screen_width/screen_height from $file" >&2
    exit 1
  fi
  echo "Layout gui/$layout.yaml -> ${width}x${height} px"
  args+=(-s gui_layout "$layout" -s screen_width "$width" -s screen_height "$height")
fi

cd "$REPO"
exec "$VENV/bin/esphome" "${args[@]}" "$cmd" "$CONFIG" "$@"

