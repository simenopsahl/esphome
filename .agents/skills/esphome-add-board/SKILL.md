---
name: esphome-add-board
description: >-
  Use this skill when adding a new hardware board variant, pinout definition, or microcontroller model to the repository.
---

# Adding a New Hardware Board Variant

This skill defines the process for onboarding a new development board or microcontroller variant into this repository.

---

## 1. Create the Pinout Mapping (`board/pins/<board_name>.yaml`)

Hardware pin mappings must be isolated in `board/pins/` and defined strictly through substitutions. This keeps driver declarations generic.

### Structure:
```yaml
substitutions:
  pin:
    # Internal SPI / Display Bus
    int:
      spi:
        clk: GPIOxx
        mosi: GPIOxx
        miso: GPIOxx
        disp:
          cs: GPIOxx
          dc: GPIOxx
          reset: GPIOxx
    # Internal I2C Bus
    i2c:
      sda: GPIOxx
      scl: GPIOxx
    # Buttons
    btn:
      a: GPIOxx
      b: GPIOxx
    # Backlight / LEDs
    light:
      disp_bl: GPIOxx
      status_led: GPIOxx
```

---

## 2. Create the Board Definition (`board/<board_name>.yaml`)

Create the core board file in `board/`:

```yaml
# Documentation link and hardware notes
# Base functions for: <Board Model>

packages:
  pins: !include pins/<board_name>.yaml

substitutions:
  device:
    model: "<Board Full Name>"
    framework:
      type: esp-idf # or arduino
  module:
    internal:
      i2c_bus: i2c_internal

esphome:
  project:
    name: "<Vendor>.<Model>"
    version: "1.0"

esp32:
  variant: ESP32 # ESP32S3, ESP32C3, ESP32C6, etc.
  flash_size: 16MB
  cpu_frequency: 240MHz
  framework:
    type: ${device.framework.type}

# Configure onboard peripherals using the substitutions:
# spi:
#   clk_pin: ${pin.int.spi.clk}
#   mosi_pin: ${pin.int.spi.mosi}
#   miso_pin: ${pin.int.spi.miso}

# display:
#   - platform: ...
```

---

## 3. Register the Board

1. Add the new device name to the list in [`board/README.md`](../../board/README.md).
2. If the board has unique expansion connectors, add any relevant bus definitions to [`board/extension/`](../../board/extension).

---

## 4. Verification

Create a temporary test configuration or point an existing test node at `board: !include board/<board_name>.yaml` and run:

```powershell
esphome config <test-device.yaml>
```

Verify that all GPIO substitution variables resolve without syntax or validation errors.

