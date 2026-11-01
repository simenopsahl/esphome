#!/usr/bin/env python3
"""Download compiled firmware.factory.bin and firmware.elf from the pve-esphome File Browser.

Usage:
    python tools/fetch_firmware.py <device_name> [remote_short_name] [build_id]

Examples:
    python tools/fetch_firmware.py waveshare-audio-board-b2b9d0
    python tools/fetch_firmware.py m5s-atom-s3r-0387f4 m5s-as3r-0387f4
"""

from pathlib import Path
import re
import sys
import urllib.request

FILEBROWSER_BASE = "http://192.168.68.110:8080"
DEFAULT_BUILD_ID = "XId1jk5h"
LOCAL_BUILD_ROOT = Path(r"C:\Users\simen\.esphome_build")
REPO_ROOT = Path(__file__).resolve().parent.parent


def resolve_short_name(device_name: str) -> str:
    """Resolve `device.name` from <device_name>.yaml if it differs from the filename."""
    yaml_path = REPO_ROOT / f"{device_name}.yaml"
    if yaml_path.is_file():
        text = yaml_path.read_text(encoding="utf-8", errors="replace")
        # Match `device:\n    name: <short_name>` under substitutions
        m = re.search(r"^\s*device:\s*\r?\n(?:\s+.*\r?\n)*?\s+name:\s*([^\s#]+)", text, re.MULTILINE)
        if m:
            return m.group(1).strip("'\"")
    return device_name


def login() -> str:
    req = urllib.request.Request(
        f"{FILEBROWSER_BASE}/api/login",
        data=b"{}",
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=15) as resp:
        return resp.read().decode("utf-8").strip().strip('"')


def download_file(token: str, remote_url: str, dest: Path) -> None:
    req = urllib.request.Request(remote_url, headers={"X-Auth": token}, method="GET")
    with urllib.request.urlopen(req, timeout=120) as resp:
        data = resp.read()
    dest.write_bytes(data)
    print(f"Downloaded {dest.name}: {len(data):,} bytes -> {dest}")


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__.strip())
        return 1

    device_name = sys.argv[1].removesuffix(".yaml")
    short_name = sys.argv[2] if len(sys.argv) >= 3 else resolve_short_name(device_name)
    build_id = sys.argv[3] if len(sys.argv) >= 4 else DEFAULT_BUILD_ID

    out_dir = LOCAL_BUILD_ROOT / device_name
    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"Fetching build artifacts for '{device_name}' (remote short_name='{short_name}', build_id='{build_id}')...")
    token = login()
    base_url = (
        f"{FILEBROWSER_BASE}/api/raw/root/config/.esphome/.remote_builds/"
        f"{build_id}/.esphome/build/{short_name}/build"
    )

    for artifact in ("firmware.factory.bin", "firmware.elf"):
        download_file(token, f"{base_url}/{artifact}", out_dir / artifact)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

