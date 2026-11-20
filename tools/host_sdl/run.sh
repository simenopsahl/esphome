#!/usr/bin/env bash
# Build / run the virtual host-sdl ESPHome device (Linux / WSL2).
#
# Usage: tools/host_sdl/run.sh <config|compile|run|clean> [layout] [--epaper] [--size WxH] [--modules] [--cc1101-mode packet|async] [extra esphome args...]
#   layout = gui/<layout>.yaml (default) or gui/epaper/pages/<layout>.yaml (with --epaper)
#   --epaper   = use .host-sdl-epaper.yaml instead of .host-sdl.yaml
#   --size WxH = explicit screen_width x screen_height override (e.g. 320x240, 480x320)
#   --modules  = include board/module/host_simulated_modules.yaml (air_quality, dual_kmeter, cc1101)
#   --cc1101-mode = packet (default) or async when --modules is active
#
# Environment:
#   ESPHOME_VENV      ESPHome virtualenv (default: ~/.venvs/esphome)
#   ESPHOME_DATA_DIR  build/cache dir (default: ~/.esphome-host, kept off the network share)
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV="${ESPHOME_VENV:-$HOME/.venvs/esphome}"
export ESPHOME_DATA_DIR="${ESPHOME_DATA_DIR:-$HOME/.esphome-host}"
CONFIG=".host-sdl.yaml"

cmd="${1:-run}"
shift $(( $# > 0 ? 1 : 0 ))

layout=""
size_override=""
enable_epaper=0
enable_modules=0
cc1101_mode=""
extra_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --epaper)
      enable_epaper=1
      CONFIG=".host-sdl-epaper.yaml"
      shift
      ;;
    --size)
      size_override="${2:-}"
      shift 2
      ;;
    --modules)
      enable_modules=1
      shift
      ;;
    --cc1101-mode)
      cc1101_mode="${2:-}"
      shift 2
      ;;
    -*)
      extra_args+=("$1")
      shift
      ;;
    *)
      if [[ -z "$layout" ]]; then
        layout="$1"
      else
        extra_args+=("$1")
      fi
      shift
      ;;
  esac
done

if [[ ! -x "$VENV/bin/esphome" ]]; then
  echo "ESPHome not found in $VENV — run tools/host_sdl/setup_wsl.sh first" >&2
  exit 1
fi

args=()
width=""
height=""

if [[ "$enable_epaper" -eq 1 ]]; then
  if [[ -n "$layout" ]]; then
    file="$REPO/gui/epaper/pages/$layout/main.yaml"
    if [[ ! -f "$file" ]]; then
      echo "Unknown e-Paper page '$layout' (no $file)" >&2
      exit 1
    fi
    args+=(-s epaper_page "$layout")
  fi
elif [[ -n "$layout" ]]; then
  file="$REPO/gui/$layout.yaml"
  if [[ ! -f "$file" ]]; then
    echo "Unknown layout '$layout' (no $file)" >&2
    exit 1
  fi
  args+=(-s gui_layout "$layout")
  # Layout files declare their pixel size in the &page_styles block (unless overridden by --size)
  if [[ -z "$size_override" ]]; then
    width="$(grep -m1 -E '^\s*screen_width:' "$file" | grep -oE '[0-9]+' || true)"
    height="$(grep -m1 -E '^\s*screen_height:' "$file" | grep -oE '[0-9]+' || true)"
  fi
fi

if [[ -n "$size_override" ]]; then
  if [[ "$size_override" =~ ^([0-9]+)x([0-9]+)$ ]]; then
    width="${BASH_REMATCH[1]}"
    height="${BASH_REMATCH[2]}"
  else
    echo "Invalid --size '$size_override' (expected <width>x<height>, e.g. 320x240)" >&2
    exit 1
  fi
fi

if [[ -n "$width" && -n "$height" ]]; then
  echo "Display size -> ${width}x${height} px (layout: ${layout:-default})"
  if [[ "$enable_epaper" -eq 1 ]]; then
    args+=(-s panel_width "$width" -s panel_height "$height")
    if (( width <= 240 && height <= 240 )); then
      args+=(-s epaper_rotation 90)
    fi
  else
    args+=(-s screen_width "$width" -s screen_height "$height")
  fi
fi

if [[ "$enable_modules" -eq 1 ]]; then
  if [[ "$cc1101_mode" == "async" ]]; then
    echo "Including simulated module pages (air_quality, dual_kmeter, cc1101 wmbus + async)"
    args+=(-s sim_modules_pkg "host_simulated_modules_async")
  else
    echo "Including simulated module pages (air_quality, dual_kmeter, cc1101 wmbus + packet)"
    args+=(-s sim_modules_pkg "host_simulated_modules")
  fi
fi

cd "$REPO"
exec "$VENV/bin/esphome" "${args[@]}" "$cmd" "$CONFIG" "${extra_args[@]}"
