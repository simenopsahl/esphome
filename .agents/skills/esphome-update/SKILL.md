---
name: esphome-update
description: >-
  Use this skill to check, upgrade, or manage the local Python ESPHome CLI installation (both Windows host and WSL) and verify device configs post-update.
---

# ESPHome CLI Update & Synchronization

This skill defines the procedure for inspecting, upgrading, and verifying the local ESPHome CLI environments (Windows and WSL) so that they remain in lockstep with Home Assistant and remote build servers.

## When to Run

Run this procedure whenever:
- Home Assistant or your remote build server has updated to a newer ESPHome release (e.g. `2026.10.0`).
- You encounter syntax schema differences or new features that are not yet recognized by your local CLI.
- You need to pin or downgrade the local ESPHome version for troubleshooting.

---

## Environment Details

There are two local ESPHome installations on this machine:

### 1. Windows Host (Native)
- Executable: `C:\Users\simen\AppData\Local\Programs\Python\Python312\Scripts\esphome.exe`
- Package Manager: `python -m pip`
- Invocation: `esphome <command>`

### 2. WSL Ubuntu Environment
- Virtualenv Directory: `/home/simen/virtualesphome`
- Executable: `/home/simen/virtualesphome/bin/esphome`
- Package Manager: `/home/simen/virtualesphome/bin/pip`
- Invocation from PowerShell:
  ```powershell
  wsl --cd ~ -d Ubuntu bash -c "/home/simen/virtualesphome/bin/esphome <command>"
  ```
- Invocation inside WSL bash:
  ```bash
  source ~/virtualesphome/bin/activate
  esphome <command>
  ```

---

## Step 1: Check Current and Available Versions

### Windows Host
```powershell
esphome version
python -m pip index versions esphome
```

### WSL Ubuntu
```powershell
wsl --cd ~ -d Ubuntu bash -c "/home/simen/virtualesphome/bin/esphome version"
```

---

## Step 2: Upgrade ESPHome

### On Windows Host
Pin to target version:
```powershell
python -m pip install --upgrade esphome==2026.10.0
```
Or upgrade to latest:
```powershell
python -m pip install --upgrade esphome
```

### On WSL Ubuntu
Pin to target version:
```powershell
wsl --cd ~ -d Ubuntu bash -c "/home/simen/virtualesphome/bin/pip install --upgrade esphome==2026.10.0"
```
Or upgrade to latest:
```powershell
wsl --cd ~ -d Ubuntu bash -c "/home/simen/virtualesphome/bin/pip install --upgrade esphome"
```

---

## Step 3: Verify Upgrade

### Windows Host
```powershell
esphome version
```

### WSL Ubuntu
```powershell
wsl --cd ~ -d Ubuntu bash -c "/home/simen/virtualesphome/bin/esphome version"
```

---

## Step 4: Post-Update Validation & Cache Clearing

After updating ESPHome across minor or major versions:

1. **Verify device configurations**:
   Run `esphome config` against active device nodes to detect any breaking schema changes or deprecations:
   ```powershell
   esphome config m5s-fire-a70780.yaml
   ```

2. **Clean build cache if compile issues occur**:
   If platform or component internal structures changed between ESPHome versions, clean stale PlatformIO artifacts:
   ```powershell
   esphome clean <device-config.yaml>
   ```
