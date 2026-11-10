# `epaper` Migration Plan — Architecture, Council Verdict & Session Roadmap

> **Living project document.** Located in [`tools/gui_migration/EPAPER.md`](file:///x:/esphome/tools/gui_migration/EPAPER.md).
> Focused on migrating the e-Paper GUI components from imperative C++ `display: pages: lambda:` configurations to declarative LVGL (`lvgl:`).
> Designed for multi-session pacing so any AI assistant or human can pick up immediately from the last completed session.

- **LLM Council Deliverables**:
  - Visual Briefing: [`council-report-epaper.html`](file:///x:/esphome/tools/gui_migration/council-report-epaper.html)
  - Full Transcript: [`council-transcript-epaper.md`](file:///x:/esphome/tools/gui_migration/council-transcript-epaper.md)
- **Session Tracker & Logs**: See [`SESSIONS.md`](file:///x:/esphome/tools/gui_migration/SESSIONS.md) (Phase 2: Sessions E1–E6) for the step-by-step session checklists and completion reports.
- **Legend**: ✅ Verified on ESPHome 2026.9.1 · ⚠️ Needs verification during spike/session.

---

## 0. Complete Inventory of E-Paper Devices & Replacement Plan (14 Total)

All 14 devices currently implement [`features/epaper.yaml`](file:///x:/esphome/features/epaper.yaml) through their board or display module configuration.

| # | Device YAML | MCU & PSRAM | Resolution & Aspect | Color Mode | Current Driver | Pages Implemented | Status & Migration Role |
| :- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | [`elecrow-crowpanel-42-4dbf00.yaml`](file:///x:/esphome/elecrow-crowpanel-42-4dbf00.yaml) | ESP32-S3 (**8MB PSRAM**) | `400×300` | B/W (1-bit) | `crowpanel_epaper` (`4.20in-v1.2`) | `page_info` (+ right-edge nav icons) | 🎯 **Candidate to replace `waveshare-weather-station`** |
| 2 | [`elecrow-crowpanel-579-b89ddc.yaml`](file:///x:/esphome/elecrow-crowpanel-579-b89ddc.yaml) | ESP32-S3 (**8MB PSRAM**) | `792×272` | B/W (1-bit) | `crowpanel_epaper` (`5.79in`) | `page_info` | ⚪ Storage (Migrate to LVGL `page_info` + sections) |
| 3 | [`m5s-air-quality-b62db0.yaml`](file:///x:/esphome/m5s-air-quality-b62db0.yaml) | ESP32-S3 | `200×200` (rot 180°) | B/W (1-bit) | `waveshare_epaper` (`1.54inv2`) | `page_local`, `page_info` (AQ table), `page_room` (+`graph:`) | 🟢 **Active** (Migrate to LVGL) |
| 4 | [`m5s-coreink-9fc390.yaml`](file:///x:/esphome/m5s-coreink-9fc390.yaml) | ESP32-PICO-D4 (**No PSRAM**) | `200×200` (rot 90°) | B/W (1-bit) | `waveshare_epaper` (`1.54inv2`) | `page_local`, `page_info`, `page_room`, `page_out` | 🟢 **Active** (Remain in use; migrate if `2df58c` spike succeeds, else bypass) |
| 5 | [`m5s-coreink-9fc2c8.yaml`](file:///x:/esphome/m5s-coreink-9fc2c8.yaml) | ESP32-PICO-D4 (**No PSRAM**) | `200×200` (rot 90°) | B/W (1-bit) | `waveshare_epaper` (`1.54inv2`) | `page_local`, `page_info`, `page_room`, `page_out` | 🟢 **Active** (Remain in use; migrate if `2df58c` spike succeeds, else bypass) |
| 6 | [`m5s-coreink-2df58c.yaml`](file:///x:/esphome/m5s-coreink-2df58c.yaml) | ESP32-PICO-D4 (**No PSRAM**) | `200×200` (rot 90°) | B/W (1-bit) | `waveshare_epaper` (`1.54inv2`) | `page_local`, `page_info`, `page_room`, `page_out` | 🧪 **Spare Testbed** for `200×200` LVGL heap/flush spike |
| 7 | [`seeed-trmnl-e1001-85a77d0.yaml`](file:///x:/esphome/seeed-trmnl-e1001-85a77d0.yaml) | ESP32-S3 (**8MB PSRAM**) | `800×480` | B/W (4-gray capable) | `epaper_spi` (`Seeed-reTerminal-E1001`) | `page_info`, `page_image` | 🎯 **Candidate to replace `waveshare-weather-station`** |
| 8 | [`seeed-trmnl-e1002-ec2478.yaml`](file:///x:/esphome/seeed-trmnl-e1002-ec2478.yaml) | ESP32-S3 (**8MB PSRAM**) | `800×480` | **Spectra™ 6-Color** (K/W/R/G/B/Y) | `epaper_spi` (`Seeed-reTerminal-E1002`) | `page_info`, `page_image`, `page_content` (Daily Info) | 🎯 **Candidate to replace `waveshare-display-board`** |
| 9 | [`seeed-trmnl-e1003-190420.yaml`](file:///x:/esphome/seeed-trmnl-e1003-190420.yaml) | ESP32-S3 (**8MB PSRAM**) | `1872×1404` | **16-Grayscale** (+ GT911 Touch) | `it8951` (`Seeed-reTerminal-E1003`) | `page_info`, `page_image`, `page_content` (Daily Info) | 🎯 **Candidate to replace `waveshare-display-board`** |
| 10 | [`seeed-trmnl-diy-b08e18.yaml`](file:///x:/esphome/seeed-trmnl-diy-b08e18.yaml) | ESP32-S3 (**8MB PSRAM**) | TBD | B/W | TBD | Initial testing incomplete | ⚪ Storage (Defer until hardware setup finishes) |
| 11 | [`waveshare-weather-station.yaml`](file:///x:/esphome/waveshare-weather-station.yaml) | ESP32-WROOM-32 (**No PSRAM**) | `400×300` &rarr; `300×400` (rot 270°) | B/W (1-bit) | `waveshare_epaper` (`4.20in`) | `page_content` (Weather + 3× `graph:` Ute/Foran/Bak) | 🔄 **Active &rarr; To be retired** (Replaced by `e1001`, `zectrix-note4c`, or `crowpanel-42`; capture baseline screenshot for feature parity) |
| 12 | [`waveshare-display-board.yaml`](file:///x:/esphome/waveshare-display-board.yaml) | ESP32-WROOM-32 (**No PSRAM**) | `800×480` &rarr; `480×800` (rot 90°) | B/W (1-bit) | `waveshare_epaper` (`7.50in-bV2`) | `page_content` (Daily Info: Weather, FolloRen, NordPool, Bus, Car) | 🔄 **Active &rarr; To be retired** (Replaced by `e1002` or `e1003`; capture baseline screenshot for feature parity) |
| 13 | [`waveshare-simen.yaml`](file:///x:/esphome/waveshare-simen.yaml) | ESP32-WROOM-32 (**No PSRAM**) | `400×300` &rarr; `300×400` (rot 270°) | B/W (1-bit) | `waveshare_epaper` (`4.20in`) | `page_content` (Weather + 3× `graph:`) | 🛑 **Retire / Reference only** |
| 14 | [`zectrix-note4c.yaml`](file:///x:/esphome/zectrix-note4c.yaml) | ESP32-S3 (**8MB PSRAM**) | `400×300` &rarr; `300×400` (rot 270°) | **4-Color B/W/R/Y** | `epaper_spi` (`jd79660`) | `page_info` | 🎯 **Candidate to replace `waveshare-weather-station`** |

---

## 1. Goals

1. **Migrate e-Paper display lambdas to declarative LVGL (`lvgl:`)** so layouts use responsive flex/grid containers instead of fragile hardcoded `(x, y)` coordinates.
2. **Replace legacy non-PSRAM WROOM-32 devices with modern ESP32-S3 + PSRAM hardware**:
   - Promote **`seeed-trmnl-e1002`** (Spectra 6-color) or **`seeed-trmnl-e1003`** (10.3" 1872×1404 grayscale) to replace `waveshare-display-board` ("Daily Info" dashboard).
   - Promote **`seeed-trmnl-e1001`**, **`zectrix-note4c`** (4-color B/W/R/Y), or **`elecrow-crowpanel-42`** to replace `waveshare-weather-station` (Weather + 3× Temperature Graphs).
   - Keep **`m5s-coreink`** units in active use: test LVGL on spare `m5s-coreink-2df58c` (`200×200` is only `5 KB` 1-bit buffer + `15 KB` LVGL strip buffer), or bypass them if memory proves too tight.
3. **Eliminate the `page_black` / `page_white` navigation bug**: Move the screen repair/scrubbing screens out of navigable `pages:` into an `lvgl: top_layer:` overlay that is only shown when `repair_script` runs.
4. **Verify feature parity and visual quality via `host-sdl`**: Capture reference screenshots of the original C++ display lambdas (`waveshare-display-board`, `waveshare-weather-station`, `m5s-air-quality`, `m5s-coreink`, `e1002`) before building their LVGL replacements.

---

## 2. Core Technical Challenges & Council Solutions

### 2.1 Different Display Components (`waveshare_epaper` vs. `epaper_spi` vs. `crowpanel_epaper` vs. `it8951`)
- **Original Concern**: Some devices use legacy `waveshare_epaper`, which precludes using `epaper_spi.is_updating` as a condition before `component.update: display_eink` in `script.update_screen`.
- **Council Solution**:
  1. Retiring `waveshare-display-board`, `waveshare-weather-station`, and `waveshare-simen` already eliminates three of the legacy `waveshare_epaper` boards!
  2. For the remaining devices (`epaper_spi`, `it8951`, `crowpanel_epaper`, and `m5s-air-quality` / `m5s-coreink`), **do not mix display driver rewrites with the LVGL migration**. LVGL sits cleanly on top of `display_eink` regardless of the driver.
  3. Setting `mode: single` on `script.update_screen` (and `script.wait: update_screen` inside `repair_script`) prevents overlapping screen refreshes in software across all display drivers without needing `epaper_spi.is_updating`.

---

### 2.2 LVGL Asynchronous Redraw vs. `component.update: display_eink` Race Condition ⚠️
- **Council Discovery**: In C++ `display:` lambdas, `component.update: display_eink` immediately executes the lambda and pushes pixels over SPI. In LVGL, calling `lvgl.label.update` or `lvgl.style.update` only marks widgets as *dirty*; LVGL does not render pixels into `display_eink`'s buffer until its next `lv_timer_handler()` tick. Calling `component.update: display_eink` in the same script tick flushes the **old** buffer (one cycle behind)!
- **Solution**: In `gui/epaper/base.yaml`, `script.update_screen` must trigger `lvgl.widget.redraw`, wait `100ms` so LVGL's timer flushes all dirty strips to `display_eink`, and *then* call `component.update: display_eink`:
  ```yaml
  script:
    - id: update_screen
      mode: single
      then:
        - lambda: 'id(data_updated) = false;'
        - lvgl.widget.redraw:
        - delay: 100ms   # Allow LVGL timer to flush dirty strips into display_eink buffer
        - component.update: display_eink
        - lambda: 'id(recorded_display_refresh) += 1;'
        - lambda: 'id(display_last_update).publish_state(id(time_homeassistant).now().timestamp);'
  ```

---

### 2.3 Narrow Use-Cases, Niche Layouts & Physical Button Nav Hints (`// NAV`)
- **The Challenge**: E-paper devices have bespoke functions and physical bezel buttons (`stick_up/press/down` on CoreInk, `KEY0/1/2` on reTerminal E1001/1002/1003, `button_ok/up/down` on Zectrix, right-side buttons on CrowPanel).
- **Council Solution — Modular "Bento Sections" + Device Chrome**:
  - Instead of a monolithic `gui/main.yaml` that forces identical full-screen pages onto every device, `gui/epaper/` provides:
    1. **Device-specific nav-hint chrome packages** (`gui/epaper/chrome/nav_coreink.yaml`, `nav_reterminal.yaml`, `nav_crowpanel.yaml`, `nav_zectrix.yaml`) that align icons with physical bezel buttons, plus a shared `status_bar.yaml` (`Wi-Fi`, `HA`, `@ HH:MM` timestamp).
    2. **Standard `page_info` (`gui/epaper/pages/info.yaml`)**: Every e-Paper device gets a clean, responsive Device Info page (Name, SSID, IP, MAC, Battery, Button hints).
    3. **Composable Bento-Grid Section Packages (`gui/epaper/sections/`)**:
       - The "Daily Info" page (`waveshare-display-board` $\rightarrow$ `e1002` / `e1003`) and the "Weather Station" page (`waveshare-weather-station` $\rightarrow$ `e1001` / `zectrix` / `crowpanel-42`) are built as reusable LVGL flex/grid containers (`weather_forecast.yaml`, `folloren.yaml`, `nordpool_graph.yaml`, `bus_departures.yaml`, `car_status.yaml`, `rolling_graph.yaml`).
       - This lets you test the Weather Station layout on *all three* replacement candidates (`e1001`, `zectrix-note4c`, `crowpanel-42`) and the Daily Info layout on *both* replacement candidates (`e1002`, `e1003`) simply by including the section packages!

---

### 2.4 LVGL Themes, 1-Bit Fonts & Every-5th-Refresh Color Inversion
- **Color Palettes Needed**:
  1. `gui/epaper/themes/bw.yaml` — Monochrome Black/White (`e1001`, `crowpanel-42/579`, `m5s-air-quality`, `m5s-coreink`)
  2. `gui/epaper/themes/bwry4.yaml` — 4-Color Black/White/Red/Yellow (`zectrix-note4c`)
  3. `gui/epaper/themes/spectra6.yaml` — 6-Color Black/White/Red/Green/Blue/Yellow (`seeed-trmnl-e1002`)
  4. `gui/epaper/themes/gray16.yaml` — 16-Level Grayscale (`seeed-trmnl-e1003`)
- **Crisp 1-Bit Fonts (`bpp: 1`) & Unified MDI Font Glyphs (Replacing `it.image` `mdi:*` Icons)**:
  - Standard LVGL `bpp: 4` anti-aliased fonts turn into fuzzy/jagged blobs when thresholded to 1-bit B/W and consume 4× the Flash/RAM. `gui/epaper/fonts/bw.yaml` will compile fonts with `bpp: 1` for razor-sharp monochrome text, while `color_gray.yaml` uses `bpp: 2` / `bpp: 4`.
  - Several devices (`elecrow-crowpanel-42`, `elecrow-crowpanel-579`, `m5s-coreink-*`, `m5s-air-quality`) currently declare an `image: - platform: file` block to rasterize `mdi:*` icons (`mdi:wifi`, `mdi:home-assistant`, `mdi:refresh`, `mdi:arrow-right-thick`, `mdi:arrow-left-thick`, `mdi:home`, `mdi:exit-to-app`, `mdi:information`, `mdi:thermometer`) and draw them with `it.image(...)`.
  - During migration, we will **remove all `mdi:*` `image:` blocks** and render all icons as LVGL `label` glyphs using the shared [`gui/fonts/glyphs.yaml`](file:///x:/esphome/gui/fonts/glyphs.yaml) list (which is purely a list of codepoints, independent of `bpp`).
  - Any missing navigation/action icons (`refresh` `\U000F0450`, `arrow-right-thick` `\U000F0055`, `arrow-left-thick` `\U000F004E`, `exit-to-app` `\U000F0206`) will be added directly to [`gui/fonts/glyphs.yaml`](file:///x:/esphome/gui/fonts/glyphs.yaml).
- **How LVGL Handles Every-5th-Refresh Color Inversion**:
  - In LVGL, widgets reference shared `style_definitions:` (`epaper_page_style`, `epaper_card_style`, `epaper_accent1_style`, `epaper_accent2_style`, `epaper_accent3_style`).
  - When `recorded_display_refresh % 5 == 0` triggers (if enabled via `epaper_invert_every_5th: "true"`), an action calls `lvgl.style.update` on `epaper_page_style` and `epaper_card_style` to swap `bg_color` and `text_color` (and rotate accent colors) right before `lvgl.widget.redraw`!

---

### 2.5 Replacing ESPHome `graph:` (`it.graph(...)`) in LVGL
- **NordPool Graph (`e1002`, `e1003`)**: Reuses the proven LVGL `line` widget + runtime container measurement (`lv_obj_get_content_width/height`) from [`gui/pages/nordpool/sensors.yaml`](file:///x:/esphome/gui/pages/nordpool/sensors.yaml).
- **Temperature History Graphs (`Ute`, `Foran`, `Bak` on Weather Station replacements; `Room` on `m5s-air-quality`)**: A single reusable `gui/epaper/sections/rolling_graph.yaml` package maintains a rolling point array from the sensor and renders an auto-scaled LVGL `line` widget alongside the sensor title and current temperature readout.

---

## 3. Target File Layout (`gui/epaper/`)

```text
features/
  └── epaper.yaml                 # Legacy C++ lambda support (kept for unmigrated/retired devices)
gui/
  └── epaper/
        ├── base.yaml             # Core LVGL e-Paper engine: lvgl config, update_screen script (with 100ms flush delay),
        │                         # top_layer black/white repair scrubber overlay, "Venter på data..." boot overlay
        ├── fonts/
        │     ├── bw.yaml         # Crisp 1-bpp (bpp: 1) Noto Sans + MDI fonts for 1-bit B/W panels
        │     └── color_gray.yaml # 2-bpp / 4-bpp fonts for 4-color, Spectra 6-color, and 16-grayscale panels
        ├── styles/
        │     └── main.yaml       # Master style_definitions (epaper_page_style, epaper_card_style, epaper_accent1..3_style)
        ├── themes/
        │     ├── bw.yaml         # 1-bit Black & White palette + optional 5th-refresh lvgl.style.update inversion
        │     ├── bwry4.yaml      # 4-color Black/White/Red/Yellow palette (Zectrix Note4C)
        │     ├── spectra6.yaml   # 6-color Black/White/Red/Green/Blue/Yellow palette (Seeed reTerminal E1002)
        │     └── gray16.yaml     # 16-level Grayscale palette (Seeed reTerminal E1003)
        ├── chrome/
        │     ├── status_bar.yaml     # Wi-Fi icon, HA API icon, and "@ HH:MM" refresh timestamp
        │     ├── nav_coreink.yaml    # Bezel button icon hints for M5Stack CoreInk
        │     ├── nav_reterminal.yaml # Bezel button icon hints for Seeed reTerminal E1001/E1002/E1003
        │     ├── nav_crowpanel.yaml  # Right-edge bezel button icon hints for Elecrow CrowPanel 4.2" / 5.79"
        │     └── nav_zectrix.yaml    # Bezel button icon hints for Zectrix Note4C
        ├── pages/
        │     ├── info.yaml           # Standard responsive e-Paper Device Info page (Name, Network, Battery, Buttons)
        │     ├── image.yaml          # Reusable random/cycled full-screen image page (for E1001/E1002/E1003)
        │     └── temp_single.yaml    # Large single-temperature page (for CoreInk local/room/out & Air Quality)
        └── sections/                 # Composable Bento-grid sections for Weather Station & Daily Info dashboards
              ├── weather_header.yaml   # 1-or-2 icon current/next weather condition header (Weather Station)
              ├── weather_forecast.yaml # Current weather + Målt/Meldt temps + 4/5-slot forecast row (Daily Info)
              ├── folloren.yaml         # FolloRen waste pickup section
              ├── nordpool_graph.yaml   # NordPool 9h LVGL line graph + price_0 / price_4 / price_8 labels
              ├── bus_departures.yaml   # Bregnefaret 510 Drøbak bus departure table
              ├── car_status.yaml       # EV battery % + range km section
              └── rolling_graph.yaml    # Reusable LVGL line graph + temperature readout for a single sensor (Ute/Foran/Bak)
```

---

## 4. Verifying Feature Parity & Functionality (`host-sdl`)

Each original device's dimensions, colors, and C++ `display: pages: lambda:` will be loaded in `host-sdl` during **Session E1** (with mock sensor data injected from `host_simulated_env.yaml`) to capture baseline PNG screenshots. Keeping both the original lambda reference and image output in `tools/gui_migration/epaper_baseline/<device>/` guarantees visual and functional parity when building the LVGL replacements on the new devices.

---

## 5. Multi-Session Roadmap (Sessions E1–E6)

Detailed checklists and handover logs are tracked in [`SESSIONS.md`](file:///x:/esphome/tools/gui_migration/SESSIONS.md):

| Session | Title | Target Devices & Deliverables | Scope / Complexity |
| :--- | :--- | :--- | :--- |
| **Session E1** | **`host-sdl` C++ Lambda Reference Snapshots** | Add C++ `display:` e-Paper preview harness to `host-sdl`; capture reference PNGs of `waveshare-display-board` (`480×800`), `waveshare-weather-station` (`300×400`), `seeed-trmnl-e1002` (`800×480` 6-color), `zectrix-note4c` (`300×400` 4-color), `elecrow-crowpanel-42` (`400×300`), `m5s-air-quality` (`200×200`), and `m5s-coreink` (`200×200`) in `tools/gui_migration/epaper_baseline/`. | Low–Medium (Tooling + Reference screenshots) |
| **Session E2** | **`gui/epaper/` Foundation, Themes, Overlays & `page_info`** | Build `gui/epaper/base.yaml` (`update_screen` with `100ms` flush delay, `top_layer` black/white repair scrubber + `"Venter på data..."` boot overlay), `fonts/{bw,color_gray}.yaml`, `styles/main.yaml`, `themes/{bw,bwry4,spectra6,gray16}.yaml`, `chrome/*`, and `pages/info.yaml`. Validate `page_info` in `host-sdl` across all 4 color modes. | Medium (Core toolkit) |
| **Session E3** | **Replacing `waveshare-display-board` $\rightarrow$ `e1002` (Spectra 6) & `e1003` (1872×1404)** | Build the 5 Bento sections (`weather_forecast`, `folloren`, `nordpool_graph`, `bus_departures`, `car_status`) + `pages/image.yaml`. Migrate `seeed-trmnl-e1002-ec2478.yaml` (`800×480` 6-color) and `seeed-trmnl-e1003-190420.yaml` (`1872×1404` 16-gray) to full LVGL Daily Info dashboards. Compare screenshots against `waveshare-display-board` & `e1002` baselines. | Medium–High (5 Bento sections + 2 flagship replacements) |
| **Session E4** | **Replacing `waveshare-weather-station` $\rightarrow$ `e1001`, `zectrix-note4c` & `crowpanel-42/579`** | Build `sections/weather_header.yaml` and `sections/rolling_graph.yaml` (`Ute`, `Foran`, `Bak` 12h LVGL line graphs). Migrate `seeed-trmnl-e1001-85a77d0.yaml`, `zectrix-note4c.yaml` (4-color), `elecrow-crowpanel-42-4dbf00.yaml`, and `elecrow-crowpanel-579-b89ddc.yaml` so you can compare and pick your favorite physical replacement! | Medium (Rolling graph widget + 4 PSRAM S3 devices) |
| **Session E5** | **200×200 Active Fleet (`m5s-air-quality` & `m5s-coreink` Spike)** | Build `pages/temp_single.yaml`. Migrate `m5s-air-quality-b62db0.yaml` (ESP32-S3, `200×200`) to LVGL (`page_local`, AQ table `page_info`, `page_room` with `rolling_graph`). Run LVGL memory spike on spare `m5s-coreink-2df58c.yaml` (`ESP32-PICO-D4`, `buffer_size: 10%`, `bpp: 1`); if stable, apply to `9fc390` & `9fc2c8` (otherwise leave CoreInks on C++ lambdas with the `is_repairing` button-skip fix). | Low–Medium (200×200 fleet) |
| **Session E6** | **Hardware Verification, Legacy Retirement & Documentation** | Flash and test chosen replacement devices in production (`e1002`/`e1003` and `e1001`/`zectrix`/`crowpanel-42`), archive retired `waveshare-*` configs, and update `gui/ARCHITECTURE.md` & `AGENTS.md`. | Low (Verification & cleanup) |
