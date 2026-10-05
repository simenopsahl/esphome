---
name: esphome-new-device
description: >-
  Use this skill when scaffolding or generating a new ESPHome device configuration file in this repository.
---

# Scaffolding a New ESPHome Device

This skill defines the step-by-step workflow for creating a new device configuration file following this repository's modular architecture.

---

## Workflow

### 1. Identify Hardware & Name the Configuration
- File naming pattern: `<prefix>-<model>-<id>.yaml` (e.g., `m5s-atom-s3-lite-1234ab.yaml`, `seeed-xiao-s3-5678cd.yaml`).
- Inspect [`board/`](../../board) to choose the matching base hardware configuration (e.g., `board/m5stack_atom_s3_lite.yaml`).
- Inspect [`board/pins/`](../../board/pins) to verify the pin mappings for that board.

### 2. Assemble the Root YAML Package Inclusions
Every device configuration starts by including the shared packages, followed by hardware-specific modules:

```yaml
packages:
  # Base system packages
  <<: !include_dir_named common
  <<: !include_dir_named bluetooth # optional, if BLE proxy/tracker needed

  # Board definition (automatically includes corresponding pins)
  board: !include board/<board_name>.yaml

  # Extensions (e.g., MBUS, Grove, HATs) - optional
  # mbus_i2c: !include board/extension/mbus_i2c.yaml

  # Modules and sensors - optional
  # env_iv: !include board/module/unit_env_iv.yaml

  # GUI (if device has a display) - optional
  # colors: !include gui/themes/colors_night_owl.yaml
  # lvgl: !include gui/<resolution>.yaml

  # Features - optional
  # combined_env_sensors: !include features/combined_env_sensors.yaml
```

### 3. Define Substitutions
Specify the device's unique parameters:

```yaml
substitutions:
  common:
    log_level: WARN
  device:
    name: <device-file-name-without-yaml>
    location: "Living Room"
  module:
    # Route module I2C/SPI buses if needed:
    env_i2c_bus: i2c_internal
```

### 4. Configure Device Metadata and Network
Set up ESPHome naming and Wi-Fi overrides:

```yaml
esphome:
  comment: ${device.model} @ ${device.location}
  name: ${device.name}
  friendly_name: ${device.name}
  area: ${device.location}

wifi:
  use_address: ${device.name}.local
  # Optional static IP configuration:
  # manual_ip:
  #   gateway: 192.168.68.1
  #   subnet: 255.255.252.0
  #   static_ip: 192.168.71.xx
```

### 5. Wire Peripherals and Sensor Extensions (`!extend`)
Never redefine existing sensors from packages; extend them:

```yaml
binary_sensor:
  # Physical buttons provided by the board config:
  - id: !extend btn_a
    on_press:
      - logger.log: "Button Pressed"

sensor:
  # Combine local sensor feeds into virtual sensors if using combined_env_sensors:
  - id: !extend temperature_local
    sources:
      - source: temperature_sht40_unit_env
```

### 6. Validate
Run the validation command:

```powershell
esphome config <new-device-config.yaml>
```
Ensure no undefined substitutions or include errors are reported.

