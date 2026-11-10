#!/usr/bin/env python3
"""
Session E1 — Virtual e-Paper C++ Lambda Baseline Screenshot Harness.

Generates `.host-sdl-epaper.yaml` for each target e-Paper device configuration,
compiles and runs it in headless SDL mode under WSL (`host-sdl`), populates
sensors (both live Home Assistant API and realistic/live-fetched initial states
plus multi-hour graph histories), captures BMP snapshots via ESPHome's native
`snapshot.take` action, and converts them to PNGs under:
  tools/gui_migration/epaper_baseline/<device>/<page>.png

Usage:
  python tools/gui_migration/capture_epaper_baseline.py [device-name ...]
  python tools/gui_migration/capture_epaper_baseline.py --generate-only <device-name>
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
import yaml

REPO_ROOT = Path(__file__).resolve().parents[2]
BASELINE_DIR = REPO_ROOT / "tools" / "gui_migration" / "epaper_baseline"
HOST_YAML_PATH = REPO_ROOT / ".host-sdl-epaper.yaml"


# Custom YAML loader that preserves !include / !secret / !extend tags as strings or dicts
class ESPHomeYamlLoader(yaml.SafeLoader):
    pass


def _include_constructor(loader: yaml.SafeLoader, node: yaml.Node):
    if isinstance(node, yaml.ScalarNode):
        val = loader.construct_scalar(node)
        # Normalize relative paths from board/ or board/module/ to repo root
        while val.startswith("../"):
            val = val[3:]
        return f"!include {val}"
    return {}


def _secret_constructor(loader: yaml.SafeLoader, node: yaml.Node):
    return f"!secret {loader.construct_scalar(node)}"


def _extend_constructor(loader: yaml.SafeLoader, node: yaml.Node):
    return f"!extend {loader.construct_scalar(node)}"


def _lambda_constructor(loader: yaml.SafeLoader, node: yaml.Node):
    return f"!lambda {loader.construct_scalar(node)}"


ESPHomeYamlLoader.add_constructor("!include", _include_constructor)
ESPHomeYamlLoader.add_constructor("!secret", _secret_constructor)
ESPHomeYamlLoader.add_constructor("!extend", _extend_constructor)
ESPHomeYamlLoader.add_constructor("!lambda", _lambda_constructor)
ESPHomeYamlLoader.add_constructor("!remove", lambda l, n: None)
ESPHomeYamlLoader.add_constructor(
    "!include_dir_named", lambda l, n: {}
)


def load_yaml(path: Path) -> dict:
    text = path.read_text(encoding="utf-8")
    text = re.sub(r"(?m)^\s*<<:\s*!include_dir_named\b.*$", "", text)
    # Handle multi-document YAML (e.g., board files with `labels: [...] \n ---`)
    docs = list(yaml.load_all(text, Loader=ESPHomeYamlLoader))
    for doc in reversed(docs):
        if isinstance(doc, dict):
            return doc
    return {}


def deep_merge(base: dict, override: dict) -> dict:
    result = dict(base)
    for k, v in override.items():
        if k in result and isinstance(result[k], dict) and isinstance(v, dict):
            result[k] = deep_merge(result[k], v)
        else:
            result[k] = v
    return result


# Live Home Assistant snapshot values (fetched from HA so all sensors have
# valid states immediately on boot even if HA API reconnect is in backoff,
# while `platform: homeassistant` also updates live when connected).
LIVE_HA_TEXT_STATES = {
    "bus_bregnefaret_due_at_0": "23:16",
    "bus_bregnefaret_line_0": "- Seiersten",
    "bus_bregnefaret_due_at_1": "23:29",
    "bus_bregnefaret_line_1": "- Frogn",
    "bus_bregnefaret_due_at_2": "23:59",
    "bus_bregnefaret_line_2": "- Seiersten",
    "folloren_next": "Om 3 dager",
    "folloren_restavfall": "13. Nov",
    "folloren_papir": "18. Des",
    "weather_condition_now": "night-partly-cloudy",
    "weather_condition_next": "snowy-rainy",
    "weather_condition_0": "snowy-rainy",
    "weather_timestamp_0": "23:00",
    "weather_condition_1": "cloudy",
    "weather_timestamp_1": "00:00",
    "weather_condition_2": "cloudy",
    "weather_timestamp_2": "01:00",
    "weather_condition_3": "cloudy",
    "weather_timestamp_3": "02:00",
    "weather_condition_4": "cloudy",
    "weather_timestamp_4": "03:00",
    "nordpool_time_0": "23:00",
    "nordpool_time_4": "03:00",
    "nordpool_time_8": "07:00",
}

LIVE_HA_NUMERIC_STATES = {
    "car_battery_percent": 80.0,
    "car_battery_range": 320.0,
    "weather_temperature_forecast": -0.1,
    "weather_temperature_actual": 2.2,
    "weather_temperature_0": -0.1,
    "weather_temperature_1": -0.1,
    "weather_temperature_2": -0.2,
    "weather_temperature_3": -0.4,
    "weather_temperature_4": -0.8,
    "nordpool_sensor": 82.0,
    "nordpool_price_0": 82.0,
    "nordpool_price_4": 82.0,
    "nordpool_price_8": 97.0,
    "temperature_ute": 2.2,
    "temperature_foran": 0.4,
    "temperature_bak": 4.0,
    "temperature_feels_like": 22.4,
    "temperature_room_feels_like": 22.4,
    "temperature_out_feels_like": -1.0,
}

# Hourly series for graphs so 9h/12h/8h graphs render full realistic curves
GRAPH_SERIES = {
    "trace_nordpool": [78.0, 80.0, 82.0, 82.0, 82.0, 82.0, 85.0, 97.0, 97.0],
    "trace_temperature_ute": [4.2, 3.8, 3.5, 3.1, 2.8, 2.6, 2.4, 2.3, 2.2],
    "trace_temperature_foran": [2.5, 2.1, 1.8, 1.5, 1.2, 0.9, 0.7, 0.5, 0.4],
    "trace_temperature_bak": [5.8, 5.4, 5.1, 4.9, 4.6, 4.4, 4.2, 4.1, 4.0],
    "trace_temperature_room": [21.8, 22.0, 22.1, 22.3, 22.5, 22.6, 22.5, 22.4, 22.4],
}


DEVICES = {
    "waveshare-display-board": {
        "device_yaml": "waveshare-display-board.yaml",
        "board_yamls": ["board/waveshare_epaper_driver.yaml"],
        "width": 480,
        "height": 800,
        "pages": ["page_content"],
        "ip": "192.168.71.110",
        "mac": "A0:B1:C2:D3:E4:01",
        "accents": {},
    },
    "waveshare-weather-station": {
        "device_yaml": "waveshare-weather-station.yaml",
        "board_yamls": ["board/waveshare_epaper_driver.yaml"],
        "width": 300,
        "height": 400,
        "pages": ["page_content"],
        "ip": "192.168.71.111",
        "mac": "A0:B1:C2:D3:E4:02",
        "accents": {},
    },
    "seeed-trmnl-e1002-ec2478": {
        "device_yaml": "seeed-trmnl-e1002-ec2478.yaml",
        "board_yamls": ["board/module/display_reterminal_e1002.yaml"],
        "width": 800,
        "height": 480,
        "pages": ["page_info", "page_content", "page_image"],
        "ip": "192.168.71.37",
        "mac": "EC:24:78:11:22:33",
        "accents": {
            "epaper_accent1_color": "Color(0, 255, 0, 0)",
            "epaper_accent2_color": "Color(0, 0, 255, 0)",
            "epaper_accent3_color": "Color(255, 0, 0, 0)",
        },
    },
    "seeed-trmnl-e1003-190420": {
        "device_yaml": "seeed-trmnl-e1003-190420.yaml",
        "board_yamls": ["board/module/display_reterminal_e1003.yaml"],
        "width": 1872,
        "height": 1404,
        "pages": ["page_info"],
        "ip": "192.168.71.38",
        "mac": "19:04:20:11:22:33",
        "accents": {},
    },
    "seeed-trmnl-e1001-85a7d0": {
        "device_yaml": "seeed-trmnl-e1001-85a7d0.yaml",
        "board_yamls": ["board/module/display_reterminal_e1001.yaml"],
        "width": 800,
        "height": 480,
        "pages": ["page_info"],
        "ip": "192.168.71.36",
        "mac": "85:A7:D0:11:22:33",
        "accents": {},
    },
    "zectrix-note4c": {
        "device_yaml": "zectrix-note4c.yaml",
        "board_yamls": ["board/zectrix_note4c.yaml"],
        "width": 300,
        "height": 400,
        "pages": ["page_info"],
        "ip": "192.168.71.39",
        "mac": "C5:E0:11:22:33:44",
        "accents": {
            "epaper_accent_color": "Color(255, 0, 0, 0)",
            "epaper_accent1_color": "Color(255, 255, 0, 0)",
            "epaper_accent2_color": "Color(255, 0, 0, 0)",
        },
    },
    "elecrow-crowpanel-42-4dbf00": {
        "device_yaml": "elecrow-crowpanel-42-4dbf00.yaml",
        "board_yamls": ["board/elecrow_crowpanel_epaper.yaml"],
        "width": 400,
        "height": 300,
        "pages": ["page_info"],
        "ip": "192.168.70.30",
        "mac": "4D:BF:00:11:22:33",
        "accents": {},
    },
    "elecrow-crowpanel-579-b89ddc": {
        "device_yaml": "elecrow-crowpanel-579-b89ddc.yaml",
        "board_yamls": ["board/elecrow_crowpanel_epaper.yaml"],
        "width": 792,
        "height": 272,
        "pages": ["page_info"],
        "ip": "192.168.70.31",
        "mac": "B8:9D:DC:11:22:33",
        "accents": {},
    },
    "m5s-air-quality-b62db0": {
        "device_yaml": "m5s-air-quality-b62db0.yaml",
        "board_yamls": ["board/m5stack_air_quality.yaml"],
        "width": 200,
        "height": 200,
        "pages": ["page_local", "page_info", "page_room"],
        "ip": "192.168.71.42",
        "mac": "B6:2D:B0:11:22:33",
        "accents": {},
    },
    "m5s-coreink-9fc390": {
        "device_yaml": "m5s-coreink-9fc390.yaml",
        "board_yamls": ["board/m5stack_coreink.yaml"],
        "width": 200,
        "height": 200,
        "pages": ["page_local", "page_info", "page_room", "page_out"],
        "ip": "192.168.71.40",
        "mac": "9F:C3:90:11:22:33",
        "accents": {},
    },
}


def patch_lambda(code: str, device_name: str) -> str:
    code = code.replace("id(component_wifi).is_connected()", "true")
    code = code.replace("id(component_api).is_connected()", "true")
    code = code.replace("App.get_name().c_str()", f'"{device_name}"')
    code = code.replace(
        "id(time_rtc).now().timestamp",
        "(id(time_homeassistant).now().is_valid() ? id(time_homeassistant).now().timestamp : ::time(nullptr))",
    )
    code = code.replace(
        "id(time_homeassistant).now().timestamp",
        "(id(time_homeassistant).now().is_valid() ? id(time_homeassistant).now().timestamp : ::time(nullptr))",
    )
    code = code.replace("esp_random()", "0")
    return code


def extract_pages(docs: list[dict], device_name: str) -> list[dict]:
    pages_by_id: dict[str, dict] = {}
    for doc in docs:
        disp_list = doc.get("display", [])
        if isinstance(disp_list, dict):
            disp_list = [disp_list]
        for disp in disp_list:
            for p in disp.get("pages", []) or []:
                pid = str(p.get("id", ""))
                if pid in ("page_black", "page_white"):
                    continue
                lam = p.get("lambda", "")
                pages_by_id[pid] = {
                    "id": pid,
                    "lambda": patch_lambda(lam, device_name),
                }
    return list(pages_by_id.values())


def extract_fonts(docs: list[dict]) -> list[dict]:
    fonts_by_id: dict[str, dict] = {}
    for doc in docs:
        for f in doc.get("font", []) or []:
            fid = str(f.get("id", ""))
            fcopy = dict(f)
            # Use bpp: 1 so Font::print draws crisp glyphs without blending against COLOR_OFF (black)
            fcopy["bpp"] = 1
            fonts_by_id[fid] = fcopy
    return list(fonts_by_id.values())


def extract_images(docs: list[dict]) -> list[dict]:
    images: list[dict] = []
    for doc in docs:
        for img in doc.get("image", []) or []:
            images.append(img)
    return images


def extract_graphs(docs: list[dict]) -> list[dict]:
    graphs: list[dict] = []
    for doc in docs:
        for g in doc.get("graph", []) or []:
            graphs.append(g)
    return graphs


def extract_ha_entities(docs: list[dict], domain: str) -> list[dict]:
    items_by_id: dict[str, dict] = {}
    for doc in docs:
        for item in doc.get(domain, []) or []:
            if item.get("platform") == "homeassistant":
                iid = str(item.get("id", ""))
                clean = {
                    k: v
                    for k, v in item.items()
                    if k in ("platform", "entity_id", "attribute", "id", "internal")
                }
                items_by_id[iid] = clean
    return list(items_by_id.values())


def generate_host_yaml(device_name: str) -> Path:
    spec = DEVICES[device_name]
    docs = [load_yaml(REPO_ROOT / p) for p in spec["board_yamls"]]
    docs.append(load_yaml(REPO_ROOT / spec["device_yaml"]))

    substitutions: dict = {}
    for doc in docs:
        if "substitutions" in doc:
            substitutions = deep_merge(substitutions, doc["substitutions"])
    substitutions["screen_width"] = str(spec["width"])
    substitutions["screen_height"] = str(spec["height"])

    pages = extract_pages(docs, device_name)
    fonts = extract_fonts(docs)
    images = extract_images(docs)
    graphs = extract_graphs(docs)
    ha_sensors = extract_ha_entities(docs, "sensor")
    ha_text_sensors = extract_ha_entities(docs, "text_sensor")
    ha_binary_sensors = extract_ha_entities(docs, "binary_sensor")

    ha_sensor_ids = {s["id"] for s in ha_sensors}
    ha_text_sensor_ids = {s["id"] for s in ha_text_sensors}
    ha_binary_sensor_ids = {s["id"] for s in ha_binary_sensors}

    # Local hardware sensors that may be referenced by page_info / page_local
    local_numeric_defaults = {
        "battery_voltage": 4.08,
        "battery_level": 92.0,
        "bat_adc": 3.48,
        "bat_percent": 75.0,
        "temperature_sht40": 23.4,
        "humidity_sht40": 38.5,
        "temperature_local": 25.6 if "coreink" in device_name else 28.4,
        "humidity_local": 37.0,
        "pressure_local": 988.4,
        "temperature_scd40_aq": 28.5,
        "temperature_sen55": 28.3,
        "pm1_0_sen55": 1.3,
        "pm2_5_sen55": 1.4,
        "pm4_0_sen55": 1.4,
        "pm10_0_sen55": 1.4,
        "co2_scd40_aq": 883.0,
        "voc_sen55": 63.0,
        "nox_sen55": 1.0,
    }
    local_sensors = []
    for sid, val in local_numeric_defaults.items():
        if sid not in ha_sensor_ids:
            local_sensors.append(
                {
                    "platform": "template",
                    "id": sid,
                    "internal": True,
                    "lambda": f"return {val}f;",
                    "update_interval": "60s",
                }
            )

    local_text_defaults = {
        "sensor_ssid": "IoT-WiFi",
        "sensor_ip_address": spec["ip"],
        "sensor_mac_address": spec["mac"],
    }
    local_text_sensors = []
    for tid, val in local_text_defaults.items():
        if tid not in ha_text_sensor_ids:
            local_text_sensors.append(
                {
                    "platform": "template",
                    "id": tid,
                    "internal": True,
                    "lambda": f'return std::string("{val}");',
                    "update_interval": "60s",
                }
            )

    local_binary_sensors = []
    if "sd_card_detect" not in ha_binary_sensor_ids:
        local_binary_sensors.append(
            {
                "platform": "template",
                "id": "sd_card_detect",
                "internal": True,
                "lambda": "return true;",
            }
        )

    # Build C++ seed lines for HA sensors + graph traces
    seed_lines = [
        "id(initial_data_received) = true;",
        "id(data_updated) = true;",
        "id(epaper_bg_color) = Color(255, 255, 255, 0);",
        "id(epaper_text_color) = Color(0, 0, 0, 0);",
    ]
    for acc_id, acc_val in spec["accents"].items():
        seed_lines.append(f"id({acc_id}) = {acc_val};")

    for sid, val in local_numeric_defaults.items():
        if sid not in ha_sensor_ids:
            seed_lines.append(f"id({sid}).publish_state({val}f);")

    for tid, val in local_text_defaults.items():
        if tid not in ha_text_sensor_ids:
            seed_lines.append(f'id({tid}).publish_state("{val}");')

    for s in ha_sensors:
        sid = s["id"]
        if sid in LIVE_HA_NUMERIC_STATES:
            val = LIVE_HA_NUMERIC_STATES[sid]
            seed_lines.append(
                f"if (!id({sid}).has_state() || std::isnan(id({sid}).state)) id({sid}).publish_state({val}f);"
            )

    for ts in ha_text_sensors:
        tid = ts["id"]
        if tid in LIVE_HA_TEXT_STATES:
            val = LIVE_HA_TEXT_STATES[tid]
            seed_lines.append(
                f'if (!id({tid}).has_state() || id({tid}).state.empty()) id({tid}).publish_state("{val}");'
            )

    # Graph trace seeding
    graph_traces_present = []
    for g in graphs:
        for tr in g.get("traces", []) or []:
            tr_id = str(tr.get("id", ""))
            if tr_id in GRAPH_SERIES:
                graph_traces_present.append(tr_id)

    on_boot_steps: list[dict] = [
        {"lambda": "\n".join(seed_lines)},
        {"delay": "1500ms"},
        {"lambda": "\n".join(seed_lines)},
    ]

    if graph_traces_present:
        init_trace_lines = []
        for tr_id in graph_traces_present:
            init_trace_lines.append(
                f"const_cast<esphome::graph::HistoryData *>(id({tr_id})->get_tracedata())->set_update_time_ms(1);"
            )
        on_boot_steps.append({"lambda": "\n".join(init_trace_lines)})
        for step_idx in range(9):
            step_lines = []
            for tr_id in graph_traces_present:
                val = GRAPH_SERIES[tr_id][step_idx]
                step_lines.append(
                    f"const_cast<esphome::graph::HistoryData *>(id({tr_id})->get_tracedata())->take_sample({val}f);"
                )
            on_boot_steps.append({"delay": "65ms"})
            on_boot_steps.append({"lambda": "\n".join(step_lines)})

    for page_id in spec["pages"]:
        on_boot_steps.extend(
            [
                {"display.page.show": page_id},
                {"component.update": "display_eink"},
                {"delay": "150ms"},
                {
                    "snapshot.take": {
                        "id": "display_eink",
                        "filename": f"{page_id}.bmp",
                    }
                },
                {"delay": "100ms"},
            ]
        )

    on_boot_steps.append(
        {"logger.log": "[EPAPER_BASELINE] ALL_SNAPSHOTS_DONE"}
    )
    on_boot_steps.append(
        {"lambda": "exit(0);"}
    )

    config: dict = {
        "substitutions": substitutions,
        "host": {"mac_address": "02:5D:1A:00:00:01"},
        "esphome": {
            "name": "host-sdl",
            "friendly_name": f"Host SDL ({device_name})",
            "on_boot": [{"priority": -200.0, "then": on_boot_steps}],
        },
        "logger": {"level": "INFO"},
        "api": {
            "id": "component_api",
            "reboot_timeout": "90min",
            "encryption": {"key": "!secret homeassistant_api"},
        },
        "time": [
            {
                "platform": "homeassistant",
                "id": "time_homeassistant",
                "timezone": "Europe/Oslo",
            }
        ],
        "globals": [
            {"id": "data_updated", "type": "bool", "restore_value": False, "initial_value": "true"},
            {"id": "initial_data_received", "type": "bool", "restore_value": False, "initial_value": "true"},
            {"id": "recorded_display_refresh", "type": "int", "restore_value": False, "initial_value": "1"},
            {"id": "epaper_bg_color", "type": "Color", "restore_value": False, "initial_value": "Color(255, 255, 255, 0)"},
            {"id": "epaper_text_color", "type": "Color", "restore_value": False, "initial_value": "Color(0, 0, 0, 0)"},
            {"id": "epaper_accent_color", "type": "Color", "restore_value": False, "initial_value": "Color(255, 0, 0, 0)"},
            {"id": "epaper_accent1_color", "type": "Color", "restore_value": False, "initial_value": "Color(0, 255, 0, 0)"},
            {"id": "epaper_accent2_color", "type": "Color", "restore_value": False, "initial_value": "Color(0, 0, 255, 0)"},
            {"id": "epaper_accent3_color", "type": "Color", "restore_value": False, "initial_value": "Color(255, 0, 0, 0)"},
        ],
        "color": [
            {"id": "color_black", "red_int": 0, "green_int": 0, "blue_int": 0},
            {"id": "color_white", "red_int": 255, "green_int": 255, "blue_int": 255},
            {"id": "color_red", "red_int": 255, "green_int": 0, "blue_int": 0},
            {"id": "color_green", "red_int": 0, "green_int": 255, "blue_int": 0},
            {"id": "color_blue", "red_int": 0, "green_int": 0, "blue_int": 255},
            {"id": "color_yellow", "red_int": 255, "green_int": 255, "blue_int": 0},
        ],
        "display": [
            {
                "platform": "sdl",
                "id": "display_eink",
                "headless": True,
                "dimensions": {
                    "width": spec["width"],
                    "height": spec["height"],
                },
                "update_interval": "never",
                "pages": pages,
            }
        ],
        "font": fonts,
        "sensor": ha_sensors + local_sensors,
        "text_sensor": ha_text_sensors + local_text_sensors,
        "binary_sensor": ha_binary_sensors + local_binary_sensors,
    }

    if images:
        config["image"] = images
    if graphs:
        config["graph"] = graphs

    # Custom YAML dump that emits !include tags unquoted
    raw_yaml = yaml.safe_dump(config, sort_keys=False, allow_unicode=True, width=120)
    raw_yaml = re.sub(r"'!include ([^']+)'", r"!include \1", raw_yaml)
    raw_yaml = re.sub(r"'!secret ([^']+)'", r"!secret \1", raw_yaml)
    HOST_YAML_PATH.write_text(raw_yaml, encoding="utf-8")
    return HOST_YAML_PATH


def run_capture_in_wsl(device_name: str) -> list[Path]:
    spec = DEVICES[device_name]
    generate_host_yaml(device_name)

    out_dir = BASELINE_DIR / device_name
    out_dir.mkdir(parents=True, exist_ok=True)

    # Ensure /mnt/ha-config is mounted in WSL (same as tools/host_sdl/host-sdl.ps1)
    subprocess.run(
        [
            "wsl",
            "-d",
            "Ubuntu",
            "-u",
            "root",
            "--cd",
            "~",
            "--",
            "bash",
            "-c",
            "mkdir -p /mnt/ha-config && (mountpoint -q /mnt/ha-config || mount -t drvfs '\\\\192.168.68.100\\config' /mnt/ha-config)",
        ],
        check=True,
    )

    wsl_script = f"""
