# `gui/` Migration — Session Tracker & Handover Log

> **How to use this file (for both User and AI Assistants)**:
>
> 1. **At the start of a new session**: Read [`README.md`](file:///x:/esphome/tools/gui_migration/README.md) and the **Session Status Table** + **Handover Log** below to see the exact state of the migration.
> 2. **During a session**: Work only on the active session's checklist items unless the user explicitly asks to continue to the next session.
> 3. **At the end of a session**: Check off completed tasks (`[x]`), update the **Session Status Table**, and fill in the session's **Completion Report** at the bottom of that session section.

---

## 📊 Session Status Table

| Session | Focus Area | Status | Completed Date | Key Notes / Blockers |
| :--- | :--- | :--- | :--- | :--- |
| **Pre-Work** | ESPHome 2026.9.1 upgrade, `host-sdl` + `snapshot` + `touchscreen`, initial architecture analysis | ✅ **Done** | 2026-10-17 | WSL & Windows upgraded to `2026.9.1`; `board/host_sdl.yaml` supports dual `sdl` + `snapshot` display and `sdl` touchscreen. |
| **Session 1** | Tooling (`-Size <WxH>`) & Baseline Screenshots | ✅ **Done** | 2026-10-18 | Added `-Size`, `-Modules`, `-Cc1101Mode` to `host-sdl.ps1`/`run.sh`; captured 35 baseline PNGs across 6 resolutions + 5 module pages. |
| **Session 2** | Chrome (`header`/`footer`/`base`) & Theme Foundation | ⏳ **Next Up** | — | Ready to start. |
| **Session 3** | Simple Pages (`temperature`, `status`) & M5Stack Core Module Pages Cleanup (`air_quality`, `cc1101`, `dual_kmeter`) | ⬜ Pending | — | Depends on Session 2. Module pages stay `320×240` (only `.sizing` anchors removed). |
| **Session 4** | Clock Page Migration (`page.yaml` & `page_narrow.yaml`) | ⬜ Pending | — | Depends on Session 2. |
| **Session 5** | NordPool Graph Spike & Migration (Option B + Option A comparison) | ⬜ Pending | — | Depends on Session 2. |
| **Session 6** | Weather Page Migration (`page.yaml` & `page_narrow.yaml`) | ⬜ Pending | — | Depends on Session 2. |
| **Session 7** | Control Page, Unified `gui/main.yaml` & `gui/<W>x<H>.yaml` Stubs | ⬜ Pending | — | Depends on Sessions 3–6. |

---

## Pre-Work — Host SDL Virtual Device & Planning (✅ Completed)

### Completed Tasks

- [x] Upgraded ESPHome to **`2026.9.1`** in WSL (`~/.venvs/esphome`) and Windows Python.
- [x] Updated default ESPHome version in [`tools/host_sdl/setup_wsl.sh`](file:///x:/esphome/tools/host_sdl/setup_wsl.sh).
- [x] Reverted [`gui/weather/weather_icons.yaml`](file:///x:/esphome/gui/weather/weather_icons.yaml) and [`gui/widgets/boot_screen/sensors.yaml`](file:///x:/esphome/gui/widgets/boot_screen/sensors.yaml) to `image: - platform: file`.
- [x] Configured [`board/host_sdl.yaml`](file:///x:/esphome/board/host_sdl.yaml) with dual displays (`display_snapshot` + `display_sdl`), `touchscreen: platform: sdl`, `take_snapshot` API service, and `Take Snapshot` button.
- [x] Updated [`tools/host_sdl/host-sdl.ps1`](file:///x:/esphome/tools/host_sdl/host-sdl.ps1) to use `127.0.0.1` instead of `localhost` for fast HTTP `/screenshot` and `/page/<n>` calls.
- [x] Confirmed all architectural decisions and inspected the 5 [`board/module/module_*.yaml`](file:///x:/esphome/board/module/) GUI files.

---

## Session 1 — Tooling (`-Size <WxH>`) & Baseline Snapshots (✅ Completed)

### Goal

Enhance `host-sdl` tooling so we can launch and screenshot any arbitrary screen resolution (`-Size 480x320`, `-Size 240x320`, etc.) and optionally include module pages (`cc1101`, `dual_kmeter`, `air_quality`) in `.host-sdl.yaml` for visual testing. Capture baseline ("before") PNG screenshots in `tools/gui_migration/baseline/`.

### Tasks

- [x] **1.1** Update [`tools/host_sdl/run.sh`](file:///x:/esphome/tools/host_sdl/run.sh) and [`tools/host_sdl/host-sdl.ps1`](file:///x:/esphome/tools/host_sdl/host-sdl.ps1) to accept `-Size <WxH>` (e.g. `320x240`, `240x320`, `480x320`, `128x128`, `240x240`) via `-s screen_width <W> -s screen_height <H>` CLI substitutions, while still supporting existing layout names.
- [x] **1.2** Add `-Modules` and `-Cc1101Mode {packet|async}` flags to `host-sdl.ps1` (`--modules` / `--cc1101-mode` in `run.sh`) backed by [`board/module/host_simulated_modules.yaml`](file:///x:/esphome/board/module/host_simulated_modules.yaml), [`board/module/host_simulated_modules_async.yaml`](file:///x:/esphome/board/module/host_simulated_modules_async.yaml), and [`board/module/host_simulated_modules_none.yaml`](file:///x:/esphome/board/module/host_simulated_modules_none.yaml) to preview the 5 module pages (`air_quality`, `dual_kmeter`, `cc1101` wmbus/packet/async) with realistic mock sensor states on `host-sdl`. Also added fallback seed data in [`board/module/host_simulated_env.yaml`](file:///x:/esphome/board/module/host_simulated_env.yaml) so NordPool, Weather, and Room Temp pages always render populated data even when offline.
- [x] **1.3** Capture baseline screenshots of existing pages (`clock`, `temperature`, `status`, `nordpool`, `weather`, plus all 5 module pages) across representative layouts (`240x320`, `320x240`, `480x320`, `128x128`, `240r`, `240x135`) and save them under [`tools/gui_migration/baseline/`](file:///x:/esphome/tools/gui_migration/baseline/) via [`tools/gui_migration/capture_baseline.sh`](file:///x:/esphome/tools/gui_migration/capture_baseline.sh).
- [x] **1.4** Record completion report below and update the Session Status Table.

### Verification

```powershell
pwsh tools/host_sdl/host-sdl.ps1 config 240x320 -Modules
pwsh tools/host_sdl/host-sdl.ps1 screenshot
```

### Session 1 Completion Report

- **Date**: 2026-10-18
- **Summary of Changes**:
  - Updated [`tools/host_sdl/run.sh`](file:///x:/esphome/tools/host_sdl/run.sh) and [`tools/host_sdl/host-sdl.ps1`](file:///x:/esphome/tools/host_sdl/host-sdl.ps1) with `-Size <WxH>`, `-Modules`, and `-Cc1101Mode <packet|async>`.
  - Updated [`.host-sdl.yaml`](file:///x:/esphome/.host-sdl.yaml) to include `sim_modules: !include board/module/${sim_modules_pkg}.yaml` after `lvgl:` so module pages append cleanly at page indices `6..9` without shifting the 5 core pages (`1..5`).
  - Created [`board/module/host_simulated_modules.yaml`](file:///x:/esphome/board/module/host_simulated_modules.yaml), [`board/module/host_simulated_modules_async.yaml`](file:///x:/esphome/board/module/host_simulated_modules_async.yaml), and [`board/module/host_simulated_modules_none.yaml`](file:///x:/esphome/board/module/host_simulated_modules_none.yaml), plus fallback HA sensor seeding in [`board/module/host_simulated_env.yaml`](file:///x:/esphome/board/module/host_simulated_env.yaml).
  - Created [`tools/gui_migration/capture_baseline.sh`](file:///x:/esphome/tools/gui_migration/capture_baseline.sh) for automated batch screenshot capture across resolutions.
- **Baseline Screenshots Saved**:
  - [`tools/gui_migration/baseline/240x320/`](file:///x:/esphome/tools/gui_migration/baseline/240x320/) (`01_clock.png` .. `10_cc1101_async.png` — all 5 standard pages + all 5 module pages)
  - [`tools/gui_migration/baseline/320x240/`](file:///x:/esphome/tools/gui_migration/baseline/320x240/) (`01_clock.png` .. `05_status.png`)
  - [`tools/gui_migration/baseline/480x320/`](file:///x:/esphome/tools/gui_migration/baseline/480x320/) (`01_clock.png` .. `05_status.png`)
  - [`tools/gui_migration/baseline/128x128/`](file:///x:/esphome/tools/gui_migration/baseline/128x128/) (`01_clock.png` .. `05_status.png`)
  - [`tools/gui_migration/baseline/240r/`](file:///x:/esphome/tools/gui_migration/baseline/240r/) (`01_clock.png` .. `05_status.png`)
  - [`tools/gui_migration/baseline/240x135/`](file:///x:/esphome/tools/gui_migration/baseline/240x135/) (`01_clock.png` .. `05_status.png`)
- **Notes for Session 2**:
  - ESPHome 2026.9.1 YAML loader does not support `${...}` substitution inside a merge-key `!include` filename (`<<: !include { file: .../${var}.yaml }`), whereas standard package includes (`pkg: !include .../${var}.yaml`) work as expected. Keep this in mind when designing `gui/chrome/` and `gui/main.yaml` in Session 2.

---

## Session 2 — Chrome (`header`/`footer`/`base`) & Theme Foundation

### Goal

Build the resolution-independent foundation in `gui/chrome/` and `gui/themes/` without breaking any existing `gui/<W>x<H>.yaml` files.

### Tasks

- [ ] **2.1** Create `gui/chrome/base.yaml`:
  - Define default substitutions (`chrome_header: "true"`, `chrome_footer: "true"`, `chrome_h: ${ [24, (device.screen.height|int // 8)] | max }`, `page_pad_top`, `page_pad_bottom`).
  - Define `script: - id: set_page_title` (with parameter `title: string`) as a safe no-op when no header is loaded.
- [ ] **2.2** Create `gui/chrome/header.yaml`:
  - Contribute the top status/title bar to `lvgl: top_layer: widgets:` sized to `height: ${chrome_h}`.
  - `!extend set_page_title` to update `lbl_page_title`.
  - Include header status sensors (`gui/widgets/header/sensors.yaml`).
- [ ] **2.3** Create `gui/chrome/footer.yaml`:
  - Contribute the bottom navigation bar to `lvgl: top_layer: widgets:` sized to `height: ${chrome_h}`.
- [ ] **2.4** Update `gui/themes/` & `gui/styles/`:
  - Define shared `page_style` (`pad_top: ${page_pad_top}`, `pad_bottom: ${page_pad_bottom}`, `bg_color`, `border_width: 0`, `radius: 0`) and `container_style` in the LVGL theme/style definitions.
  - Define 3 font size tiers (`small` for `<200px`, `medium` for `200..319px`, `large` for `>=320px`) or responsive font size substitutions.
- [ ] **2.5** Verify with `esphome config host-sdl.yaml` and a test render in `host-sdl`.

### Session 2 Completion Report

- **Date**: _TBD_
- **Summary of Changes**: _TBD_
- **Verification Results**: _TBD_
- **Notes for Session 3**: _TBD_

---

## Session 3 — Simple Pages & M5Stack Core Module Pages Cleanup

### Goal

Migrate `temperature` and `status` to `styles: page_style` and `set_page_title`, and clean up the 5 M5Stack Core (`320×240`) module pages (`air_quality`, `cc1101/async`, `cc1101/packet`, `cc1101/wmbus`, `dual_kmeter`) so they use `styles: page_style` + `set_page_title` (keeping their existing `320×240` layouts intact) and delete the duplicated `.sizing:` blocks from all 5 `board/module/module_*.yaml` files.

### Tasks

- [ ] **3.1** Migrate [`gui/pages/temperature/page.yaml`](file:///x:/esphome/gui/pages/temperature/page.yaml) and [`page_narrow.yaml`](file:///x:/esphome/gui/pages/temperature/page_narrow.yaml) (or unify if a single responsive layout covers both orientations).
- [ ] **3.2** Migrate [`gui/pages/status/page.yaml`](file:///x:/esphome/gui/pages/status/page.yaml) and [`page_narrow.yaml`](file:///x:/esphome/gui/pages/status/page_narrow.yaml).
- [ ] **3.3** Update [`gui/pages/air_quality/page.yaml`](file:///x:/esphome/gui/pages/air_quality/page.yaml) to use `styles: page_style` + `set_page_title` and remove the `.sizing:` block from [`board/module/module_air_quality.yaml`](file:///x:/esphome/board/module/module_air_quality.yaml).
- [ ] **3.4** Update [`gui/pages/cc1101/async.yaml`](file:///x:/esphome/gui/pages/cc1101/async.yaml), [`packet.yaml`](file:///x:/esphome/gui/pages/cc1101/packet.yaml), and [`wmbus.yaml`](file:///x:/esphome/gui/pages/cc1101/wmbus.yaml) to use `styles: page_style` + `set_page_title` and remove the `.sizing:` blocks from:
  - [`board/module/module_cc1101_async.yaml`](file:///x:/esphome/board/module/module_cc1101_async.yaml)
  - [`board/module/module_cc1101_packet.yaml`](file:///x:/esphome/board/module/module_cc1101_packet.yaml)
  - [`board/module/module_cc1101_wmbus.yaml`](file:///x:/esphome/board/module/module_cc1101_wmbus.yaml)
- [ ] **3.5** Update [`gui/pages/dual_kmeter/page.yaml`](file:///x:/esphome/gui/pages/dual_kmeter/page.yaml) to use `styles: page_style` + `set_page_title` (keeping its `320×240` water tank dimensions `110×166`, radius `38`, and label offsets as-is, and reading `${module.tank_temp_min}` / `${module.tank_temp_max}` directly), and remove the `.sizing:` block from [`board/module/module_dual_kmeter.yaml`](file:///x:/esphome/board/module/module_dual_kmeter.yaml).
- [ ] **3.6** Validate devices using these modules (`esphome config`) and verify in `host-sdl` at `240x320` ( plus `temperature` and `status` across `320x240`, `480x320`, `128x128`).

### Session 3 Completion Report

- **Date**: _TBD_
- **Summary of Changes**: _TBD_
- **Verification & Screenshots**: _TBD_
- **Notes for Session 4**: _TBD_

---

## Session 4 — Clock Page Migration

### Goal

Migrate [`gui/pages/clock/page.yaml`](file:///x:/esphome/gui/pages/clock/page.yaml) and [`gui/pages/clock/page_narrow.yaml`](file:///x:/esphome/gui/pages/clock/page_narrow.yaml) to `styles: page_style`, `set_page_title`, and dynamic meter sizing across resolutions.

### Tasks

- [ ] **4.1** Migrate [`gui/pages/clock/page.yaml`](file:///x:/esphome/gui/pages/clock/page.yaml) and [`gui/pages/clock/page_narrow.yaml`](file:///x:/esphome/gui/pages/clock/page_narrow.yaml):
  - Apply `styles: page_style` and `set_page_title`.
  - Compute analog clock `meter` diameter dynamically from `min(screen_width, usable_height)` via Jinja substitutions so no per-layout `clock_size` anchor is needed.
- [ ] **4.2** Verify with `esphome config` and capture `host-sdl` screenshots across landscape, portrait, and small/round sizes.

### Session 4 Completion Report

- **Date**: _TBD_
- **Summary of Changes**: _TBD_
- **Verification & Screenshots**: _TBD_
- **Notes for Session 5**: _TBD_

---

## Session 5 — NordPool Graph Spike & Migration (Option B + Option A Comparison)

### Goal

Migrate [`gui/pages/nordpool/`](file:///x:/esphome/gui/pages/nordpool/) to a `1x3` `[CONTENT, FR(1), CONTENT]` grid layout, implement **Option B** (runtime container measurement for the `line` widget), and build **Option A** (LVGL `chart` widget) alongside it so we can visually compare both in `host-sdl`.

### Tasks

- [ ] **5.1** Convert [`gui/pages/nordpool/page.yaml`](file:///x:/esphome/gui/pages/nordpool/page.yaml) (and `page_narrow.yaml`) to a `1x3` `GRID` (`grid_rows: [CONTENT, FR(1), CONTENT]`, `grid_columns: [FR(1)]`) using `styles: page_style` and `set_page_title`.
- [ ] **5.2** Implement **Option B** in [`gui/pages/nordpool/sensors.yaml`](file:///x:/esphome/gui/pages/nordpool/sensors.yaml):
  - Remove `graph_container_w` and `graph_container_h` parameters from `nordpool_initialize_graph_layout` and `nordpool_update_chart`.
  - Measure `nordpool_graph_area` at runtime via `lv_obj_update_layout()` + `lv_obj_get_content_width()` / `lv_obj_get_content_height()`.
  - Keep hardcoded `sensor.waveshare_nordpool_future_offset` entity ID as confirmed.
- [ ] **5.3** Build **Option A** prototype (LVGL `chart` widget inside `nordpool_graph_area`) and capture side-by-side `host-sdl` screenshots of **Option B vs Option A** at `320x240`, `240x320`, and `480x320`.
- [ ] **5.4** Present comparison screenshots to the user to pick the final NordPool graph rendering mode.

### Session 5 Completion Report

- **Date**: _TBD_
- **Option B vs Option A Comparison**: _TBD_
- **User Decision**: _TBD_
- **Notes for Session 6**: _TBD_

---

## Session 6 — Weather Page Migration

### Goal

Migrate [`gui/pages/weather/page.yaml`](file:///x:/esphome/gui/pages/weather/page.yaml) and [`gui/pages/weather/page_narrow.yaml`](file:///x:/esphome/gui/pages/weather/page_narrow.yaml) to responsive grid/flex sizing without hardcoded row/column pixel heights.

### Tasks

- [ ] **6.1** Replace `weather_row_1_height`, `weather_row_2_height`, `weather_col_1_width`, etc. in [`gui/pages/weather/page.yaml`](file:///x:/esphome/gui/pages/weather/page.yaml) and [`page_narrow.yaml`](file:///x:/esphome/gui/pages/weather/page_narrow.yaml) with percentage / `FR()` / `SIZE_CONTENT` layouts.
- [ ] **6.2** Select weather icon tier (`20px`, `40px`, `60px`) dynamically via Jinja based on screen dimensions instead of per-layout anchors.
- [ ] **6.3** Preserve hardcoded HA entity IDs in [`gui/weather/weather_today.yaml`](file:///x:/esphome/gui/weather/weather_today.yaml) and [`gui/weather/weather_forecast.yaml`](file:///x:/esphome/gui/weather/weather_forecast.yaml) (`sensor.waveshare_display_data`, `weather.local`).
- [ ] **6.4** Verify with `esphome config` and capture `host-sdl` screenshots at `320x240`, `240x320`, `480x320`, and `128x128`.

### Session 6 Completion Report

- **Date**: _TBD_
- **Summary of Changes**: _TBD_
- **Verification & Screenshots**: _TBD_
- **Notes for Session 7**: _TBD_

---

## Session 7 — Control Page, Unified `gui/main.yaml` & Compatibility Stubs

### Goal

Migrate [`gui/pages/control/page.yaml`](file:///x:/esphome/gui/pages/control/page.yaml), create the unified [`gui/main.yaml`](file:///x:/esphome/gui/main.yaml), convert all `gui/<W>x<H>.yaml` files into thin compatibility stubs (with `header-only` chrome on tiny/round screens), and validate all device configurations across the repository.

### Tasks

- [ ] **7.1** Migrate [`gui/pages/control/page.yaml`](file:///x:/esphome/gui/pages/control/page.yaml) to `styles: page_style`, `set_page_title`, and responsive grid/flex tiles.
- [ ] **7.2** Create [`gui/main.yaml`](file:///x:/esphome/gui/main.yaml) combining theme, fonts, chrome (`base` + conditional `header`/`footer`), overlays (`boot_screen`, `ota_screen`), and pages.
- [ ] **7.3** Convert the 11 resolution files (`gui/128x128.yaml`, `gui/135x240.yaml`, `gui/172x320.yaml`, `gui/240r.yaml`, `gui/240x135.yaml`, `gui/240x320.yaml`, `gui/320x240.yaml`, `gui/320x480.yaml`, `gui/480x320.yaml`, `gui/720x1280.yaml`, `gui/1280x720.yaml`) into thin compatibility stubs that include `gui/main.yaml` with appropriate defaults (e.g. `chrome_footer: "false"` on `128x128`, `240x135`, `135x240`, `240r`), leaving `gui/5x5.yaml` untouched.
- [ ] **7.4** Run `esphome config` across all physical root device YAMLs that include `gui/*.yaml` or `board/module/module_*.yaml` to verify zero regressions.
- [ ] **7.5** Capture final multi-resolution screenshot matrix in `host-sdl` and complete the project handover report.

### Session 7 Completion Report

- **Date**: _TBD_
- **Summary of Changes**: _TBD_
- **Devices Validated**: _TBD_
- **Final Notes**: _TBD_
