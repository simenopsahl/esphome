# LLM Council Transcript — e-Paper Display Lambda to LVGL Migration

**Date:** 2026-11-10  
**Topic:** Migrating 14 ESPHome e-Paper Display Devices from Imperative C++ `display: pages: lambda:` to Declarative LVGL (`lvgl:`)

---

## 1. Original User Request

> `/llm-council`  
> Next task: EPaper Display lambda to LVGL migration  
> I created this file `x:\esphome\tools\gui_migration\EPAPER.md` based on the README in the same folder.  
> It has some thoughts of mine that didn't quite fit in this chatbox. Feel free to edit that file as you go, and possibly adding steps to `SESSIONS.md`.  
> We are a ways away to start migrating and editing yaml files. I'd like to consider the project first, and see if you have questions or suggestions.  
> This task could prove to be almost as big as the GUI-migration itself, it might be prudent to divide it into sessions as well.

---

## 2. Framed Question (with Workspace Context Enrichment)

### Core Decision & Architectural Question
How should we architect and execute the migration of 14 ESPHome e-Paper display devices from imperative C++ `display: pages: lambda:` rendering to declarative LVGL (`lvgl:`), and how much should be unified vs. kept device-specific?

### Enriched Technical Context from the Workspace
1. **Device Fleet (14 E-Paper Devices)**:
   - **Active in production (6 devices)**:
     - `m5s-coreink-9fc390` & `m5s-coreink-9fc2c8` (ESP32-PICO-D4, **no PSRAM**, 200x200 B/W, `waveshare_epaper` `1.54inv2`). Shows local/room/outdoor temp (`120px` font) + info page + physical stick nav hints.
     - `m5s-air-quality-b62db0` (ESP32-S3, 200x200 B/W, `waveshare_epaper` `1.54inv2`). Shows local temp, SEN55/SCD40 air quality table, and room temp with an ESPHome `graph:` (`it.graph(...)`).
     - `waveshare-weather-station` (ESP32-WROOM-32, **no PSRAM**, 400x300 B/W rotated 270° -> 300x400 portrait, `waveshare_epaper` `4.20in`). Renders weather icons + 3 historical temperature `graph:` traces (`it.graph(...)`).
     - `waveshare-display-board` (ESP32-WROOM-32, **no PSRAM**, 800x480 B/W rotated 90° -> 480x800 portrait, `waveshare_epaper` `7.50in-bV2`). "Weatherman"-style daily info dashboard: weather + 5-slot forecast, FolloRen waste pickup, NordPool 9h `graph:`, bus departures, EV battery.
   - **Inactive / Storage / Testing (8 devices)**:
     - `m5s-coreink-2df58c` (200x200 B/W), `waveshare-simen` (400x300 B/W), `elecrow-crowpanel-42-4dbf00` (ESP32-S3 + 8MB PSRAM, 400x300 B/W, external `crowpanel_epaper`), `elecrow-crowpanel-579-b89ddc` (ESP32-S3 + 8MB PSRAM, 792x272 B/W, `crowpanel_epaper`), `seeed-trmnl-e1001-85a77d0` (ESP32-S3 + PSRAM, 800x480 B/W, `epaper_spi`), `seeed-trmnl-e1002-ec2478` (ESP32-S3 + PSRAM, 800x480 **Spectra 6-color**, `epaper_spi`), `seeed-trmnl-e1003-190420` (ESP32-S3 + PSRAM, **1872x1404** 16-grayscale, `it8951` + GT911 touch), `seeed-trmnl-diy-b08e18`, `zectrix-note4c` (ESP32-S3 + 8MB PSRAM, 400x300 **4-color B/W/Red/Yellow**, `epaper_spi` `jd79660`).
2. **How E-Paper Currently Works (`features/epaper.yaml`)**:
   - `update_interval: never` on `display_eink`.
   - Every 5 minutes (if `data_updated == true` and `presence_detected` is ON), `update_screen` calls `component.update: display_eink` and increments `recorded_display_refresh`.
   - **Every 5th refresh (`recorded_display_refresh % 5 == 0`)**, `on_value` swaps the C++ `Color` globals (`epaper_bg_color` <-> `epaper_text_color`, and rotates `epaper_accent*_color`) to prevent e-ink ghosting/burn-in.
   - Includes `page_black` and `page_white` with a `repair_script` that cycles black/white with 20s delays. Currently, because `page_black`/`page_white` are in `display: pages:`, pressing "Next Page" on physical buttons accidentally cycles through the blank black and white repair pages!
   - Several pages use ESPHome's native `graph:` component (`it.graph(...)`), which draws directly onto an ESPHome `Display` buffer in C++ lambdas, **not** an LVGL widget.
