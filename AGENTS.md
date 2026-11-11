# ESPHome Modular Architecture — Agent Guidelines

Welcome to the ESPHome split-configuration repository. This project manages multiple ESP32/ESP8266/ESP32-S3/ESP32-P4/ESP32-C3/ESP32-C6 devices (primarily M5Stack, Waveshare, Seeed, and GeekMagic hardware) using a highly modular, package-based architecture.

This document serves as the canonical guide for AI coding assistants operating within this repository.

---

## 1. Core Architectural Paradigm

Instead of monolithic single-file configurations, devices are composed dynamically using ESPHome's `packages` and `substitutions` features:

```text
Root Device YAML (e.g., m5s-fire-a70780.yaml, jc4880p443.yaml)
  ├── common/           (Shared base: Wi-Fi, OTA, API, time, logger, fallback restart)
  ├── bluetooth/        (BLE tracker & Home Assistant Bluetooth Proxy)
  ├── board/            (Platform, framework, clock, flash, display, onboard peripherals)
  │     └── board/pins/ (Substitutions mapping physical GPIOs to semantic pin names)
  ├── board/extension/  (Headers, HATs, Grove buses, MBUS breakout units)
  ├── board/module/     (Sensors, e-Paper controllers, environment monitors)
  ├── features/         (Cross-device logic: NordPool alerts, combined sensor templates)
  └── gui/              (Responsive LVGL UI engine — see gui/ARCHITECTURE.md)
        ├── main.yaml   (Unified responsive entry point; <W>x<H>.yaml stubs set dimensions)
        ├── common/     (Top-layer chrome/header/footer, boot_screen, ota_screen)
        ├── fonts/      (Responsive 5-tier font sizing in main.yaml + MDI glyphs.yaml)
        ├── functions/  (Auto-display, HA nav, swipe nav, number/text label helpers)
        ├── pages/      (Full-screen pages: clock, temperature, nordpool, weather, status, climate, grid_dashboard, modules)
        ├── styles/     (Shared LVGL style_definitions: page_styles.yaml, container_styles.yaml)
        ├── themes/     (Widget theme defaults in main.yaml + color palettes colors_*.yaml)
        └── tiles/      (Modular grid_dashboard tiles: climate, light, switch, button, sensor, binary_sensor, text_sensor)
```

### Key Composition Rules

1. **Never redefine components; use `!extend`**:
   - Shared components declared in `common/`, `board/`, or `features/` (like `temperature_local`, `btn_a`, `presence_detected`) should be extended in device YAMLs using `- id: !extend <id>`.
2. **Keep Shared Packages Generic**:
   - Never hardcode device-specific IDs, entity names, or static IPs inside files in `common/`, `board/`, or `gui/`.
   - All customizations for a specific physical node belong in the root device YAML file.
3. **Use the Substitution Hierarchy**:
   - Pins are defined in `board/pins/<board>.yaml` using the `${pin...}` namespace (e.g., `${pin.int.spi.clk}`, `${pin.btn.a}`).
   - Hardware buses and parameters are parameterized via `${module...}` and `${device...}` substitutions.

---

## 2. Directory Layout & Lookup Guide

