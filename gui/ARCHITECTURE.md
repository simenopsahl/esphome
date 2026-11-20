# Modular LVGL GUI Architecture

This directory (`gui/`) implements two complementary, resolution-independent, package-based LVGL user interface systems for ESPHome displays:

1. **LCD / Touch LVGL Engine (`gui/main.yaml`)**: For backlit LCDs and touchscreens ranging from `128×128` to `480×800` / `800×480`.
2. **e-Paper LVGL Engine (`gui/epaper/base.yaml`)**: For monochrome, 4-color BWRY, 6-color Spectra 6, and 16-grayscale e-Paper displays ranging from `200×200` (CoreInk / Air Quality) up to `1872×1404` (reTerminal E1003).

For general ESPHome LVGL widget and layout syntax rules, see [`gui/LVGL_REFERENCE.md`](file:///x:/esphome/gui/LVGL_REFERENCE.md).

---

## 1. Directory Structure

```text
gui/
  ├── main.yaml               # Unified responsive LCD LVGL entry point (pages, theme, styles, chrome)
  ├── <W>x<H>.yaml            # Thin LCD resolution stubs (set screen_width/screen_height and include main.yaml)
  ├── 5x5.yaml                # Standalone config for 5×5 RGB LED matrix displays
  ├── common/                 # Global LCD top_layer chrome & system overlays
  │     ├── chrome/
  │     │     ├── base.yaml   # Chrome height/padding math & safe no-op `set_page_title` script
  │     │     ├── header.yaml # Top status bar (`titlebar`: Wi-Fi, page title/icon, clock, HA status)
  │     │     └── footer.yaml # Bottom touch navigation bar (`navbar`: Prev, Home, Next)
  │     ├── boot_screen/      # Boot splash overlay (`widget.yaml` + `sensors.yaml`)
  │     └── ota_screen/       # OTA progress bar overlay (`widget.yaml` + `sensors.yaml`)
  ├── epaper/                 # Modular LVGL engine for e-Paper panels (`lvgl_comp` + `display_eink`)
  │     ├── base.yaml         # Core e-Paper LVGL engine, on-demand refresh pipeline & overlays
  │     ├── _defaults.yaml    # Lowest-priority fallback substitutions (`display.*`, `epaper_nav_pad_*`)
  │     ├── chrome/           # Top-layer status bar & hardware bezel key hints
  │     │     ├── status_bar.yaml      # Header/footer bar (`@ HH:MM`, Wi-Fi/HA badges, page icon/title)
  │     │     ├── nav_coreink.yaml     # M5Stack CoreInk dial hints (rotation-aware: 0/90/180/270)
  │     │     ├── nav_crowpanel.yaml   # Elecrow CrowPanel 5-key bezel hints (rotation-aware)
  │     │     ├── nav_reterminal.yaml  # Seeed reTerminal 3-key top bezel hints (rotation-aware)
  │     │     ├── nav_zectrix.yaml     # Zectrix Note4C touch navigation bar
  │     │     └── nav_none.yaml        # No-op navigation stub (`epaper.nav: none`)
  │     ├── fonts/            # Responsive 5-tier e-Paper font sizing (`bw.yaml` 1-bit & `color_gray.yaml` 4-bit)
  │     │     ├── _fonts.yaml          # Shared font size math (`font_xs`..`font_fl`, `mdi_sm`..`mdi_xl`)
  │     │     ├── bw.yaml              # 1-bit sharp monochrome wrapper (`epaper_font_bpp: "1"`)
  │     │     └── color_gray.yaml      # 4-bit anti-aliased wrapper (`epaper_font_bpp: "4"`)
  │     ├── pages/            # Self-contained e-Paper page packages (`main.yaml` + `page.yaml` + `sensors.yaml`)
  │     │     ├── info/                # Default diagnostic info page (`page_info`, included by `base.yaml`)
  │     │     ├── temperature/         # Unified room/device temperature page (`local.yaml`, `remote.yaml`, `page.yaml`)
  │     │     ├── air_quality/         # M5Stack Air Quality multi-sensor grid (`page_air_quality`)
  │     │     ├── weather_station/     # 3-graph + forecast weather station (`page_weather_station`)
  │     │     ├── daily/               # Multi-column Daily Info Bento dashboard (`page_daily`)
  │     │     └── image/               # Full-screen photo/art frame (`page_image`)
  │     ├── sections/         # Reusable e-Paper layout sections & data bindings
  │     │     ├── rolling_graph/       # 12-hour 36-slot polyline graph (`section`, `_core`, `remote_series`, `local`, `remote`, `ha_attr`, `ha_series`)
  │     │     ├── weather_header/      # Current weather icon + condition + high/low summary
  │     │     ├── weather_forecast/    # Multi-slot weather forecast strip
  │     │     ├── nordpool_graph/      # 48-slot NordPool electricity price bar graph + stats
  │     │     ├── bus_departures/      # Real-time public transit departure rows
  │     │     ├── car_status/          # EV battery SoC, range, charging & lock status
  │     │     ├── folloren/            # Recycle bin collection schedule & status
  │     │     └── picture/             # Rotating 800×480 photo bento & shared photo assets (`> 960px` panels)
  │     ├── styles/
  │     │     └── main.yaml            # Shared e-Paper LVGL `style_definitions`
  │     └── themes/           # e-Paper color palettes & 5th-refresh anti-ghosting polarity inversion
  │           ├── _base.yaml           # Shared palette colors & `apply_epaper_theme_refresh` script
  │           ├── lvgl_theme.yaml      # Base LVGL widget theme for e-Paper
  │           ├── bw.yaml              # 1-bit Black & White theme (`invert_every_5th: "true"`)
  │           ├── bwry4.yaml           # 4-color Black/White/Red/Yellow theme (`invert_accents: "false"`)
  │           ├── spectra6.yaml        # 6-color Spectra 6 theme (`invert_every_5th: "false"`)
  │           └── gray16.yaml          # 16-level Grayscale theme (`invert_every_5th: "false"`)
  ├── fonts/
  │     ├── main.yaml         # 5-tier responsive LCD font sizing (`font_sm`..`font_fl`, `mdi_sm`..`mdi_xxl`)
  │     └── glyphs.yaml       # Material Design Icons (MDI) codepoints included across all `mdi_*` sizes
  ├── functions/              # Reusable LVGL behavioral packages & fragments (LCD & e-Paper)
  │     ├── control_auto_display.yaml  # Presence-based backlight dimming & auto-off
  │     ├── control_navigation.yaml    # HA page navigation buttons, `on_page_navigate` & `sensor_current_page`
  │     ├── swipe_navigation.yaml      # Left/right touch swipe page navigation fragment
  │     ├── anti_burn_in.yaml          # Pixel-shift antiburn helper
  │     ├── number_sensor.yaml         # Bind an arbitrary LVGL label to a Home Assistant numeric sensor
  │     └── text_sensor.yaml           # Bind an arbitrary LVGL label to a Home Assistant text sensor
  ├── img/                    # Image & SVG assets (boot logo, weather condition icons, `eink/` assets)
  ├── pages/                  # Full-screen LCD LVGL pages (`page.yaml` + optional `sensors.yaml`)
  │     ├── clock/            # Analog/digital clock page (standard)
  │     ├── temperature/      # Room temperature page (standard; includes `local.yaml` & `remote.yaml`)
  │     ├── nordpool/         # NordPool electricity price graph page (standard)
  │     ├── weather/          # Weather today + 4-slot forecast page (standard; includes `icons.yaml`)
  │     ├── status/           # Device diagnostic status page (standard)
  │     ├── grid_dashboard/   # Configurable NxM grid page (`page_dashboard`) for dashboard tiles (opt-in)
  │     ├── climate/          # Interactive thermostat arc page + HVAC/fan/swing popups (opt-in)
  │     ├── air_quality/      # CO2 / PM1 / PM2.5 / PM10 hardware module page (opt-in via `board/module/`)
  │     ├── cc1101/           # Sub-GHz radio / wMBus water meter pages (`async`, `packet`, `wmbus`)
  │     └── dual_kmeter/      # Dual thermocouple hot-water tank graphic page (`page_kmeter`)
  ├── styles/                 # Shared LCD LVGL `style_definitions:` (loaded via `!include_dir_merge_list styles`)
  │     ├── page_styles.yaml      # `- id: page_style` (reserves `page_pad_top` & `page_pad_bottom` for chrome)
  │     └── container_styles.yaml # `- id: container_style` (100%x100% transparent content area with padding)
  ├── themes/                 # LCD LVGL widget theme & color palettes
  │     ├── main.yaml             # Default styling for `button`, `obj` (card surface), `label`, `arc`, etc.
  │     └── colors_*.yaml         # Color palette definitions (`colors_night_owl.yaml`, `colors.yaml`, etc.)
  └── tiles/                  # Modular LCD dashboard tiles that `!extend` `page_dashboard`
        ├── binary_sensor/    # `local.yaml` & `remote.yaml` (read-only on/off indicator tile)
        ├── button/           # `local.yaml` & `remote.yaml` (stateless action/service trigger tile)
        ├── climate/          # `local.yaml` & `remote.yaml` (live setpoint/action tile -> opens `page_climate`)
        ├── clock/            # `flip_clock.yaml` & `time_button.yaml` (24h digital clock tiles -> opens `page_analog_clock`)
        ├── imu/              # `local.yaml` (responsive IMU bubble-level crosshair tile)
        ├── light/            # `local.yaml` & `remote.yaml` (toggle + brightness % tile)
        ├── sensor/           # `local.yaml` & `remote.yaml` (numeric sensor readout tile)
        ├── switch/           # `local.yaml` & `remote.yaml` (stateful toggle tile)
        └── text_sensor/      # `local.yaml` & `remote.yaml` (string state/attribute readout tile)
```

---

## 2. Responsive Layout Engine

All layouts scale automatically from two flat substitutions declared by the board or resolution stub (`gui/<W>x<H>.yaml`):

- `screen_width`: Horizontal pixel count (e.g. `"240"`, `"320"`, `"480"`, `"800"`)
- `screen_height`: Vertical pixel count (e.g. `"135"`, `"240"`, `"320"`, `"800"`)

### How Chrome & Content Area Work Together

1. **[`gui/common/chrome/base.yaml`](file:///x:/esphome/gui/common/chrome/base.yaml)** computes:
   - `chrome_h`: Height of the top header (`titlebar`) and bottom navigation bar (`navbar`), scaling from `24px` (`128×128`) up to `44px` (`800×480`).
   - `page_pad_top` / `page_pad_bottom`: Automatically set to `chrome_h` when `chrome_header` / `chrome_footer` is `"true"` (or `0` when `"false"`, such as `chrome_footer: "false"` on small/round screens $\le 240$ px).
   - `container_pad_top/bottom/left/right`: Inner margins around page content.
2. **[`gui/styles/page_styles.yaml`](file:///x:/esphome/gui/styles/page_styles.yaml)** applies `pad_top: ${page_pad_top}` and `pad_bottom: ${page_pad_bottom}` to `page_style`. Every full-screen page uses `styles: page_style`, guaranteeing its content region sits cleanly between the header and footer on `top_layer`.
3. **Narrow Screen Adaptation (`screen_width < 180`)**:
   - When `screen_width < 180` (e.g. `128×128` or `135×240` portrait), `header.yaml` switches `lbl_page_title` from the text title to the page's MDI icon (`chrome_use_icon_title: "true"`), and standard pages automatically compact their layouts via Jinja conditionals.

### `container` vs. `obj`

- Use **`container`** for invisible layout grouping (flex rows/columns, grid wrappers). `container` defaults to `width: 100%`, `height: 100%`, transparent background, zero border/padding, and does not intercept touch events.
- Use **`obj`** when you want a visible card, tile, or modal popup surface. In [`gui/themes/main.yaml`](file:///x:/esphome/gui/themes/main.yaml), `obj` is styled as a rounded card (`bg_color: c_bg_s`, `border_width: 1`, `radius: 20`) matching `button`.

---

## 3. Standard vs. Opt-In Pages

### Standard Pages (Included by [`gui/main.yaml`](file:///x:/esphome/gui/main.yaml))

Every device that includes `gui/main.yaml` (or a `gui/<W>x<H>.yaml` stub) receives 5 built-in pages:

1. `page_analog_clock` ([`gui/pages/clock/`](file:///x:/esphome/gui/pages/clock))
2. `page_temperature` ([`gui/pages/temperature/`](file:///x:/esphome/gui/pages/temperature))
3. `page_nordpool` ([`gui/pages/nordpool/`](file:///x:/esphome/gui/pages/nordpool))
4. `page_weather` ([`gui/pages/weather/`](file:///x:/esphome/gui/pages/weather))
5. `page_status` ([`gui/pages/status/`](file:///x:/esphome/gui/pages/status))

> [!TIP]
> A device can remove any standard page it does not need by adding `- id: !remove <page_id>` under `lvgl: pages:`.

### Opt-In Pages (`grid_dashboard`, `climate`, Module Pages)

Additional pages are added in the root device YAML under `lvgl: pages:`:

```yaml
lvgl:
  pages:
    # 1. Configurable NxM Grid Dashboard (default layout: 2x2)
    - <<: !include
        file: gui/pages/grid_dashboard/page.yaml
        vars:
          page_title: "Kontrollpanel"
          layout: 4x2   # <rows>x<cols> shorthand (e.g. 2x2, 3x2, 4x2)

    # 2. Full-screen Climate Page
    - <<: !include
        file: gui/pages/climate/page.yaml
        vars:
          page_title: "Varmepumpe"
```

---

## 4. Dashboard Tiles (`gui/tiles/`)

Tiles are self-contained ESPHome packages (`local.yaml` for on-device ESPHome components, `remote.yaml` for Home Assistant entities) that combine:

1. The LVGL tile widget (injected into `page_dashboard` via `lvgl: pages: - id: !extend ${page_id}`).
2. The underlying ESPHome / Home Assistant sensors, switches, or scripts that keep the tile's icon color and labels synchronized.

### Example: Adding Tiles to `page_dashboard`

```yaml
packages:
  # Climate shortcut tile (also pulls in gui/pages/climate/sensors.yaml automatically)
  tile_climate: !include
    file: gui/tiles/climate/remote.yaml
    vars:
      uid: heatpump
      row: 0
      col: 0
      col_span: 2
      title: "Varmepumpe"
      entity_id: climate.faircon_norgespumpa12_proxy

  # Remote HA sensor tile
  tile_outdoor_temp: !include
    file: gui/tiles/sensor/remote.yaml
    vars:
      uid: outdoor_temp
      row: 1
      col: 0
      icon: "\U000F0595"
      title: "Ute"
      entity_id: sensor.ute_temperature
      unit: "°C"
      format: "%.1f"

  # Local display backlight tile (short click toggles, long press cycles brightness)
  tile_backlight: !include
    file: gui/tiles/light/local.yaml
    vars:
      uid: backlight
      row: 1
      col: 1
      title: "Skjermlys"
      light_id: backlight_light
```

---

## 5. E-Paper LVGL Architecture (`gui/epaper/`)

While `gui/main.yaml` drives continuous-refresh color LCD/MIPI panels, [`gui/epaper/base.yaml`](file:///x:/esphome/gui/epaper/base.yaml) provides a dedicated, power-aware LVGL subsystem tailored for bistable e-Paper displays (`200x200` M5Stack CoreInk / AirQuality, `400x300` CrowPanel 4.2", `800x480` Waveshare 7.5").

### A. On-Demand Refresh Pipeline (`gui/epaper/base.yaml`)

Unlike LCD panels that redraw at 30–60 FPS, e-Paper panels have slow physical waveform transitions (1–4 seconds) and degrade if refreshed continuously:

1. **Paused by Default**:
   - `lvgl_comp` initializes with `paused: true`, `update_interval: never`, and `on_draw_end: - lvgl.pause: { id: lvgl_comp }`.
   - Sensor updates modify LVGL widget states in memory (`lvgl.label.update`, `lvgl.line.update`, etc.) without triggering a physical SPI e-Paper transfer.
2. **Single-Frame `update_screen` Script**:
   - Triggered on scheduled intervals (e.g., `on_time` every 5 minutes), physical navigation buttons, Home Assistant navigation actions (`btn_previous_page`, `btn_home_page`, `btn_next_page` via `control_navigation.yaml`'s `on_page_navigate` hook), or initial boot data arrival (`epaper_boot_overlay`).
   - Increments `recorded_display_refresh`, runs `apply_epaper_theme_refresh` (which optionally inverts foreground/background colors every 5th refresh when `display.invert_every_5th: "true"` to prevent ghosting) and `epaper_refresh_status` (updating the header/footer clock and connectivity icons), resumes `lvgl_comp` for one render cycle, and calls `component.update: display_eink`.
3. **Home Assistant Page State & Remote Navigation (`control_navigation.yaml`)**:
   - [`gui/epaper/base.yaml`](file:///x:/esphome/gui/epaper/base.yaml) includes [`gui/functions/control_navigation.yaml`](file:///x:/esphome/gui/functions/control_navigation.yaml), exposing `btn_previous_page`, `btn_home_page`, `btn_next_page`, and `sensor_current_page` cleanly to Home Assistant just like LCD devices.
   - Because `set_page_title` publishes the plain text `title` to `sensor_current_page` (template text sensor) rather than scraping display label glyphs, Home Assistant always receives readable page titles even on compact 200×200 header layouts that display only an icon.
   - `epaper/base.yaml` defines `go_home` (`animation: NONE, time: 0ms`) and extends `on_page_navigate` to run `update_screen` when HA navigation buttons are pressed, while boot `go_home` skips refreshing so `epaper_boot_overlay` waits cleanly for initial data.
4. **Ghosting Scrubbing (`repair_script`)**:
   - Activated via `button_screen_repair` or a scheduled maintenance cron.
   - Cycles full-screen black and white fills on `epaper_scrubber_overlay` (`top_layer`) across three physical display updates before restoring the active page.

---

### B. Substitution Hierarchy & Package Inclusion

Board configurations (e.g., [`board/m5stack_coreink.yaml`](file:///x:/esphome/board/m5stack_coreink.yaml), [`board/elecrow_crowpanel_epaper.yaml`](file:///x:/esphome/board/elecrow_crowpanel_epaper.yaml)) only need to include **`epaper: !include ../gui/epaper/base.yaml`**. All theme, font, navigation chrome, default info page, and HA `control_navigation` packages are pulled in automatically by `base.yaml` using two substitution namespaces:

1. **Package Selectors (`epaper.*`)** — Must be literal strings in board/device configs because ESPHome's `do_packages_pass` expands `!include` paths in a single non-recursive pass:
   - `epaper.theme`: `"bw"`, `"bwry4"`, `"spectra6"`, or `"gray16"`. Includes `gui/epaper/themes/${epaper.theme}.yaml`.
   - `epaper.fonts`: `"bw"` (1-bpp crisp fonts) or `"color_gray"` (4-bpp anti-aliased fonts). Includes `gui/epaper/fonts/${epaper.fonts}.yaml`.
   - `epaper.nav`: `"coreink"`, `"crowpanel"`, `"reterminal"`, `"zectrix"`, or `"none"`. Includes `gui/epaper/chrome/nav_${epaper.nav}.yaml` (which sets `epaper_nav_pad_*` margins and renders side/top button hint ticks).

2. **Display & Page Options (`display.*`)** — Mirrors the LCD LVGL substitution convention:
   - `display.rotation`: `"0"`, `"90"`, `"180"`, or `"270"`. Automatically swaps `_ep_w` / `_ep_h` logical canvas dimensions on `90°`/`270°` rotations.
   - `display.home_page`: ID of the page shown on boot (default `"page_info"`, or `"page_local"` / `"page_room"` / `"page_daily"` / `"page_weather_station"`).
   - `display.buffer_size`: LVGL draw buffer fraction (default `"100%"`).
   - `display.invert_every_5th`: `"true"` or `"false"` (periodic full-screen polarity inversion).
   - `display.invert_accents`: `"true"` or `"false"` (high-contrast black fill on cards/headers).
   - `display.chrome.header`: `"true"` or `"false"` (top status bar visibility).
   - `display.chrome.status_in_footer`: `"true"` or `"false"` (compact bottom status bar for 200×200 displays without a top header).
   - `display.page.temperature.area_label`: Room title displayed on `page_temperature` (default `"Local"`).
   - `display.page.info.*`: Controls optional diagnostic rows and physical button hints on the built-in `page_info` (`skip`, `show_battery_pct`, `show_battery_volt`, `show_temp`, `show_humidity`, `show_sd`, `hint_1`, `hint_2`, `hint_3`).
   - `display.page.image.{dir,type}`: Source directory and image encoding for SD/flash image slideshow pages.

3. **Why [`gui/epaper/_defaults.yaml`](file:///x:/esphome/gui/epaper/_defaults.yaml) Exists**:
   - In ESPHome, a parent file's `substitutions:` block overrides all `substitutions:` blocks of files it includes under `packages:`.
   - By placing fallback defaults inside `_defaults.yaml` and including it **first** in `base.yaml`'s `packages:` map (`epaper_defaults: !include _defaults.yaml`), `themes/${epaper.theme}.yaml` and `chrome/nav_${epaper.nav}.yaml` can override default colors and nav padding, while board and root device YAMLs override everything.
   - Time source selection (`_ep_time_id`) is derived automatically from `${device.rtc}` (`time_rtc` when `device.rtc == "true"`, otherwise `time_homeassistant`).

---

### C. E-Paper Pages, Sections & Rolling Graphs

1. **Built-in `page_info` (`gui/epaper/pages/info/main.yaml`)**:
   - Included automatically by `base.yaml` (`epaper_info: !include pages/info/main.yaml`).
   - Displays device name, IP, MAC, RSSI, uptime, refresh counter, and board-configured battery/sensor/SD rows and physical button hints. Set `display.page.info.skip: "true"` on single-page devices to omit it from navigation.

2. **Self-Contained Full-Screen Page Packages (`gui/epaper/pages/<name>/`)**:
   Each page folder exposes a self-contained ESPHome package (`main.yaml`, or `local.yaml` / `remote.yaml` for `temperature/`) that bundles both its sensor/script bindings and its `lvgl: pages:` entry. Device YAMLs only need a single `!include` under `packages:` (in display order) — no duplicate `lvgl: pages:` block is required:
   - **`daily/main.yaml`** (`page_daily`): Two-column Daily Info Bento dashboard combining weather forecast, FolloRen, EV car status, NordPool graph, and bus departures.
   - **`weather_station/main.yaml`** (`page_weather_station`): Responsive weather header + 3 rolling 12-hour temperature graphs (`Ute`, `Foran`, `Bak`) adapting to portrait, landscape, and ultra-wide ribbon panels.
   - **`image/main.yaml`** (`page_image`): Full-screen image/poster viewer with edge-to-edge chrome hiding.
   - **`air_quality/main.yaml`** (`page_air_quality`): Multi-metric indoor air quality page (`SEN55` / `SCD40` CO₂, PM, VOC, NOx).
   - **`temperature/{local,remote}.yaml`** (`page_room` / `page_local` / `page_out`): Unified temperature hero page with optional 12-hour rolling graph (`remote.yaml`) or local sensor binding (`local.yaml`).

3. **Modular Sections (`gui/epaper/sections/<name>/`)**:
   - Reusable widget blocks (`clock`, `weather`, `temperatures`, `power_price`, `transport`, `calendar`, `sun`, `air_quality_bar`, `rolling_graph`) split into `section.yaml` (LVGL layout) and `sensors.yaml` (HA sensors & update scripts).

4. **12-Hour Rolling Polyline Graphs (`gui/epaper/sections/rolling_graph/`)**:
   - Renders a 36-point historical polyline (`line` widget) with dynamic min/max Y-axis scaling, dashed average reference line, and min/max/avg summary labels (`_core.yaml`).
   - **Data Sources**:
     - `remote_series.yaml` (**Preferred for HA entities**): Calls Home Assistant's `script.esphome_get_rolling_graph_data` via `homeassistant.action` (`capture_response: true`), parsing `response["response"]["csv"]` (36 comma-separated statistics samples over 12 hours at 20-minute intervals) into `id(${uid}_buf)`. Triggered once on `epaper_on_api_connected` and every `20min` (`sample_interval`) to avoid continuous script polling.
     - `local.yaml` / `remote.yaml`: Samples a live local or remote sensor into a ring buffer on the ESP32 every `sample_interval` (default `20min`).
     - `ha_attr.yaml` / `ha_series.yaml`: Legacy attribute/state CSV importers from `sensor.epaper_data_temperatures`.

---

### D. Virtual `host-sdl` Preview for E-Paper

You can build and preview any e-Paper resolution natively via WSL + Windows X11/WSLg without physical hardware:

```powershell
# 200x200 (defaults to CoreInk nav, rotation 90, page_room)
pwsh tools/host_sdl/host-sdl.ps1 run -Epaper -Size 200x200

# 400x300 (CrowPanel 4.2" layout)
pwsh tools/host_sdl/host-sdl.ps1 run -Epaper -Size 400x300

# Capture screenshot of the active host-sdl window
pwsh tools/host_sdl/host-sdl.ps1 screenshot
```

---

## 6. Authoring Checklist for New Pages & Tiles

1. **Page Boilerplate**:
   - Every LCD page in `gui/pages/<name>/page.yaml` should set `styles: page_style`, call `script.execute: { id: set_page_title, title: ..., icon: ... }` in `on_load:`, and include `<<: !include ../../functions/swipe_navigation.yaml`.
   - Wrap the page's main body in a `- container:` with `styles: container_style`.
   - Every e-Paper page in `gui/epaper/pages/<name>/page.yaml` should compute its layout using `epaper_header_h`, `epaper_footer_h`, `epaper_pad_x`, `epaper_pad_y`, and `epaper_nav_pad_*` (from `gui/epaper/base.yaml`) so it automatically respects top header (`display.chrome.header`), bottom footer (`display.chrome.status_in_footer`), and side button hint margins (`epaper_nav_pad_*`).
2. **Jinja Substitution Rules in ESPHome**:
   - Do not reference one computed Jinja substitution inside another computed Jinja substitution in the same `substitutions:` block (ESPHome evaluates them in a single pass before chained expressions resolve). Write each formula directly in terms of `screen_width`, `screen_height`, `page_pad_top/bottom`, and `container_pad_*`.
   - In `packages:` `!include` paths, only reference literal string substitutions (e.g. `${epaper.theme}`), never chained substitutions, because `do_packages_pass` runs a single non-recursive `substitute()` pass.
   - In inline YAML lambdas, never write an unquoted ternary `a ? b : c` on a single line (YAML parses `:` as a mapping key); always use a block scalar (`!lambda |-`).
   - In `homeassistant.action` `data:` maps, wrap substituted numeric parameters in `!lambda 'return "${hours | default("12")}";'` so ESPHome's Jinja evaluator does not coerce them into `EInt` and fail `cv.string_strict`.
   - In raw `lvgl.label.update` `format:` strings (when not using C++ lambdas), always escape literal percent signs as `%%`. In sensor tiles, inline lambdas handle unit concatenation automatically so `unit: "%"` is safe as-is.
3. **Font Glyphs**:
   - Whenever a new MDI icon `\U000Fxxxx` is used in an LCD or e-Paper page or tile, add its codepoint to [`gui/fonts/glyphs.yaml`](file:///x:/esphome/gui/fonts/glyphs.yaml) so it is compiled into `mdi_sm`..`mdi_xxl` and into the e-Paper font definitions in [`gui/epaper/fonts/_fonts.yaml`](file:///x:/esphome/gui/epaper/fonts/_fonts.yaml), [`gui/epaper/fonts/bw.yaml`](file:///x:/esphome/gui/epaper/fonts/bw.yaml), and [`gui/epaper/fonts/color_gray.yaml`](file:///x:/esphome/gui/epaper/fonts/color_gray.yaml).
