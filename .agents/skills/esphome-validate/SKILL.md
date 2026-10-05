---
name: esphome-validate
description: >-
  Use this skill to validate YAML syntax, verify substitution variables, or test C++ compilation of an ESPHome configuration file using the ESPHome CLI.
---

# ESPHome Configuration Validation

This skill defines the procedure for verifying ESPHome YAML configurations in this repository.

## When to Run

Run this procedure whenever:
- A device configuration file (e.g., `m5s-*.yaml`) is created or modified.
- A shared package in `board/`, `common/`, `features/`, or `gui/` is modified.
- You want to confirm that all substitutions and `!include` directives resolve cleanly without missing variables.

---

## Step 1: Static YAML & Substitution Validation

Run `esphome config` against the target device configuration file:

```powershell
esphome config <device-config.yaml>
```

### What to check:
1. **Exit Code**: Must exit with code `0`.
2. **Missing Substitutions**: If the output warns about unreplaced `${...}` variables, verify:
   - Does the root YAML provide the required substitution under `substitutions:`?
   - Does the included board or module have default values, or did a variable name change?
3. **Include Paths**: Ensure relative paths in `!include` and `!include_dir_named` are correct relative to the root workspace directory.
4. **Secret Placeholders**: ESPHome will check if all `!secret` references exist in `secrets.yaml`.

---

## Step 2: C++ Dry-Run Compilation (Optional / Comprehensive)

When modifying platform drivers, custom C++ lambdas, external components, or LVGL pages, perform a full compile check without flashing:

```powershell
esphome compile <device-config.yaml>
```

### What to check:
1. **Compilation Success**: Ensures PlatformIO dependencies, framework versions, and ESP-IDF/Arduino code generation succeed.
2. **Flash & RAM Sizing**: Note the RAM and Flash usage printed at the end of the build.
3. **Compilation Errors**:
   - If header files or C++ types conflict, check component lambdas.
   - If caching issues occur, run `esphome clean <device-config.yaml>` and re-run `esphome compile`.

