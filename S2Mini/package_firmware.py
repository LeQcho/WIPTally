#!/usr/bin/env python3
"""Run after `pio run`; copies the exact images needed by flash.py."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil

root = Path(__file__).resolve().parent
core = Path(os.environ.get("PLATFORMIO_CORE_DIR", str(Path.home() / ".platformio")))
destination = root / "firmware"
destination.mkdir(exist_ok=True)
parser = argparse.ArgumentParser()
parser.add_argument("--roles", nargs="+", choices=("master", "receiver", "receiver-rgb", "sony-c3"), default=("master", "receiver", "receiver-rgb", "sony-c3"))
roles = parser.parse_args().roles
for role in roles:
    (destination / role).mkdir(exist_ok=True)
    for name in ("bootloader.bin", "partitions.bin", "firmware.bin"):
        shutil.copy2(root / ".pio" / "build" / role / name, destination / role / name)
shutil.copy2(core / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin", destination / "boot_app0.bin")
digest = {str(f.relative_to(destination)): hashlib.sha256(f.read_bytes()).hexdigest()
          for f in sorted(destination.rglob("*.bin"))}
source_digest = {str(f.relative_to(root)): hashlib.sha256(f.read_bytes()).hexdigest()
                 for f in sorted([*root.glob("src/*.cpp"), *root.glob("include/*.h"),
                                  *root.glob("lib/**/*.cpp"), *root.glob("lib/**/*.h"),
                                  *root.glob("lib/**/*.c"), *root.glob("lib/**/*.S"), root / "platformio.ini"])}
manifest_path = destination / "manifest.json"
manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
manifest.update({
    "board": "lolin_s2_mini", "chip": "esp32s2", "platform": "espressif32@6.12.0",
    "arduino": "2.0.17", "esptool": "4.9.0", "sha256": digest, "source_sha256": source_digest
})
manifest["profiles"] = {
    role: {"board": "lolin_c3_mini" if role == "sony-c3" else "lolin_s2_mini",
           "chip": "esp32c3" if role == "sony-c3" else "esp32s2",
           "experimental": role == "sony-c3"} for role in ("master", "receiver", "receiver-rgb", "sony-c3")
}
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
print("Firmware package ready:", destination)
