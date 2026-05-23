#!/usr/bin/env python3
"""
tools/flash_fs.py — Flash the FAT16 filesystem image to the ESP32

Finds PlatformIO's bundled esptool automatically so you don't need
esptool installed globally.

Usage:
  python tools/flash_fs.py            # uses default port from platformio.ini
  python tools/flash_fs.py --port COM7

Run build_fs.py first if you've changed any scripts:
  python tools/build_fs.py && python tools/flash_fs.py
"""

import os
import sys
import glob
import subprocess
import argparse

# ── Configuration ─────────────────────────────────────────────────────────────
FLASH_OFFSET = "0x600000"
BAUD_RATE    = "921600"
CHIP         = "esp32s3"
DEFAULT_PORT = "COM7"

# ─────────────────────────────────────────────────────────────────────────────
def find_esptool() -> str:
    """Locate esptool.py inside PlatformIO's package cache."""
    pio_home = os.path.expanduser("~/.platformio")
    patterns = [
        os.path.join(pio_home, "packages", "tool-esptoolpy", "esptool.py"),
    ]
    for p in patterns:
        if os.path.isfile(p):
            return p
    # Broad search
    results = glob.glob(os.path.join(pio_home, "packages", "**", "esptool.py"),
                        recursive=True)
    if results:
        return results[0]
    raise FileNotFoundError(
        "esptool.py not found in ~/.platformio/packages/\n"
        "Run 'pio pkg install' first."
    )


def find_python() -> str:
    """Use PlatformIO's own Python so esptool deps are available."""
    pio_python = os.path.expanduser("~/.platformio/penv/Scripts/python.exe")
    if os.path.isfile(pio_python):
        return pio_python
    return sys.executable


def main():
    parser = argparse.ArgumentParser(description="Flash FAT16 image to ESP32")
    parser.add_argument("--port", default=DEFAULT_PORT,
                        help=f"Serial port (default: {DEFAULT_PORT})")
    args = parser.parse_args()

    root      = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    image     = os.path.join(root, "tools", "fat16.bin")

    if not os.path.isfile(image):
        print(f"ERROR: {image} not found.")
        print("Run 'python tools/build_fs.py' first.")
        sys.exit(1)

    python   = find_python()
    esptool  = find_esptool()

    cmd = [
        python, esptool,
        "--chip",  CHIP,
        "--port",  args.port,
        "--baud",  BAUD_RATE,
        "write_flash", FLASH_OFFSET, image,
    ]

    print(f"Flashing {image} ({os.path.getsize(image):,} bytes)")
    print(f"  Port   : {args.port}")
    print(f"  Offset : {FLASH_OFFSET}")
    print()
    print(" ".join(cmd))
    print()

    result = subprocess.run(cmd)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()