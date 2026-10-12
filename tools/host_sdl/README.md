# ESPHome Host SDL Virtual Device

Simulate and preview modular LVGL configurations locally on your desktop using ESPHome's `host` platform and an SDL2 window — without flashing physical hardware.

---

## 1. Overview & Architecture

Instead of maintaining a separate divergent branch, the virtual device runs directly on `main` and imports the repository's shared modular packages:

```text
host-sdl.yaml
  ├── common/               (api, logger, restart, status, time)
  ├── board/host_sdl.yaml   (SDL display, touchscreen, keyboard keys, virtual backlight)
  ├── board/module/         (host_simulated_env.yaml - drifting test sensor data)
  ├── features/             (combined_env_sensors.yaml)
  ├── gui/${gui_layout}.yaml(LVGL theme, styles, pages, widgets, navigation)
  └── components/           (lvgl_screenshot - HTTP screenshot & page navigation API)
```

- **SDL Display (`display_sdl`)**: Opens a native desktop window via WSLg (Windows Subsystem for Linux GUI).
- **Touchscreen (`touchscreen_sdl`)**: Mouse clicks and drags act as capacitive touch gestures (swiping left/right changes pages).
- **Keyboard Buttons**: Left Arrow = Button A (Previous), Down Arrow = Button B (Home), Right Arrow = Button C (Next).
- **Screenshot HTTP Server (Port 8080)**: Serves full RGB framebuffer PNGs (`GET /screenshot`) and controls pages (`GET /page/next`, `/page/prev`, `/page/<index>`).
- **ESPHome Native API (Port 6053)**: Exposes full ESPHome API to Home Assistant.

---

## 2. Quick Start (Windows / PowerShell)

Run these commands from the repository root in PowerShell (`pwsh`):

### Run the Default Display Layout (320x240)
```powershell
pwsh tools/host_sdl/host-sdl.ps1 run
```

### Run Any Specific Layout
Pass the layout filename (from `gui/<layout>.yaml`):
```powershell
pwsh tools/host_sdl/host-sdl.ps1 run 240x320
pwsh tools/host_sdl/host-sdl.ps1 run 480x320
pwsh tools/host_sdl/host-sdl.ps1 run 128x128
```

### Validate Configuration Without Compiling
```powershell
pwsh tools/host_sdl/host-sdl.ps1 config
```

### Take a Screenshot
Saves a timestamped PNG into `tools/host_sdl/screenshots/`:
```powershell
pwsh tools/host_sdl/host-sdl.ps1 screenshot
```

### Navigate Pages Remotely
```powershell
pwsh tools/host_sdl/host-sdl.ps1 page next
pwsh tools/host_sdl/host-sdl.ps1 page prev
pwsh tools/host_sdl/host-sdl.ps1 page 0
```

### Stop the Virtual Device
```powershell
pwsh tools/host_sdl/host-sdl.ps1 stop
```

---

## 3. Connecting to Home Assistant

The virtual device can communicate directly with your Home Assistant server (`192.168.68.100`) as an ESPHome node.

### Option A: WSL2 Mirrored Networking (Recommended for Windows 11)
Windows 11 supports mirrored networking, where WSL2 shares your desktop's LAN IP (`192.168.68.61`) directly:

1. Create or edit `%USERPROFILE%\.wslconfig` in Windows:
   ```ini
   [wsl2]
   networkingMode=mirrored
   ```
2. Restart WSL:
   ```powershell
   wsl --shutdown
   ```
3. Run the virtual device:
   ```powershell
   pwsh tools/host_sdl/host-sdl.ps1 run
   ```
4. In Home Assistant:
   - Navigate to **Settings -> Devices & Services -> Integrations**.
   - Click **Add Integration** -> search for **ESPHome**.
   - Host: `192.168.68.61`, Port: `6053`.
   - When prompted for the encryption key, enter the key from `secrets.yaml` under `homeassistant_api`.

Once connected:
- Home Assistant immediately imports all entities.
- Home Assistant pushes live states for `sensor.etg1_kjokken_temperature`, occupancy binary sensors, weather, NordPool prices, etc. directly onto the virtual screen!

### Option B: Windows Port Forwarding (If using WSL2 NAT mode)
If running WSL2 in default NAT mode, forward port 6053 in an Administrator PowerShell:
```powershell
netsh interface portproxy add v4tov4 listenport=6053 listenaddress=0.0.0.0 connectport=6053 connectaddress=localhost
```
Then add the device in Home Assistant pointing to `192.168.68.61`.

