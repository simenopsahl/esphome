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

Both Windows 11 (native) and WSL (`~/.venvs/esphome`) have the ESPHome CLI installed:

- **Use the Windows 11 ESPHome installation for configuration validation, compilation, flashing, and logs**: Because the workspace resides on a Windows/SMB path (`X:\esphome`), running ESPHome directly on Windows 11 avoids WSL2 cross-filesystem (`drvfs`/9P) I/O overhead and provides direct access to USB COM ports.
- **Use the WSL ESPHome installation only for `host-sdl` virtual display builds and screenshot verification**: The `host` platform with SDL2 requires Linux (`pwsh tools/host_sdl/host-sdl.ps1 ...`).

### Windows Compilation & Local Build Path (SMB / UNC Fix)
Because `X:\esphome` maps to a network SMB share (`\\192.168.68.100\config\esphome`), `cmd.exe` and CMake will fail if building directly on the share (`UNC paths are not supported` or `Permission denied` on compiler test binaries).
To build locally without errors:
1. Temporarily specify a local Windows path in the root device YAML under `esphome.build_path`:
   ```yaml
   esphome:
     build_path: C:/Users/simen/.esphome_build/<device-name>
   ```
2. When executing build/flash commands in the agent environment, run with unsandboxed permissions (`BypassSandbox: true`) because SMB network access and local COM ports require host elevation.

> [!WARNING]
> **Remove `build_path` after local testing**: The Home Assistant ESPHome Dashboard runs in a Linux container on the server and cannot access Windows `C:/...` paths. Leaving `build_path: C:/...` in a device YAML will break subsequent updates and installs run from the Home Assistant ESPHome dashboard. Always remove or comment out `build_path` once local verification and flashing are complete.

### Common CLI Commands

```powershell
# 1. Validate YAML syntax and substitution resolution (on Windows 11):
esphome config <device-config.yaml>

# 2. Compile firmware locally (writes binary & ELF to local build_path):
esphome compile <device-config.yaml>

# 3. Flash / Upload via local USB COM port:
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

## 5. Agent Skills Available

Specialized multi-step procedures are located in `.agents/skills/`:

- **`esphome-validate`**: Automated syntax validation and dry-run compilation.
- **`esphome-new-device`**: Scaffold a new device configuration following repo conventions.
- **`esphome-add-board`**: Guide for adding a new board or pinout mapping.
- **`esphome-update`**: Guide for updating and synchronizing local ESPHome CLI with HA/build servers.
