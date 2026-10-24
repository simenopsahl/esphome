# LVGL Reference for ESPHome — Quick Guide for AI Assistants

ESPHome uses **LVGL v9.5** (not v8). Documentation: <https://esphome.io/components/lvgl/>

## Widgets

ESPHome LVGL supports the widgets:  
`animimg`, `arc`, `bar`, `button`, `buttonmatrix`, `canvas`, `checkbox`, `container`, `dropdown`, `image`, `keyboard`, `label`, `led`, `line`, `list`, `meter`, `msgboxes`, `obj`, `qrcode`, `roller`, `slider`, `spinbox`, `spinner`, `switch`, `table`, `tabview`, `textarea`, `tileview`.  
Notice that this list may not include every single widget available in LVGL, and some widgets may have specific limitations or differences in ESPHome's implementation. Always refer to the official ESPHome LVGL documentation for the most accurate and up-to-date information.

## Grid Layout

- Shorthand: `layout: 5x9` means **5 rows × 9 columns** (rows first, cols second)
- Manual: `grid_rows: [FR(1), FR(1), 100px]` / `grid_columns: [FR(1), CONTENT]`
- `FR(n)` = proportional free units, `CONTENT` = size to children, or fixed pixel values
- Child placement: `grid_cell_row_pos` / `grid_cell_column_pos` (0-based)
- Spanning: `grid_cell_row_span` / `grid_cell_column_span` (default 1)
- Cell alignment: `grid_cell_x_align` / `grid_cell_y_align` — values: `START`, `END`, `CENTER`, `STRETCH`

## Flex Layout

- `flex_flow`: `ROW`, `COLUMN`, `ROW_WRAP`, `COLUMN_WRAP` (+ `_REVERSE` variants)
- `flex_align_main`: `START`, `END`, `CENTER`, `SPACE_EVENLY`, `SPACE_AROUND`, `SPACE_BETWEEN`
- `flex_align_cross`: same as above + `STRETCH`
- `flex_align_track`: alignment of tracks (CSS align-content equivalent)
- `flex_grow`: integer to distribute remaining space proportionally
- Shorthands: `layout: horizontal` / `layout: vertical`

## CRITICAL: Layouts Override Direct Positioning

When a parent uses flex or grid layout, child `x`, `y`, `width`, `height`, and `align` properties are **ignored/overridden** by the layout engine. Use the layout's alignment options (e.g., `flex_align_cross: center`) instead of `align: center` on children.

The `hidden`, `ignore_layout`, and `floating` flags exclude widgets from layout calculations.

## Image Widget — Gotchas

### Tiling Problem

LVGL v8 **tiles/repeats images** when the widget's bounding box is larger than the source image. This happens when:

- You set explicit `width`/`height` larger than the image
- You add `pad_top`/`pad_bottom` to an image with `SIZE_CONTENT` (inflates the bounding box)

### `inner_align` Does NOT Exist

`inner_align` is **not a valid property** for image widgets in ESPHome LVGL. Do not use it.

### Solution: The Object Wrapper Pattern

To create a fixed-size slot for variable-size images without tiling, wrap the image in an `obj`:

```yaml
# Define an anchor for reuse
- &image_wrapper
  height: 80        # fixed slot size
  width: 80
  bg_opa: TRANSP    # invisible background
  scrollbar_mode: "OFF"

# Use it in widgets
- obj:
    <<: *image_wrapper
    widgets:
      - image:
          id: my_icon
          src: my_image_80
          align: center   # center within the fixed-size obj
```

The `obj` provides a consistent fixed-size container. The image inside uses `align: center` (which works here because the obj is NOT using a layout). No tiling occurs because the image stays at its natural size.

### Image Properties

- `src`: ID of an ESPHome `image:` configuration (Required)
- `angle`: 0-360 rotation
- `zoom`: 0.1-10 scale factor
- `offset_x` / `offset_y`: shift image position within widget
- `pivot_x` / `pivot_y`: rotation center point
- `antialias`: better quality transforms (slower)
- `mode`: `VIRTUAL` (default, overflows) or `REAL` (clips to widget size)
- `image_recolor` + `image_recolor_opa`: tint the image (opa defaults to TRANSP — must set both)
- Format: RGB565 with optional transparency

## Sizing

- `SIZE_CONTENT`: widget auto-sizes to fit its children (only right/bottom children counted)
- `min_width`, `max_width`, `min_height`, `max_height`: size limits
- Percentage values: relative to parent's content area
- CSS box model: bounding box → border → padding → content

## Container vs Obj

- **`container`**: unstyled, invisible by default, 100% width/height — use as an invisible grouping element
- **`obj`**: has default LVGL styling (rounded rectangle, background), catches touches — use when you want a visible box

## Opacity

- `TRANSP` = fully transparent, `COVER` = fully opaque
- Also accepts: float `0.0`-`1.0`, percentage `0%`-`100%`, lambda returning `0`-`255`

## Scrollbar Mode

- Values: `"OFF"`, `"ON"`, `"ACTIVE"`, `"AUTO"` (default)
- **Must quote `"OFF"` and `"ON"`** in YAML (otherwise parsed as boolean)

## Colors

- Hex: `0xFF0000`
- CSS names: `springgreen`, `dodgerblue`, `coral`, `teal`, etc.
- In lambda: `lv_color_hex(0xRRGGBB)`
- **This library convention**: always use named colors, never raw hex. See `themes/colors*.yaml`.

## Font / Glyph Pitfalls

- `strftime("%l")` pads single-digit hours with a space character
- If the font doesn't contain a space glyph, LVGL renders a missing-glyph rectangle (ghost box)
- Fix: use direct math (`hour % 12`, `std::to_string()`) instead of strftime for custom fonts with limited glyphs

## YAML Anchor Pattern for Reusable Styles

Define anchors under a dummy key (ignored by ESPHome), then reference with `<<: *name`:

```yaml
.my_styles:
    - &card_base
      bg_color: gray200
      bg_opa: cover
      radius: 3
      layout:
        type: flex
        flex_flow: COLUMN
        flex_align_cross: center

# Later in widgets:
- obj:
    <<: *card_base
    widgets:
      - label:
          text: "Hello"
```

## Useful Debug Tip

From the ESPHome docs: to visualize real, calculated sizes of transparent widgets, temporarily set `outline_width: 1` on them.