| Directory | Purpose | Key Conventions |
| :--- | :--- | :--- |
| **Root (`/`)** | Deployed device nodes (e.g., `m5s-fire-a70780.yaml`, `ulanzi.yaml`). | Named `<prefix>-<model>-<id>.yaml`. Includes packages, defines substitutions, handles static IP and room-specific logic. |
| **[`board/`](file:///x:/esphome/board)** | Board-level hardware declarations. | Specifies `esp32:` platform, clock speed, flash size, onboard display drivers, and pulls `board/pins/<board>.yaml`. |
| **[`board/pins/`](file:///x:/esphome/board/pins)** | Hardware pin mappings. | Contains substitutions only (`substitutions: pin: ...`). Decouples code from physical GPIO assignments. |
| **[`board/extension/`](file:///x:/esphome/board/extension)** | Expansion buses and hats. | Defines I2C/SPI/UART buses (e.g. `mbus_i2c.yaml`, `grove_i2c.yaml`, `atom_mate_hat.yaml`). |
| **[`board/module/`](file:///x:/esphome/board/module)** | Modular peripherals & sensors. | Reusable unit configurations (e.g. `unit_env_iv.yaml`, `unit_co2.yaml`, `module_cc1101_async.yaml`). |
| **[`bluetooth/`](file:///x:/esphome/bluetooth)** | Bluetooth functionality. | BLE tracker & BLE proxy packages (`bluetooth_proxy.yaml`, `esp32_ble_tracker.yaml`). |
| **[`common/`](file:///x:/esphome/common)** | Universal base packages. | Wi-Fi, OTA, Home Assistant API, logger, fallback reboot, status sensors. Included via `<<: !include_dir_named common`. |
| **[`features/`](file:///x:/esphome/features)** | Cross-device feature bundles. | Combined virtual sensors (`combined_env_sensors.yaml`), NordPool power indicators, IR commands. |
| **[`gui/`](file:///x:/esphome/gui)** | Modular responsive LVGL UI system. | Entry point [`gui/main.yaml`](file:///x:/esphome/gui/main.yaml) (and `<W>x<H>.yaml` stubs). Subdirectories: `common/`, `fonts/`, `functions/`, `pages/`, `styles/`, `themes/`, `tiles/`. Detailed guide: [`gui/ARCHITECTURE.md`](file:///x:/esphome/gui/ARCHITECTURE.md) & [`gui/LVGL_REFERENCE.md`](file:///x:/esphome/gui/LVGL_REFERENCE.md). |
| **[`pinmap/`](file:///x:/esphome/pinmap)** | Reference pinout sheets. | Human and agent documentation for hardware connectors. |

---

## 3. Verification & CLI Workflows

Both Windows 11 (native) and WSL (`~/.venvs/esphome`) have the ESPHome CLI installed. In addition, a high-performance LXC remote build server and File Browser are integrated with Home Assistant.

- **Fastest Compilation (Preferred)**: Offload compilation to the remote build server (`pve-esphome` @ `192.168.68.110`) via the `homeassistant` MCP tool `compile` (or `esphome-device-builder__compile`). This compiles in **~15–30 seconds** on local NVMe storage, bypassing SMB network overhead and eliminating the need to set/unset local `build_path` overrides.
- **Local Windows 11 CLI**: Use for fast syntax validation (`esphome config <device.yaml>`), direct USB serial flashing/logging (`COM<N>`), and fallback offline compilation.
- **WSL ESPHome CLI**: Reserved for `host-sdl` virtual display builds and screenshot verification (`pwsh tools/host_sdl/host-sdl.ps1 ...`).

---

### Remote Compilation & Artifact Retrieval Workflow (Recommended)

1. **Queue Build via Home Assistant MCP**:
   Call `call_mcp_tool` with `ServerName: "homeassistant"`, `ToolName: "compile"` (or `"esphome-device-builder__compile"`), and `configuration: "<device-config>.yaml"`.
2. **Monitor Build Progress**:
   Poll status with `get_job(job_id=...)` until completed.
3. **Download Firmware & ELF via Helper Script**:
   Always use the standalone helper script [`tools/fetch_firmware.py`](file:///x:/esphome/tools/fetch_firmware.py) as a single command (no shell variables, pipes, or command chains) so Antigravity prefix-matching can auto-approve it:
   ```powershell
   python tools/fetch_firmware.py <device-name>
   ```
   - The script automatically inspects `<device-name>.yaml` to resolve `device.name` when the remote build directory name differs from the YAML filename (or you can pass `[remote-short-name]` and `[build-id]` explicitly).
   - It authenticates against `http://192.168.68.110:8080/api/login` and downloads both `firmware.factory.bin` (for USB flashing at offset `0x0`) and `firmware.elf` (for `addr2line` crash decoding) into `C:\Users\simen\.esphome_build\<device-name>\`.

---

### Flashing Firmware over USB (`esptool`)

When flashing downloaded firmware directly via COM port:
```powershell
# Flash factory combined binary to offset 0x0:
esptool --chip <chip> --port COM<N> --baud <baud> --before default-reset --after hard-reset write-flash -z 0x0 "C:\Users\simen\.esphome_build\<device>\firmware.factory.bin"
```
> [!TIP]
> **Baud Rate Selection**:
> - **ESP32 with USB-to-UART bridge (CP2104 / CH9102 / CH340, e.g. Atom Matrix / Atom Lite)**: Use `--baud 115200`. Higher baud rates often cause RTS/DTR dropouts during baud switching on these bridges.
> - **Native USB CDC / JTAG (ESP32-S3, ESP32-C3, ESP32-C6, ESP32-P4)**: Supports higher rates (`--baud 460800` or `--baud 921600`).

---

### Windows Local Compilation & Build Path (Fallback / Offline)

Because `X:\esphome` maps to a network SMB share (`\\192.168.68.100\config\esphome`), `cmd.exe` and CMake will fail if building directly on the share (`UNC paths are not supported` or `Permission denied` on compiler test binaries).
To build locally on Windows without errors:
1. Temporarily specify a local Windows path in the root device YAML under `esphome.build_path`:
   ```yaml
   esphome:
     build_path: C:/Users/simen/.esphome_build/<device-name>
   ```
2. When executing build/flash commands in the agent environment, run with unsandboxed permissions (`BypassSandbox: true`) because SMB network access and local COM ports require host elevation.

> [!WARNING]
> **Remove `build_path` after local testing**: The Home Assistant ESPHome Dashboard runs in a Linux container on the server and cannot access Windows `C:/...` paths. Leaving `build_path: C:/...` in a device YAML will break subsequent updates and installs run from the Home Assistant ESPHome dashboard. Always remove or comment out `build_path` once local verification and flashing are complete.

---

### Common CLI Commands

```powershell
# 1. Validate YAML syntax and substitution resolution (on Windows 11):
esphome config <device-config.yaml>

# 2. Compile firmware locally (writes binary & ELF to local build_path):
esphome compile <device-config.yaml>

# 3. Flash / Upload via local USB COM port (using esphome):
esphome upload <device-config.yaml> --device COM<N>

# 4. Flash / Upload via OTA over Wi-Fi:
esphome upload <device-config.yaml> --device <IP-or-hostname>

# 5. Monitor logs (always specify --device to avoid interactive prompt blocks):
esphome logs <device-config.yaml> --device 192.168.xx.xx
esphome logs <device-config.yaml> --device COM<N>

# 6. Clean build cache if artifacts become corrupted:
esphome clean <device-config.yaml>

# 7. Virtual host-sdl LVGL display (uses WSL):
pwsh tools/host_sdl/host-sdl.ps1 config 240x320
pwsh tools/host_sdl/host-sdl.ps1 run 240x320
pwsh tools/host_sdl/host-sdl.ps1 screenshot
```

### Automated Log Sampling via Python (Agent Tasks)
When capturing logs non-interactively in an automated script or background task, prevent Python buffering by using `bufsize=1` and `python -u`:
```python
import subprocess, sys, time

proc = subprocess.Popen(
    ['esphome', 'logs', '<device.yaml>', '--device', '<IP-or-COM>'],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1, encoding='utf-8'
)
end = time.time() + 8
while time.time() < end:
    line = proc.stdout.readline()
    if line:
        sys.stdout.write(line)
        sys.stdout.flush()
proc.kill()
```

### Decoding Crash Backtraces (`addr2line`)
When an ESP32 panics or aborts (`[E][esp32.crash:...]`), decode the backtrace addresses (`PC` and `BT0`..`BTn`) using the architecture-specific `addr2line` and the local `.elf` binary:
```powershell
# For Xtensa (ESP32 / ESP32-S3):
xtensa-esp32s3-elf-addr2line -pfiaC -e C:\Users\simen\.esphome_build\<device>\build\firmware.elf <addresses...>

# For RISC-V (ESP32-P4 / ESP32-C3 / ESP32-C6):
riscv32-esp-elf-addr2line -pfiaC -e C:\Users\simen\.esphome_build\<device>\build\firmware.elf <addresses...>
```

> [!IMPORTANT]
> Always run `esphome config <target>.yaml` on Windows 11 after creating or editing a configuration to verify that all packages, includes, substitutions, and IDs resolve correctly. When batch-validating multiple devices, validate at most 8 devices per command invocation to avoid command timeouts.

---

## 4. Coding Conventions & Safety

1. **Secrets & Privacy**:
   - Never hardcode passwords, SSIDs, or private tokens. Always use `!secret <secret_key>`.
   - `secrets.yaml` is gitignored and must never be committed to source control.
2. **Device Naming Conventions**:
   - Device YAML filenames: `<vendor/prefix>-<model>-<mac_suffix>.yaml` (e.g. `m5s-atom-s3-lite-953844.yaml`, `seeed-xiao-s3-d09224.yaml`).
   - `device.name` substitution matches the filename base without `.yaml`.
3. **LVGL Component IDs**:
   - Labels: `lbl_<purpose>` or `lbl_<sensor>_value`
   - Buttons: `btn_<purpose>`
   - Pages: `page_<name>` (e.g. `page_clock`, `page_temperature`, `page_weather`)
4. **Sensor Aggregation Pattern**:
   - Environmental sensors feed into `temperature_local`, `humidity_local`, and `pressure_local` templates via `sources:`.
   - In device YAMLs, extend `temperature_local` and add active sensor sources under `sources: - source: <sensor_id>`.

---

## 5. MCP Server Integrations & Version Control

The workspace is configured with MCP servers for **Home Assistant (`ha-mcp` & `homeassistant`)**, **GitHub (`github-mcp-server`)**, and **Proxmox (`proxmox`)**:

### A. Home Assistant MCP Servers (`ha-mcp` & `homeassistant`)

Two complementary Home Assistant MCP servers run side-by-side. Because most ESPHome diagnostic entities, internal sensors, and newly added device entities are **not exposed to Assist LLMs by default**, always prefer **`ha-mcp`** when inspecting entities or devices, and use **`homeassistant`** for ESPHome Device Builder compilation and Spook reference checks.

#### 1. `ha-mcp` Server (Unrestricted Entity, Device & Registry Access — Preferred for Inspection)
Powered by [`homeassistant-ai/ha-mcp`](https://github.com/homeassistant-ai/ha-mcp), providing full administrative access to **all** Home Assistant entities, devices, and registries regardless of Assist exposure:
- **Entity Search & Live State Inspection**:
  - `ha_search`: Fuzzy search across all entities, domains, and areas (finds non-exposed ESPHome sensors, diagnostic entities, buttons, and switches).
  - `ha_get_state` / `ha_get_entity` / `ha_set_entity`: Read live states/attributes (e.g., `sensor.waveshare_display_data`, `sensor.waveshare_nordpool_future_offset`, local environmental sensors, RSSI/uptime diagnostics) or enable/update disabled-by-default ESPHome entities in the registry.
- **Device & Area Registry Management**:
  - `ha_get_device` / `ha_set_device` & `ha_list_floors_areas`: Inspect ESPHome device registry entries, verify connectivity after flashing, and assign nodes to rooms/areas.
- **Service Calls, Templates, History & Logs**:
  - `ha_call_service`: Trigger ESPHome buttons, switches, lights, or custom API services during testing.
  - `ha_eval_template`: Render and test Jinja2 templates against live states.
  - `ha_get_history` / `ha_get_logs`: Inspect sensor state history (e.g., verifying combined sensor smoothing) or check HA Core logs for `esphome` integration errors.

#### 2. `homeassistant` Server (ESPHome Device Builder & Custom LLM API Bridge)
Uses Home Assistant's built-in `mcp_server` integration to bridge custom LLM API providers not present in `ha-mcp`:
- **ESPHome Device Builder (`compile`, `get_job`, `validate_config`, `install`, `list_devices`)**:
  - Offload firmware compilation and validation to the `pve-esphome` build server (`192.168.68.110`), as detailed in Section 3.
- **Spook Diagnostics (`spook__list_ghosts`, `spook__find_usages`, `spook__check_references`)**:
  - Check where an ESPHome entity is referenced across Home Assistant automations, dashboards, and groups before renaming or removing it, or detect orphaned "ghost" entities after a config change.
- **Assist Context (`homeassistant__GetLiveContext`)**:
  - Restricted to Assist-exposed entities only; do **not** rely on it to verify whether an ESPHome entity exists.

#### When to Use Which Home Assistant MCP Server
| Task | Server to Use | Key Tool(s) |
| :--- | :--- | :--- |
| Find or inspect **any** ESPHome entity (including non-exposed/diagnostic) | **`ha-mcp`** | `ha_search`, `ha_get_state`, `ha_get_entity` |
| Enable disabled-by-default ESPHome entities or update area/icon/labels | **`ha-mcp`** | `ha_set_entity`, `ha_get_device`, `ha_set_device` |
| Trigger an ESPHome button/switch/light or test Jinja2 templates | **`ha-mcp`** | `ha_call_service`, `ha_eval_template` |
| Check sensor history or HA Core logs for ESPHome connection issues | **`ha-mcp`** | `ha_get_history`, `ha_get_logs` |
| Remote-compile, validate, or poll build jobs on `pve-esphome` | **`homeassistant`** | `compile`, `get_job`, `validate_config`, `list_devices` |
| Check if renaming/removing an ESPHome entity breaks HA automations/dashboards | **`homeassistant`** | `spook__find_usages`, `spook__check_references`, `spook__list_ghosts` |

---

### B. Git / GitHub Commits (`github-mcp-server` & Local Git)
- **Repository**: `owner: "simenopsahl"`, `repo: "esphome"`, `branch: "main"` (`https://github.com/simenopsahl/esphome.git`).
- After completing any significant edit, feature, or migration session (and verifying with `esphome config`), commit the changes to Git/GitHub with a clear, descriptive commit message (using `github-mcp-server` tools like `push_files` / `create_or_update_file` or local `git` commands with `BypassSandbox: true` since `X:\esphome` is an SMB share).
- Never stage or commit `secrets.yaml`, temporary `.host-sdl*` build artifacts, or local `build_path` overrides.

### C. Proxmox MCP (`proxmox`)
- Connects to the `thinkstation` Proxmox host (`192.168.68.101`). Can be used to inspect status, resource usage, or logs of the `pve-esphome` LXC container (`vmid: 110` @ `192.168.68.110`) if remote builds stall or fail.

---

## 6. Agent Skills Available

Specialized multi-step procedures are located in `.agents/skills/`:

- **`esphome-validate`**: Automated syntax validation and dry-run compilation.
- **`esphome-new-device`**: Scaffold a new device configuration following repo conventions.
- **`esphome-add-board`**: Guide for adding a new board or pinout mapping.
- **`esphome-update`**: Guide for updating and synchronizing local ESPHome CLI with HA/build servers.