3. **Key Questions & Tensions Raised by the User in `EPAPER.md`**:
   - **Display drivers**: Should we migrate legacy `waveshare_epaper` (and `crowpanel_epaper`) devices to `epaper_spi` first?
   - **Bespoke layouts vs. shared `gui/`**: What belongs in a shared `gui/epaper.yaml` (or `features/epaper.yaml`) vs. modular widgets/sections vs. device root YAMLs?
   - **Theme & every-5th-refresh color inversion in LVGL**: How should LVGL handle 4 color modes (B/W, B/W/R/Y, Spectra 6-color, 16-grayscale) AND the every-5th-refresh color inversion?
   - **Verification**: How can we use `host-sdl` to capture baseline screenshots of the current C++ display lambdas before migrating to LVGL?

---

## 3. Round 1 — Independent Advisor Responses

### The Contrarian (Anonymized as Response B)
You are walking into a memory and timing trap by treating e-Paper as "just another LVGL target."

First, **5 of your 6 active production devices have NO PSRAM** (`ESP32-PICO-D4` and `ESP32-WROOM-32`). Look at `waveshare-display-board` (800x480) and `waveshare-weather-station` (400x300). On a WROOM-32 with BLE/Wi-Fi active, free internal heap is often under 80KB. LVGL requires a draw buffer (typically RGB565 in ESPHome's LVGL implementation—even `buffer_size: 12%` of 800x480 is ~92KB of contiguous RAM) *on top of* the e-Paper driver’s full-screen buffer (48KB for 800x480 1bpp) and the massive object tree of a complex "Weatherman" dashboard. You will hit OOM boot loops or heap fragmentation crashes on the exact production devices that currently work fine with zero-overhead `it.printf()` lambdas.

Second, **LVGL + e-Paper refresh mechanics are fundamentally mismatched**. LVGL ticks continuously and pushes partial tile flushes; e-Paper blocks or sequences multi-second SPI LUT transfers. Worse, your "every-5th-refresh color inversion" via `lvgl.style.update` is a hack waiting to break: e-Paper controllers have native hardware inversion commands (`invert_colors`) or LUT phase clearing—doing this by mutating dozens of LVGL styles in YAML will miss hardcoded widget colors and double your YAML maintenance.

Third, **replacing `it.graph()` with LVGL `line` widgets is a silent regression**. ESPHome's `graph:` automatically samples, ring-buffers, and auto-scales local/HA sensors in C++. LVGL `line` requires you to manually maintain point arrays, scale coordinates in lambdas, and store history—writing *more* imperative C++ inside YAML than you had before.

Finally, **do not touch `epaper_spi` at the same time as LVGL**. `epaper_spi` is still maturing in ESPHome and lacks several `waveshare_epaper` models. Coupling a driver rewrite with a rendering engine rewrite guarantees you won't know which layer broke when a screen won't refresh.

---

### The First Principles Thinker (Anonymized as Response D)
You are asking how to force LVGL onto 14 e-Paper devices, when the real question is: **Why is an ESP32 without PSRAM rendering complex layouts at all—and why are you using an interactive 60fps RGB widget engine for a static 1-bit electrophoretic medium?**

Strip away the assumption that "everything in this repo must use LVGL." Look at the physics and silicon constraints from the ground up:

1. **RAM vs. Resolution Physics**: Five of your six *active* production devices (`m5s-coreink`, `waveshare-weather-station`, `waveshare-display-board`) run on legacy ESP32s with **no PSRAM** (~120KB usable heap). LVGL renders in RGB565 (16-bit) by default before down-converting to 1-bit `display:`. Even at `buffer_size: 12%`, an 800x480 display requires ~96KB of contiguous DMA/internal RAM just for the draw buffer, plus tens of kilobytes for persistent LVGL object trees, styles, and large (`120px`) font caches. Meanwhile, ESPHome’s native 1-bit `display:` buffer for 800x480 is only 48KB total. Putting LVGL on a no-PSRAM ESP32 driving an 800x480 e-Paper panel is manufacturing an OOM crisis for zero visual gain.
2. **The Anti-Ghosting Inversion Myth**: Your "every 5th refresh" inverted theme (`recorded_display_refresh % 5 == 0`) is a misunderstanding of e-Paper physics. Leaving an e-ink screen inverted (black background) for 5 minutes *causes* charge-trapping and ghosting; a full hardware LUT waveform flash (white -> black -> white during a single refresh cycle) is what clears ghosting. Delete the color-swapping hack and `page_black`/`page_white` entirely. Let the hardware waveform do its job, and your "how do we invert LVGL styles dynamically" problem vanishes.
3. **The Real Architecture**: Split the fleet by physical capability, not ideological uniformity:
   - **Legacy / No-PSRAM B/W Panels (`waveshare-display-board`, `weather-station`, `coreink`)**: Either keep imperative `display:` (fixing the page-cycling bug by removing `page_black`/`page_white` from the page loop) OR turn them into dumb `online_image:` Puppet/TRMNL clients where Home Assistant renders the 1-bit PNG and the ESP32 uses near-zero RAM.
   - **PSRAM S3 Panels (`air-quality`, CrowPanel, TRMNL, Zectrix)**: Migrate *only* these to `epaper_spi` + LVGL, using declarative YAML for layout and moving `page_black`/`page_white` to an LVGL `top_layer` overlay hidden by default so button navigation never hits it.

---

### The Expansionist (Anonymized as Response A)
Everyone is treating this as a port of 14 legacy screens. That’s thinking way too small. What you actually have here is the birth of a **universal, resolution-independent "Ambient E-Ink Widget Engine"**—your own local, zero-cloud, real-time TRMNL/Weatherman platform that can unify LCD and e-Paper into a single composable UI ecosystem.

Here is the massive upside hiding in plain sight:

1. **Stop Building "Bespoke Screens"—Build "Ambient Cards" (`gui/cards/` or `gui/tiles/`)**:
   The "Weatherman" board (`waveshare-display-board`) isn't a bespoke one-off; it’s a **bento-grid of reusable information cards** (Waste Pickup, Bus Departures, EV Battery, 5-Slot Weather, NordPool Line Graph, Air Quality Table). If you build those as flex-layout LVGL containers, suddenly your `seeed-trmnl-e1002` (Spectra 6-color), `crowpanel-579` (ultra-wide ribbon), and `e1003` (1872x1404 tablet!) aren't "8 inactive devices sitting in a drawer"—they become instant, zero-effort ambient dashboards in every room just by mixing and matching cards in a flex grid! Even your LCD `grid_dashboard` devices can reuse these exact same cards.
2. **Turn LVGL Style Inversion into a First-Class "Chameleon Theme Engine"**:
   Don't mourn the loss of C++ `Color` globals—`lvgl.style.update` on 3 master styles (`style_epaper_bg`, `style_epaper_fg`, `style_epaper_accent`) is *more* powerful. It gives you instant, runtime theme switching not just for 5th-refresh anti-ghosting, but for **semantic alert states**. On B/W screens, an urgent waste pickup or price spike can invert a single card to high-contrast black; on your 4-color (`zectrix-note4c`) and 6-color (`e1002`) screens, that exact same card style automatically lights up in physical Red or Yellow ink!
3. **Move `page_black`/`page_white` to `lvgl.top_layer`**:
   Kill the button-navigation bug forever by making the e-ink scrubber an LVGL `top_layer` overlay that un-hides during `repair_script`, leaving page navigation 100% clean.
4. **Upgrade `host-sdl` into a Visual Regression Studio**:
   Running `host-sdl` on the C++ lambdas first isn't just a sanity check—it lets you generate a side-by-side "Before & After" gallery across all 4 color palettes (1-bit, 4-color, 6-color, 16-gray) in seconds on WSL before touching a single physical device.

---

### The Outsider (Anonymized as Response E)
Look at what is actually in front of you before building a grand unified framework: **8 of your 14 devices are sitting in a drawer, and the 6 that actually run only represent 4 unique screens.**

You are treating this as a 14-device platform problem when it is really a 4-screen problem. Worse, every single one of your active screens shares a fatal constraint you glossed over: **4 of the 6 active devices have no PSRAM**, including an 800x480 screen. LVGL is a heavy GUI engine designed for animated, interactive screens with plenty of memory. Putting LVGL on memory-starved, static, black-and-white e-paper displays—just so your config files look like your LCD config files—is forcing a square peg into a round hole.

Three things jump out to someone with no attachment to your architecture:

1. **Your anti-ghosting trick makes no sense.** Swapping background and text colors every 5th refresh means that for 20% of the day, your wall displays are stuck in an ugly, inverted "dark mode" for 5 minutes at a time. That isn't how e-paper clears ghosting—the hardware driver's full-refresh waveform flashes black and white during the transition and lands back on normal white. If you still want a periodic wipe, flash a full-screen black/white overlay object for one cycle and hide it. That immediately eliminates your entire "how do we dynamically invert LVGL styles" headache AND stops buttons from cycling into blank repair pages.
2. **Don't throw away automatic graphs for manual math.** Your current setups get historical sensor graphs for free. Migrating to LVGL means rewriting automatic history tracking into manual point-calculation arrays. Why do more work for the exact same black-and-white line?
3. **Unify the plumbing, not the layouts.** A 200x200 tiny cube with a 120px number and a 480x800 bus-and-garbage dashboard have zero layout in common. Share the refresh timer and button-skip logic; leave the screen layouts inside the 4 device files that actually use them.

---

### The Executor (Anonymized as Response C)
14 devices is a distraction. You have 6 active production devices, and 4 of them (`m5s-coreink` x2, `waveshare-weather-station`, `waveshare-display-board`) run on original ESP32s with **no PSRAM**. Before writing a single line of unified `gui/` architecture, you need to prove an ESP32 without PSRAM can run Wi-Fi, the HA API, and an LVGL buffer at 800x480 (`waveshare-display-board`, 384,000 pixels) without OOM-crashing on boot.

Here is what you do Monday morning:

1. **Test the hardest constraint first (Day 1)**: Build a minimal LVGL config for `waveshare-display-board` (800x480, no PSRAM) with `buffer_size: 12%` and `color_depth: 16`. Compile and flash it. If it runs out of heap, those non-PSRAM boards either stay on C++ lambdas or get cheap ESP32-S3 controller swaps. Don't architect around a hardware impossibility.
2. **Don't touch working display drivers**: Leave `waveshare_epaper` and `crowpanel_epaper` alone. Migrating drivers and UI frameworks simultaneously guarantees you won't know which one broke when the screen stays blank. LVGL sits on top of `display_eink` regardless of the underlying driver.
3. **Unify plumbing, not layouts**: Put only the refresh timer, `lvgl.pause`/`resume`, and a full-screen `top_layer` repair overlay (`lvgl.widget.show`/`hide`—fixing the button-cycling bug immediately) into `features/epaper_lvgl.yaml`. Keep page layouts 100% in the device root YAMLs. E-paper screens have bespoke dimensions and bezel buttons; forcing them into shared `gui/pages/` templates is busywork.
4. **Inversion in 6 lines**: Define two LVGL styles (`style_epaper_page` and `style_epaper_widget`) and toggle their `bg_color`/`text_color` via `lvgl.style.update` inside the existing `recorded_display_refresh % 5 == 0` check.
5. **Migrate in strict order**:
   - **Step 1**: `m5s-coreink-2df58c` (inactive 200x200 spare, no graphs). Prove LVGL + 5th-refresh style swap on real e-ink.
   - **Step 2**: Clone to the two active CoreInks.
   - **Step 3**: Build a 30-line rolling-array `script:` for the LVGL `line` widget to replace `it.graph(...)`, then migrate `m5s-air-quality-b62db0`.
   - **Step 4**: Migrate `waveshare-weather-station` and `waveshare-display-board` last. Ignore the 8 inactive drawer devices until the 6 active ones are done.

---

## 4. Round 2 — Anonymous Peer Reviews

*Mapping used in peer review: A = Expansionist, B = Contrarian, C = Executor, D = First Principles, E = Outsider.*

### Peer Review by The Contrarian
1. **Strongest Response: D (followed closely by B and C).** D is the only one that connects the silicon heap math to the actual physics of electrophoretic displays: holding an inverted black screen for 5 minutes *increases* ghosting rather than clearing it, and splitting the fleet by PSRAM vs. non-PSRAM avoids forcing an impossible architecture onto legacy WROOM-32 boards. C also deserves credit for demanding a Day-1 OOM smoke test on the 800x480 non-PSRAM board before designing anything.
2. **Biggest Blind Spot: Response A.** A is pure architectural fantasy that completely ignores hardware constraints. Building deeply nested flex-grid "Ambient Cards" and a "Chameleon Theme Engine" for an 800x480 ESP32 with **no PSRAM** will instantly OOM-crash during boot. Furthermore, Response D’s suggestion to use `online_image:` or `buffer_size: 12%` (in C) overlooks that decoding a full-screen 800x480 PNG *also* requires tens of kilobytes of RAM without PSRAM.
3. **What ALL Five Responses Missed:**
   - **LVGL Font Subpixel Anti-Aliasing on 1-bit E-Paper:** By default, ESPHome’s LVGL renders anti-aliased (4bpp) fonts in RGB565. When down-converted to 1-bit B/W `display:` without thresholding/dithering control, anti-aliased edges turn into jagged, unreadable blobs or disappear entirely unless `bpp: 1` fonts are explicitly compiled.
   - **LVGL’s Asynchronous Flush vs. `component.update: display_eink`:** Calling `component.update` on `display_eink` pushes the *existing* display buffer to the panel; if `lvgl.resume` or `lvgl.style.update` is triggered in the same script tick, LVGL hasn't actually rendered and flushed its dirty areas to the `display_eink` buffer yet, so the screen will always refresh one cycle behind the data!

### Peer Review by The First Principles Thinker
1. **Strongest Response: B (closely complemented by D and E).** Response B nails the three fatal engineering flaws in the premise: no-PSRAM ESP32 heap exhaustion at 800x480 (RGB565 draw buffer + 48KB 1bpp display buffer + Wi-Fi/BLE stack), the architectural regression of replacing ESPHome’s self-contained `it.graph()` ring-buffer with manual LVGL `line` point arrays, and the danger of coupling a driver migration (`epaper_spi`) with a renderer rewrite. Responses D and E rightly reinforce that leaving an e-Paper screen inverted for 5 minutes causes ghosting rather than curing it.
2. **Biggest Blind Spot: Response A.** Response A completely ignores physical hardware constraints. Proposing a universal flex-grid "Ambient Card" engine with runtime style-inversion themes across 800x480 and 400x300 ESP32-WROOM-32 devices with **zero PSRAM** is pure fantasy—those boards will OOM-crash before rendering a single frame.
3. **What ALL Five Missed: Deep Sleep & Battery Lifespan vs. Continuous Polling.** Devices like `m5s-coreink` and `seeed-trmnl` are designed as battery-powered e-Paper nodes, yet `features/epaper.yaml` runs continuously (`every 5 minutes` with live `presence_detected` and HA state listeners). None of the responses addressed how LVGL initialization overhead, font decompression, or HA sensor history graphs (`it.graph` vs. LVGL `line`) interact with deep sleep or power consumption. Furthermore, nobody noted that `host-sdl` only renders `lvgl:` or `display:` individually—to compare C++ lambdas against LVGL in `host-sdl`, you need a mock sensor data injection file so both render identical states.

### Peer Review by The Expansionist
1. **Strongest Response: Response C (with Response A’s modularity).** Response C cuts straight to an executable validation sequence: test the 800x480 no-PSRAM heap ceiling first, decouple display driver changes (`epaper_spi`) from the LVGL migration, fix the `page_black`/`page_white` button bug via `lvgl.top_layer`, and use a two-style `lvgl.style.update` toggle. Combining C's pragmatic execution order with A's modular "Ambient Cards" for the S3/PSRAM fleet gives both immediate safety and long-term architectural payoff.
2. **Biggest Blind Spot: Response E (and B/D's dismissal of the 8 inactive devices).** Response E assumes 8 devices are in a drawer because they aren't needed, missing *why* they are in a drawer: hand-coding pixel coordinates in C++ lambdas for 800x480, 792x272, 1872x1404, and 4/6-color displays is prohibitively tedious! Declarative LVGL flex/grid cards (Response A) is the exact unlock that turns those 8 high-end S3+PSRAM boards from shelfware into deployed room dashboards.
3. **What ALL Five Responses Missed:** Every advisor assumed LVGL on `waveshare-display-board` (800x480, no PSRAM) requires ~96KB (`buffer_size: 12%`). Nobody noted that ESPHome LVGL supports **`buffer_size: 2%` or `4%`** (~15–30KB) because e-Paper doesn't care about frame-rate—LVGL simply renders in more horizontal strips into the existing 48KB 1-bit `display_eink` buffer before a single SPI flush! Furthermore, for replacing `it.graph()`, nobody mentioned **`lvgl.canvas`** or fetching a pre-rendered history array from HA (`recorder.get_statistics`), which avoids per-node ring-buffer boilerplate.

### Peer Review by The Outsider
1. **Strongest Response: Response D (with Response C's Day-1 test).** Response D is the only one that steps back from the how-to and challenges the premise based on hardware physics. It explains *why* the no-PSRAM boards will choke (16-bit RGB565 LVGL draw buffer on top of the 48KB 1-bit display buffer), why holding an inverted black screen for 5 minutes actually *worsens* e-ink charge trapping, and offers a practical alternative. Pairing D's architecture split with C's "test the 800x480 WROOM-32 on Day 1" gives an immediately actionable plan.
2. **Biggest Blind Spot: Response A.** Response A is pure architectural astronautics. It pitches a "universal Bento-grid Ambient Card engine" and "Chameleon Theme Engine" while completely ignoring that 5 of the 6 active devices lack PSRAM and cannot even allocate an LVGL draw buffer at 800x480, let alone deep flex-container trees. It also perpetuates the broken 5th-refresh inversion hack by dressing it up as a feature.
3. **What ALL Five Missed: Font Flash/RAM Footprint and Anti-Aliasing on 1-Bit Screens.** Nobody mentioned **1-bit font rendering in LVGL**. Current C++ `display:` renders crisp 1-bit bitmap fonts directly. LVGL defaults to 4-bpp anti-aliased fonts rendered into a 16-bit RGB565 buffer; when down-sampled to a 1-bit B/W e-Paper buffer without dithering, anti-aliased edges turn into jagged, unreadable blobs—and a `120px` font in LVGL eats massive Flash and RAM unless explicitly configured as 1-bpp (`bpp: 1`).

### Peer Review by The Executor
1. **Strongest Response: B (combined with C’s execution plan).** B nails the exact byte-level math that makes or breaks this project on Day 1: an 800x480 screen on a WROOM-32 without PSRAM requires a 48KB 1-bpp display buffer *plus* a ~92KB RGB565 LVGL draw buffer (even at `buffer_size: 12%`) on an MCU with <80KB of free contiguous heap once Wi-Fi/API boot. B also correctly calls out that replacing `it.graph()` with an LVGL `line` widget forces you to write manual ring-buffer and coordinate-scaling C++ lambdas—defeating the entire point of moving to "declarative" LVGL. C turns B’s diagnosis into an immediate Monday-morning test plan.
2. **Biggest Blind Spot: Response A.** A is pure architectural fantasy. Proposing a "universal Ambient E-Ink Widget Engine" and "Chameleon Theme Engine" without noticing that 4 of the 6 active devices lack PSRAM—and will immediately OOM-boot-loop trying to allocate flex-grid LVGL containers at 800x480—wastes weeks building shared abstractions that cannot compile or run on the deployed hardware.
3. **What ALL Five Missed: Deep sleep / battery power vs. LVGL state loss, and the immediate bug fix.** None of the advisors noted that the button-cycling bug (`page_black`/`page_white` in `display: pages:`) can be fixed *today* in 2 minutes in existing C++ lambdas using a single boolean global (`id(is_repairing)`) inside the main page lambda—requiring zero LVGL migration. Furthermore, for battery-powered e-Paper devices (`coreink`, `trmnl`), LVGL loses all widget state and history arrays across deep-sleep wake cycles unless hydrated from `restore_value: yes` globals before the single `component.update` fires.

---

## 5. Chairman's Synthesis & Verdict

### Where the Council Agrees
1. **Do NOT Couple Display Driver Migration (`waveshare_epaper` $\rightarrow$ `epaper_spi`) with the LVGL Migration:**
   - Every advisor converged on this. LVGL sits on top of `display_eink` regardless of whether the underlying driver is `waveshare_epaper`, `crowpanel_epaper`, `it8951`, or `epaper_spi`. Changing drivers and rendering engines at the same time makes debugging blank screens impossible. Driver changes (if desired at all) belong in an isolated pre-work or post-work step.
2. **The `page_black` / `page_white` Button-Navigation Bug Has a Clean Overlay Solution:**
   - All 5 advisors agreed that `page_black` and `page_white` should never be navigable pages in `pages:`. In LVGL, they belong in a single full-screen `lvgl: top_layer:` scrubber overlay (`hidden: true` by default, revealed only during `repair_script`).
3. **Unify Plumbing (`features/epaper_lvgl.yaml`), Not Full-Screen Page Templates:**
   - Unlike the 20 LCD devices that all share the same 5 full-screen pages (`clock`, `temperature`, `nordpool`, `weather`, `status`), the e-Paper fleet has vastly different physical form factors (`200x200` cube, `792x272` ribbon, `480x800` portrait dashboard, `1872x1404` tablet) and physical bezel buttons. A monolithic `gui/epaper.yaml` with fixed pages is the wrong abstraction. Instead, share **plumbing** (refresh scheduler, `top_layer` repair scrubber, theme styles, optional bezel nav-hint bar) and **modular section cards** (e.g., Weather section, FolloRen waste section, Bus departures section, NordPool graph section, Device Info card).

### Where the Council Clashes
1. **Can the Non-PSRAM ESP32 Boards (`waveshare-display-board` 800x480, `waveshare-weather-station` 400x300, `m5s-coreink` 200x200) Run LVGL?**
   - **Contrarian, First Principles, Outsider & Executor** warn that 5 of the 6 active devices have **no PSRAM** (`ESP32-WROOM-32` / `ESP32-PICO-D4`), and `waveshare-display-board` (800x480 + BLE tracker + Wi-Fi + HA API) already uses 48KB for the 1-bit `display_eink` buffer. Allocating a standard `12%` RGB565 LVGL draw buffer (~92KB) will OOM-crash on boot.
   - **The Expansionist** (in peer review) counters with a crucial technical insight: because e-Paper is static (`update_interval: never`) and doesn't care about 60fps animation, ESPHome's LVGL component can be configured with **`buffer_size: 2%` or `4%`** (~15KB–30KB) and `bpp: 1` fonts! LVGL simply renders in more horizontal strips into `display_eink`'s existing 48KB RAM buffer before `component.update: display_eink` flushes to SPI.
   - **Resolution**: Don't guess—follow **The Executor's Day-1 Spike**. Test LVGL with `buffer_size: 4%` (and BLE proxy disabled if needed) on `m5s-coreink-2df58c` (200x200) and `waveshare-simen` (400x300) / `waveshare-display-board` (800x480) in Session 1.
2. **Should We Keep the "Every-5th-Refresh Inverted Dark Mode" (`recorded_display_refresh % 5 == 0`)?**
   - **First Principles & Outsider** argue that leaving an e-Paper screen inverted (black background, white text) for 5 minutes at a time (20% of the day) is visually jarring and actually *increases* electrophoretic charge trapping compared to relying on the hardware's native `full_update_every` LUT flash waveform.
   - **Expansionist & Executor** show that *if* you want to keep runtime color inversion (or use it for high-contrast alert cards), LVGL can do it cleanly in ~6 lines of YAML by defining 3 master styles (`style_epaper_bg`, `style_epaper_fg`, `style_epaper_accent`) and calling `lvgl.style.update` + `lvgl.widget.redraw` before `component.update: display_eink`.
3. **Why Are 8 Devices in a Drawer? (Bespoke Root Layouts vs. Reusable "Bento Cards"):**
   - **Outsider & Executor** say: ignore the 8 inactive devices and keep layouts inside the 4 active device YAMLs.
   - **Expansionist** points out *why* those 8 high-end S3+PSRAM devices (`seeed-trmnl-e1001/e1002/e1003`, `elecrow-crowpanel-42/579`, `zectrix-note4c`) are still on test pages: hand-calculating pixel coordinates (`it.printf(410, 227, ...)`) in C++ lambdas for 800x480, 792x272, and 1872x1404 screens is miserable! Extracting the "Weatherman" dashboard sections (Weather, FolloRen, Bus, Car, NordPool, Device Info) into reusable LVGL flex/grid section packages (`gui/epaper/sections/` or `gui/tiles/`) unlocks all 8 S3+PSRAM devices with minimal effort.

### Blind Spots the Council Caught (in Peer Review)
1. **LVGL Asynchronous Redraw vs. `component.update: display_eink` Race Condition (Contrarian):**
   - In ESPHome, `lvgl.label.update` or `lvgl.style.update` only marks LVGL objects as *dirty*; LVGL does not write pixels into `display_eink`'s buffer until its next `lv_timer_handler()` tick. If `script.update_screen` updates styles/labels and immediately calls `component.update: display_eink` in the same tick, the e-Paper screen will flush the **old** buffer (one cycle behind)!
   - **Fix**: `script.update_screen` must trigger `lvgl.widget.redraw`, wait `100ms` (so LVGL's timer flushes all strips to `display_eink`), and *then* call `component.update: display_eink`.
2. **1-Bit Font Rendering (`bpp: 1`) on Monochrome E-Paper (Contrarian & Outsider):**
   - Standard `gui/fonts/main.yaml` uses `bpp: 4` (16-level anti-aliasing). On 1-bit B/W e-Paper (`coreink`, `waveshare`, `e1001`, `crowpanel`), 4-bpp anti-aliased fonts down-converted from RGB565 without grayscale support produce ragged edges and use 4× the Flash/RAM. Monochrome e-Paper should use `bpp: 1` fonts (while `e1003` 16-grayscale can use `bpp: 4`).
3. **Replacing `it.graph(...)` for Local/HA Temperature History (`waveshare-weather-station`, `m5s-air-quality`):**
   - NordPool already has an LVGL `line` graph implementation (`gui/pages/nordpool/sensors.yaml`) because `sensor.waveshare_nordpool_future_offset` provides 8 hourly attributes (`price0`..`price8`).
   - However, `waveshare-weather-station` (`graph_ute`, `graph_foran`, `graph_bak`) and `m5s-air-quality` (`graph_temperature_room`) use ESPHome's `graph:` component to record rolling 8h–12h history in RAM. To migrate those to LVGL without writing complex C++ ring buffers on every ESP32, we should create a single reusable `gui/epaper/widgets/rolling_graph.yaml` package (or feed an array attribute from Home Assistant).

### The Recommendation
Adopt a **Two-Tier, Capability-Aware E-Paper LVGL Architecture** under `gui/epaper/` and execute in **6 focused sessions**:
1. **Keep Existing Display Drivers Intact During LVGL Migration**: Do not mix `waveshare_epaper` $\rightarrow$ `epaper_spi` changes with LVGL layout work.
2. **Build `gui/epaper/` as a Modular Toolkit (Not a Monolithic Fixed-Page App)**:
   - `gui/epaper/base.yaml`: Replaces `features/epaper.yaml` for LVGL devices. Configures `lvgl:` with low-RAM `buffer_size` (e.g., `4%` on non-PSRAM, `25%` on PSRAM), `top_layer` black/white repair scrubber overlay, the `update_screen` script (with the `100ms` LVGL flush delay before `component.update: display_eink`), and master styles (`style_epaper_bg`, `style_epaper_fg`, `style_epaper_accent1..3`).
   - `gui/epaper/themes/`: Color palettes for `bw.yaml`, `bwry4.yaml` (Zectrix), `spectra6.yaml` (E1002), and `gray16.yaml` (E1003), with optional 5th-refresh inversion controlled by a simple substitution (`epaper_invert_every_5th: "true" | "false"`).
   - `gui/epaper/chrome/`: Optional physical-button icon hint bars (`nav_coreink.yaml`, `nav_reterminal.yaml`, `nav_crowpanel.yaml`) and status footer (`wifi`, `ha`, `@ HH:MM` timestamp).
   - `gui/epaper/pages/info.yaml`: Standard, responsive e-Paper Device Info page (Name, SSID, IP, MAC, Battery, Button hints).
   - `gui/epaper/sections/`: Reusable Bento-grid sections (`weather_5slot.yaml`, `nordpool_graph.yaml`, `folloren.yaml`, `bus_departures.yaml`, `car_status.yaml`, `rolling_graph.yaml`) so `waveshare-display-board`, `seeed-trmnl-e1001/e1002/e1003`, `zectrix-note4c`, and `elecrow-crowpanel-579` can compose dashboards declaratively without hardcoded pixel coordinates.

### The One Thing to Do First
Before building shared e-Paper LVGL packages, **answer the 5 design questions in `EPAPER.md` and run Session 1 (Baseline `host-sdl` C++ Lambda Snapshots + Non-PSRAM Heap Spike on `m5s-coreink-2df58c` and `waveshare-simen`)** to prove the exact `buffer_size` and `bpp: 1` font footprint on non-PSRAM ESP32 hardware.