set -euo pipefail
export ESPHOME_DATA_DIR="$HOME/.esphome-host"
SNAP_DIR="/tmp/epaper_snapshots/{device_name}"
rm -rf "$SNAP_DIR"
mkdir -p "$SNAP_DIR"

cd /mnt/ha-config/esphome
"$HOME/.venvs/esphome/bin/esphome" compile .host-sdl-epaper.yaml

BIN="$ESPHOME_DATA_DIR/build/host-sdl/.pioenvs/host-sdl/program"
export ESPHOME_SNAPSHOT_DIR="$SNAP_DIR"

timeout 25s "$BIN"

"$HOME/.venvs/esphome/bin/python" - <<'PY'
from pathlib import Path
from PIL import Image

snap_dir = Path("/tmp/epaper_snapshots/{device_name}")
out_dir = Path("/mnt/ha-config/esphome/tools/gui_migration/epaper_baseline/{device_name}")
out_dir.mkdir(parents=True, exist_ok=True)

for bmp in sorted(snap_dir.glob("*.bmp")):
    png = out_dir / (bmp.stem + ".png")
    img = Image.open(bmp)
    img.save(png, "PNG")
    print(f"Saved {{png}} ({{img.size[0]}}x{{img.size[1]}})")
PY
"""
    try:
        proc = subprocess.run(
            ["wsl", "-d", "Ubuntu", "--cd", "~", "-e", "bash", "-c", wsl_script],
            text=True,
            capture_output=True,
            encoding="utf-8",
            errors="replace",
        )
        print(proc.stdout)
        if proc.returncode != 0:
            print(proc.stderr, file=sys.stderr)
            raise RuntimeError(f"Capture failed for {device_name} (exit {proc.returncode})")
    finally:
        if HOST_YAML_PATH.exists():
            HOST_YAML_PATH.unlink(missing_ok=True)

    return [out_dir / f"{p}.png" for p in spec["pages"]]


def main() -> None:
    parser = argparse.ArgumentParser(description="Capture e-Paper C++ lambda baseline PNGs via host-sdl")
    parser.add_argument("devices", nargs="*", help="Device names (default: all)")
    parser.add_argument("--generate-only", action="store_true", help="Only write .host-sdl-epaper.yaml")
    args = parser.parse_args()

    targets = args.devices or list(DEVICES.keys())
    for dev in targets:
        if dev not in DEVICES:
            raise SystemExit(f"Unknown device '{dev}'. Choices: {', '.join(DEVICES)}")
        print(f"=== [{dev}] ({DEVICES[dev]['width']}x{DEVICES[dev]['height']}) ===")
        if args.generate_only:
            p = generate_host_yaml(dev)
            print(f"Generated {p}")
        else:
            pngs = run_capture_in_wsl(dev)
            for png in pngs:
                print(f"  -> {png}")


if __name__ == "__main__":
    main()
