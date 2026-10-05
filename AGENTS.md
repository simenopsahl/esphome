# ESPHome Modular Architecture — Agent Guidelines

Welcome to the ESPHome split-configuration repository. This project manages multiple ESP32/ESP8266/ESP32-S3/ESP32-P4/ESP32-C3/ESP32-C6 devices (primarily M5Stack, Waveshare, Seeed, and GeekMagic hardware) using a highly modular, package-based architecture.

This document serves as the canonical guide for AI coding assistants operating within this repository.

---

## 1. Core Architectural Paradigm

Instead of monolithic single-file configurations, devices are composed dynamically using ESPHome's `packages` and `substitutions` features:

```text
Root Device YAML (e.g., m5s-fire-a70780.yaml)
  ├── common/           (Shared base: Wi-Fi, OTA, API, time, logger, fallback restart)
  ├── bluetooth/        (BLE tracker & Home Assistant Bluetooth Proxy)
  ├── board/            (Platform, framework, clock, flash, display, onboard peripherals)
  │     └── board/pins/ (Substitutions mapping physical GPIOs to semantic pin names)
  ├── board/extension/  (Headers, HATs, Grove buses, MBUS breakout units)
  ├── board/module/     (Sensors, e-Paper controllers, environment monitors)
  ├── features/         (Cross-device logic: NordPool alerts, combined sensor templates)
  └── gui/              (Resolution-specific LVGL configurations, themes, pages, widgets)
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
| **[`gui/`](file:///x:/esphome/gui)** | Modular LVGL UI system. | Organized by screen resolution (e.g. `240x320.yaml`). Subdirectories: `pages/`, `widgets/`, `themes/`, `styles/`, `fonts/`. |
| **[`pinmap/`](file:///x:/esphome/pinmap)** | Reference pinout sheets. | Human and agent documentation for hardware connectors. |

---

## 3. Verification & CLI Workflows

The local environment has the ESPHome CLI installed:

```powershell
# 1. Validate YAML syntax and substitution resolution (fast):
esphome config <device-config.yaml>

# 2. Dry-run compile without flashing (checks C++ code generation and library dependencies):
esphome compile <device-config.yaml>

# 3. Clean platformio build cache for a device (if build artifacts become stale):
esphome clean <device-config.yaml>
```

> [!IMPORTANT]
> Always run `esphome config <target>.yaml` after creating or editing a configuration to verify that all packages, includes, substitutions, and IDs resolve correctly.

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

