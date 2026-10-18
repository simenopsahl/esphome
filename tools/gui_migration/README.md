# `gui/` Migration Plan — Architecture, Decisions & Session Roadmap

> **Living project document.** Located in [`tools/gui_migration/`](file:///x:/esphome/tools/gui_migration/).
> Designed for multi-session pacing so any AI assistant or human can pick up immediately from the last completed session.
> Delete `tools/gui_migration/` once the migration is complete.

- **Upstream reference**: [agillis/esphome-modular-lvgl-buttons](https://github.com/agillis/esphome-modular-lvgl-buttons) (`ARCHITECTURE.md`, `CLAUDE.md`, `example_code/SDL-lvgl-display_modular_480px.yaml`, `example_code/advanced/SDL-solar-graph.yaml`, `pages/loading.yaml`, `ui/popup/popup_system.yaml`).
- **Session Tracker & Logs**: See [`SESSIONS.md`](file:///x:/esphome/tools/gui_migration/SESSIONS.md) for the step-by-step session checklists and completion reports.
- **Legend**: ✅ Verified on ESPHome 2026.9.1 · ⚠️ Needs verification during spike/session.

---

## 0. Confirmed Design Decisions

The following architectural decisions have been confirmed and govern all sessions:

1. **Chrome on Tiny / Round Displays (`128x128`, `240x135`, `240r`)**:
   - Use **header-only** mode (`chrome_header: "true"`, `chrome_footer: "false"`). Drop the bottom navigation bar on tiny/round screens to preserve vertical space while keeping the status/title bar at the top.
2. **NordPool Full-Screen Graph**:
   - **Primary (`Option B`)**: Keep the existing LVGL `line` widget for visual continuity, place it in a `1x3` `[CONTENT, FR(1), CONTENT]` grid, and measure the container's runtime dimensions in C++ via `lv_obj_update_layout()` + `lv_obj_get_content_width()` / `lv_obj_get_content_height()` instead of passing precalculated pixel heights.
   - **Comparison Spike (`Option A`)**: During Session 5, also build a side-by-side prototype using LVGL's native `chart` widget (`type: LINE`, `width: 100%`, `height: 100%`) and capture `host-sdl` screenshots of both Option A and Option B so we can compare appearance and choose the winner.
3. **Hardcoded Home Assistant Entity IDs**:
   - **Keep as-is.** The entity IDs in [`gui/pages/nordpool/sensors.yaml`](file:///x:/esphome/gui/pages/nordpool/sensors.yaml) (`sensor.waveshare_nordpool_future_offset`), [`gui/weather/weather_today.yaml`](file:///x:/esphome/gui/weather/weather_today.yaml), and [`gui/weather/weather_forecast.yaml`](file:///x:/esphome/gui/weather/weather_forecast.yaml) (`sensor.waveshare_display_data`, `weather.local`) are global to the Home Assistant installation and will never change per device.
4. **Resolution Layout Files (`gui/<W>x<H>.yaml`)**:
   - **Keep `gui/<W>x<H>.yaml` as thin compatibility stubs** pointing to the new unified [`gui/main.yaml`](file:///x:/esphome/gui/main.yaml) (with appropriate chrome/tier defaults). Existing root device YAMLs continue to work without requiring an all-at-once cutover.
5. **Hardware Module GUI Pages (`board/module/module_*.yaml`)**:
   - The 5 module packages (`air_quality`, `cc1101` async/packet/wmbus, `dual_kmeter`) are M5Stack Core add-ons (Fire, Core2, CoreS3, CoreS3-SE), which are exclusively `320×240` (`height: 240`, `width: 320`). They do **not** need dynamic multi-resolution layouts; we only need to hook them into `styles: page_style` + `set_page_title` so the duplicated `.sizing:` YAML anchor blocks in [`board/module/`](file:///x:/esphome/board/module/) can be removed while keeping their existing `320×240` layout/dimensions intact (see Section 3 below).

---

## 1. How the Two Designs Differ

| Area | Upstream (`esphome-modular-lvgl-buttons`) | This Repo Today | Target Architecture |
| :--- | :--- | :--- | :--- |
| **Resolution Handling** | One `screen_width`/`screen_height`; layouts use `NxM` grids, `grid_cell_*`, `%`, `FR()`. No pixel coordinates. | 11 near-identical `gui/<W>x<H>.yaml` files (~6.5 KB each) holding hand-computed pixel numbers in YAML anchors (`&page_styles`, `&nordpool_page_styles`, ...). 5 pages also have `page_narrow.yaml` twins. | Single `gui/main.yaml` + responsive grids/flex (`FR()`, `%`, `SIZE_CONTENT`) and Jinja-computed chrome padding. `gui/<W>x<H>.yaml` kept as thin compatibility stubs. |
| **Composition** | Root YAML `!include`s one package per tile/page with `vars:`. Packages `!extend` an existing page or append to `lvgl: pages:`. | The layout file owns a fixed page list and passes YAML anchors via `vars:` to each page include. | Core pages included by `gui/main.yaml` (or modular page packages); hardware modules append their own pages via `lvgl: pages:` without `.sizing:` blocks. |
| **Chrome (Header/Footer)** | **None.** Pages own 100% of the screen; `top_layer` used only for overlays (loading, popups). | `top_layer` header + footer declared inline in each layout file; every page manually sets `pad_top/pad_bottom: 32`. | Modular `gui/chrome/` packages (`base.yaml`, `header.yaml`, `footer.yaml`) with `full` / `header_only` / `none` presets and a single shared `page_style`. |
| **Theme & Fonts** | `common/theme/index.yaml` bundle (colors, fonts, `page_style`). | `gui/themes/*` + `fonts/` + `styles/`, with per-layout font sizes. | Keep Night Owl (`c_*`) palette; consolidate `page_style` + `container_style` into theme; use 3 font size tiers (`small`, `medium`, `large`) selected by screen dimensions. |

---

## 2. Core Technical Challenges & Solutions

### 2.1 Header/Footer Chrome & Page Padding

Upstream has no equivalent of [`gui/widgets/header/widget.yaml`](file:///x:/esphome/gui/widgets/header/widget.yaml) or [`gui/widgets/footer/widget.yaml`](file:///x:/esphome/gui/widgets/footer/widget.yaml).

**Solution — Optional Chrome Packages + Single `page_style`**:

```text
gui/
  chrome/
    base.yaml        # Chrome height/padding Jinja math + no-op set_page_title script (always included)
    header.yaml      # lvgl: top_layer: widgets: [titlebar] + extends set_page_title
    footer.yaml      # lvgl: top_layer: widgets: [navbar]
```

1. **Page padding must be in pixels** (✅ verified: ESPHome's `lvalid.padding` rejects `%` and `SIZE_CONTENT`). Compute padding once in `gui/chrome/base.yaml` using Jinja (`|int` required because substitution values are `EStr`):

   ```yaml
   substitutions:
     chrome_h: ${ [24, (device.screen.height|int // 8)] | max }
     page_pad_top: ${ chrome_h if chrome_header == 'true' else 0 }
     page_pad_bottom: ${ chrome_h if chrome_footer == 'true' else 0 }
   ```

   Every page applies `styles: page_style` from the theme, which sets `pad_top: ${page_pad_top}` and `pad_bottom: ${page_pad_bottom}`.
2. **Decoupling `lbl_page_title`**: Pages call `script.execute: { id: set_page_title, title: !lambda 'return "${page_title}";' }` in `on_load`. `chrome/base.yaml` defines `set_page_title` as a no-op; `chrome/header.yaml` extends `set_page_title` to update `lbl_page_title`. Removing the header never breaks compilation.
3. **Tiny/Round Displays (`128x128`, `240x135`, `240r`)**: Set `chrome_header: "true"` and `chrome_footer: "false"` (header-only).

---

### 2.2 Full-Screen NordPool Graph (`gui/pages/nordpool/`)

Currently, NordPool relies on hand-computed pixel variables (`nordpool_row_2_height: 130`, `usable_screen_width: 312`) passed from `.sizing` anchors into `nordpool_initialize_graph_layout` and `nordpool_update_chart`.

**Solution**:

1. **Grid Layout (`[CONTENT, FR(1), CONTENT]`)**:
   Make `page_nordpool` a 1-column, 3-row `GRID` where row `0` (price labels) and row `2` (time labels) are `CONTENT`, and row `1` (`nordpool_graph_area`) is `FR(1)` with `STRETCH` alignment.
2. **Option B (Primary — Runtime Measurement of `line` Widget)**:
   In `nordpool_update_chart` (called on page `on_load` and sensor update), query LVGL for the resolved container dimensions rather than passing static pixel counts:

   ```cpp
   lv_obj_update_layout(id(nordpool_graph_area));
   int w = lv_obj_get_content_width(id(nordpool_graph_area));
   int h = lv_obj_get_content_height(id(nordpool_graph_area));
   ```

3. **Option A (Spike Comparison — LVGL `chart` Widget)**:
   Place a native LVGL `chart` (`type: LINE`, `width: 100%`, `height: 100%`) inside `nordpool_graph_area` and push normalized price points. We will build both in Session 5 and compare screenshots across multiple resolutions.

---

## 3. Hardware Module GUI Packages (`board/module/module_*.yaml`)

Five hardware module packages in [`board/module/`](file:///x:/esphome/board/module/) append pages to `lvgl: pages:`. Because these modules are exclusively for M5Stack Core (`320×240`, i.e., `height: 240`, `width: 320`) add-ons (Fire, Core2, CoreS3, CoreS3-SE), **their internal layouts do not need to be reworked for other resolutions**. We only need to replace their `.sizing:` YAML anchors with `styles: page_style` + `set_page_title`:

| Module Package | GUI Page File | Migration Action (320×240 M5Stack Core Only) |
| :--- | :--- | :--- |
| [`board/module/module_cc1101_async.yaml`](file:///x:/esphome/board/module/module_cc1101_async.yaml) | [`gui/pages/cc1101/async.yaml`](file:///x:/esphome/gui/pages/cc1101/async.yaml) | Use `styles: page_style` and `set_page_title`; delete the duplicated `.sizing:` block from `module_cc1101_async.yaml`. |
| [`board/module/module_cc1101_packet.yaml`](file:///x:/esphome/board/module/module_cc1101_packet.yaml) | [`gui/pages/cc1101/packet.yaml`](file:///x:/esphome/gui/pages/cc1101/packet.yaml) | Use `styles: page_style` and `set_page_title`; delete `.sizing:` from `module_cc1101_packet.yaml`. |
| [`board/module/module_cc1101_wmbus.yaml`](file:///x:/esphome/board/module/module_cc1101_wmbus.yaml) | [`gui/pages/cc1101/wmbus.yaml`](file:///x:/esphome/gui/pages/cc1101/wmbus.yaml) | Use `styles: page_style` and `set_page_title`; delete `.sizing:` from `module_cc1101_wmbus.yaml`. |
| [`board/module/module_air_quality.yaml`](file:///x:/esphome/board/module/module_air_quality.yaml) | [`gui/pages/air_quality/page.yaml`](file:///x:/esphome/gui/pages/air_quality/page.yaml) | Use `styles: page_style` and `set_page_title`; delete `.sizing:` from `module_air_quality.yaml`. |
| [`board/module/module_dual_kmeter.yaml`](file:///x:/esphome/board/module/module_dual_kmeter.yaml) | [`gui/pages/dual_kmeter/page.yaml`](file:///x:/esphome/gui/pages/dual_kmeter/page.yaml) | Use `styles: page_style` and `set_page_title`; keep existing `tank_width: 110`, `tank_height: 166`, `tank_radius: 38` and label offsets in `dual_kmeter/page.yaml` (using `${module.tank_temp_min}` / `${module.tank_temp_max}` directly); delete `.sizing:` from `module_dual_kmeter.yaml`. |

---

## 4. Multi-Session Roadmap Overview

To avoid hitting 5-hour or weekly token limits, the migration is divided into **7 self-contained sessions**. Each session has a small, well-defined scope, verification commands, and visual screenshot checks.

| Session | Title | Key Deliverables | Scope / Complexity |
| :--- | :--- | :--- | :--- |
| **Session 1** | **Tooling & Baseline Snapshots** | Add `-Size <WxH>` (and optional module page previewing) to `host-sdl.ps1` / `run.sh`; capture baseline screenshots of all current pages across target resolutions (`320x240`, `240x320`, `480x320`, `128x128`, `240r`). | ✅ Done |
| **Session 2** | **Chrome & Theme Foundation** | Create `gui/chrome/{base,header,footer}.yaml` (`full` / `header_only` / `none` modes + `set_page_title` script); add `page_style` & `container_style` to `gui/themes/`; define font size tiers. | Medium (Additive foundation) |
| **Session 3** | **Simple Pages & Module Pages Cleanup** | Migrate `temperature` and `status` to `page_style` + `set_page_title`; switch all 5 M5Stack Core module pages (`air_quality`, `cc1101` async/packet/wmbus, `dual_kmeter`) to `page_style` + `set_page_title` (keeping their 320×240 layout as-is) and delete `.sizing:` blocks from `board/module/module_*.yaml`. | Low–Medium |
| **Session 4** | **Clock Page Migration** | Migrate `clock/page.yaml` and `clock/page_narrow.yaml` to `page_style` + `set_page_title` and dynamic meter sizing across resolutions. | Low–Medium |
| **Session 5** | **NordPool Graph Spike & Migration** | Migrate `nordpool` to `1x3` `[CONTENT, FR(1), CONTENT]` grid; implement **Option B** (runtime `lv_obj_get_content_width/height`) AND prototype **Option A** (LVGL `chart` widget); capture comparison screenshots for review. | Medium–High (1 complex page + C++ lambda spike) |
| **Session 6** | **Weather Page Migration** | Migrate `weather/page.yaml` and `weather/page_narrow.yaml` to responsive grid/flex without hardcoded row/column pixel heights; dynamic icon size selection; preserve HA entity IDs. | Medium (1 multi-section page) |
| **Session 7** | **Control Page, Unified `gui/main.yaml` & Stubs** | Migrate `control/page.yaml`; create `gui/main.yaml`; convert all `gui/<W>x<H>.yaml` (except `5x5.yaml`) into thin compatibility stubs; validate all physical device YAMLs with `esphome config`. | Medium (Final cutover & repo-wide validation) |

👉 **Open [`SESSIONS.md`](file:///x:/esphome/tools/gui_migration/SESSIONS.md) to view detailed tasks for each session and check what was completed in previous sessions.**

---

## 5. Post-Migration Backlog (Future Phase)

Once the core `gui/` migration is complete (Sessions 1–7), the following enhancements are planned next:

1. **Climate Page (`gui/pages/climate/` or modular climate widget/detail)**: Add a responsive LVGL climate control/status page aligned with `esphome-modular-lvgl-buttons`.
2. **Monochrome B/W Theme (`gui/themes/colors_bw.yaml` + e-Ink styling)**: High-contrast black/white theme with zero gradients/shadows so e-Ink displays can migrate from C++ display lambdas to LVGL.
3. **6-Color Spectra Theme (`gui/themes/colors_spectra6.yaml`)**: Dedicated 6-color palette (Black, White, Red, Yellow, Blue, Green) for Spectra 6 e-Paper displays so they can also move from custom display lambdas to LVGL.
