# Modular LVGL GUI Architecture

This directory (`gui/`) implements a resolution-independent, package-based LVGL user interface system for ESPHome displays ranging from tiny `128×128` LCDs to `480×800` / `800×480` touch panels.

For general ESPHome LVGL widget and layout syntax rules, see [`gui/LVGL_REFERENCE.md`](file:///x:/esphome/gui/LVGL_REFERENCE.md).

---

## 1. Directory Structure

```text
gui/
  ├── main.yaml               # Unified responsive LVGL entry point (pages, theme, styles, chrome)
  ├── <W>x<H>.yaml            # Thin resolution stubs (set screen_width/screen_height and include main.yaml)
  ├── 5x5.yaml                # Standalone config for 5×5 RGB LED matrix displays
  ├── common/                 # Global top_layer chrome & system overlays
  │     ├── chrome/
  │     │     ├── base.yaml   # Chrome height/padding math & safe no-op `set_page_title` script
  │     │     ├── header.yaml # Top status bar (`titlebar`: Wi-Fi, page title/icon, clock, HA status)
  │     │     └── footer.yaml # Bottom touch navigation bar (`navbar`: Prev, Home, Next)
  │     ├── boot_screen/      # Boot splash overlay (`widget.yaml` + `sensors.yaml`)
  │     └── ota_screen/       # OTA progress bar overlay (`widget.yaml` + `sensors.yaml`)
  ├── fonts/
  │     ├── main.yaml         # 5-tier responsive font sizing (`font_sm`..`font_fl`, `mdi_sm`..`mdi_xxl`)
  │     └── glyphs.yaml       # Material Design Icons (MDI) codepoints included across all `mdi_*` sizes
  ├── functions/              # Reusable behavioral packages & fragments
  │     ├── control_auto_display.yaml  # Presence-based backlight dimming & auto-off
  │     ├── control_navigation.yaml    # HA page navigation buttons & `sensor_current_page`
  │     ├── swipe_navigation.yaml      # Left/right touch swipe page navigation fragment
  │     ├── anti_burn_in.yaml          # Pixel-shift antiburn helper
  │     ├── number_sensor.yaml         # Bind an arbitrary LVGL label to a Home Assistant numeric sensor
  │     └── text_sensor.yaml           # Bind an arbitrary LVGL label to a Home Assistant text sensor
  ├── img/                    # Image & SVG assets (boot logo, weather condition icons)
  ├── pages/                  # Full-screen LVGL pages (`page.yaml` + optional `sensors.yaml`)
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
  ├── styles/                 # Shared LVGL `style_definitions:` (loaded via `!include_dir_merge_list styles`)
  │     ├── page_styles.yaml      # `- id: page_style` (reserves `page_pad_top` & `page_pad_bottom` for chrome)
  │     └── container_styles.yaml # `- id: container_style` (100%x100% transparent content area with padding)
  ├── themes/                 # LVGL widget theme & color palettes
  │     ├── main.yaml             # Default styling for `button`, `obj` (card surface), `label`, `arc`, etc.
  │     └── colors_*.yaml         # Color palette definitions (`colors_night_owl.yaml`, `colors.yaml`, etc.)
  └── tiles/                  # Modular dashboard tiles that `!extend` `page_dashboard`
        ├── binary_sensor/    # `local.yaml` & `remote.yaml` (read-only on/off indicator tile)
        ├── button/           # `local.yaml` & `remote.yaml` (stateless action/service trigger tile)
        ├── climate/          # `local.yaml` & `remote.yaml` (live setpoint/action tile -> opens `page_climate`)
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
      unit: "°C"          # Note: escape literal '%' as '%%' (unit: "%%")
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

## 5. Authoring Checklist for New Pages & Tiles

1. **Page Boilerplate**:
   - Every page in `gui/pages/<name>/page.yaml` should set `styles: page_style`, call `script.execute: { id: set_page_title, title: ..., icon: ... }` in `on_load:`, and include `<<: !include ../../functions/swipe_navigation.yaml`.
   - Wrap the page's main body in a `- container:` with `styles: container_style`.
2. **Jinja Substitution Rules in ESPHome**:
   - Do not reference one computed Jinja substitution inside another computed Jinja substitution in the same `substitutions:` block (ESPHome evaluates them in a single pass before chained expressions resolve). Write each formula directly in terms of `screen_width`, `screen_height`, `page_pad_top/bottom`, and `container_pad_*`.
   - In inline YAML lambdas, never write an unquoted ternary `a ? b : c` on a single line (YAML parses `: ` as a mapping key); always use a block scalar (`!lambda |-`).
   - In `lvgl.label.update` `format:` strings, always escape literal percent signs as `%%`.
3. **Font Glyphs**:
   - Whenever a new MDI icon `\U000Fxxxx` is used in a page or tile, add its codepoint to [`gui/fonts/glyphs.yaml`](file:///x:/esphome/gui/fonts/glyphs.yaml) so it is compiled into `mdi_sm`..`mdi_xxl`.

