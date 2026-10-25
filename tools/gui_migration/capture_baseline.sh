#!/usr/bin/env bash
# Capture baseline PNG screenshots across resolutions and pages.
# Runs inside WSL so compilation and HTTP screenshot capture happen locally.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RUN_SH="$REPO/tools/host_sdl/run.sh"
OUT_ROOT="$REPO/tools/gui_migration/baseline"
PORT=8080
ONLY_240X320="${1:-}"

cleanup() {
  pkill -x program 2>/dev/null || true
}
trap cleanup EXIT

wait_for_server() {
  local retries=40
  while (( retries > 0 )); do
    if curl -sf "http://127.0.0.1:${PORT}/screenshot" -o /dev/null; then
      return 0
    fi
    sleep 0.5
    (( retries-- ))
  done
  echo "Timed out waiting for host-sdl HTTP server on port ${PORT}" >&2
  return 1
}

capture_page() {
  local page_idx="$1"
  local out_file="$2"
  curl -sf "http://127.0.0.1:${PORT}/page/${page_idx}" >/dev/null
  # Wait 1.2s so NordPool's 1s delay in nordpool_update_chart (if triggered) or LVGL redraw settles
  sleep 1.2
  curl -sf "http://127.0.0.1:${PORT}/screenshot" -o "$out_file"
  echo "  Saved $out_file"
}

# 1. Capture 240x320 with simulated modules (packet mode):
# Pages: 1=clock, 2=temperature, 3=nordpool, 4=weather, 5=status, 6=air_quality, 7=dual_kmeter, 8=cc1101_wmbus, 9=cc1101_packet
echo "=== Building & running 240x320 (with modules: packet) ==="
cleanup
"$RUN_SH" compile 240x320 --modules
mkdir -p "$OUT_ROOT/240x320"
"$HOME/.esphome-host/build/host-sdl/.pioenvs/host-sdl/program" >/tmp/host-sdl-run.log 2>&1 &
wait_for_server
sleep 1.5
capture_page 1 "$OUT_ROOT/240x320/01_clock.png"
capture_page 2 "$OUT_ROOT/240x320/02_temperature.png"
capture_page 3 "$OUT_ROOT/240x320/03_nordpool.png"
capture_page 4 "$OUT_ROOT/240x320/04_weather.png"
capture_page 5 "$OUT_ROOT/240x320/05_status.png"
capture_page 6 "$OUT_ROOT/240x320/06_air_quality.png"
capture_page 7 "$OUT_ROOT/240x320/07_dual_kmeter.png"
capture_page 8 "$OUT_ROOT/240x320/08_cc1101_wmbus.png"
capture_page 9 "$OUT_ROOT/240x320/09_cc1101_packet.png"
cleanup

# 2. Capture 240x320 with cc1101 async mode (page 9 = cc1101_async)
echo "=== Building & running 240x320 (with modules: async) ==="
"$RUN_SH" compile 240x320 --modules --cc1101-mode async
"$HOME/.esphome-host/build/host-sdl/.pioenvs/host-sdl/program" >/tmp/host-sdl-run.log 2>&1 &
wait_for_server
sleep 1.0
capture_page 9 "$OUT_ROOT/240x320/10_cc1101_async.png"
cleanup

if [[ "$ONLY_240X320" == "--only-240x320" ]]; then
  echo "=== Completed 240x320 baseline capture ==="
  exit 0
fi

# 3. Capture standard 5 pages across other representative layouts:
for layout in 320x240 480x320 128x128 240r 240x135; do
  echo "=== Building & running $layout ==="
  "$RUN_SH" compile "$layout"
  mkdir -p "$OUT_ROOT/$layout"
  "$HOME/.esphome-host/build/host-sdl/.pioenvs/host-sdl/program" >/tmp/host-sdl-run.log 2>&1 &
  wait_for_server
  sleep 1.5
  capture_page 1 "$OUT_ROOT/$layout/01_clock.png"
  capture_page 2 "$OUT_ROOT/$layout/02_temperature.png"
  capture_page 3 "$OUT_ROOT/$layout/03_nordpool.png"
  capture_page 4 "$OUT_ROOT/$layout/04_weather.png"
  capture_page 5 "$OUT_ROOT/$layout/05_status.png"
  cleanup
done

echo "=== All baseline screenshots captured in $OUT_ROOT ==="
