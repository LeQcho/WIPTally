#!/usr/bin/env python3
"""Flash prebuilt LOLIN S2 Mini images; no source compilation required."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("role", choices=["master", "receiver", "list"])
    parser.add_argument("--port", help="e.g. /dev/cu.usbmodem01 or COM5")
    parser.add_argument("--dry-run", action="store_true", help="Verify image checksums without flashing")
    args = parser.parse_args()
    try:
        from serial.tools import list_ports
        version = importlib.metadata.version("esptool")
    except (ImportError, importlib.metadata.PackageNotFoundError):
        parser.error("Installer d'abord les dependances : python -m pip install -r requirements-flash.txt")
    if version != "4.9.0":
        parser.error("Version esptool requise : 4.9.0 (voir requirements-flash.txt)")
    ports = list(list_ports.comports())
    if args.role == "list":
        for port in ports:
            print(f"{port.device}: {port.description} (VID={port.vid}, PID={port.pid})")
        return 0
    port = args.port
    if not port:
        candidates = [p.device for p in ports if p.vid == 0x303A]
        if len(candidates) != 1:
            parser.error("Brancher une seule S2 Mini en USB, ou preciser --port. Utiliser 'list' pour voir les ports.")
        port = candidates[0]
    manifest = json.loads((ROOT / "firmware" / "manifest.json").read_text())
    images = [("0x1000", f"{args.role}/bootloader.bin"),
              ("0x8000", f"{args.role}/partitions.bin"),
              ("0xe000", "boot_app0.bin"),
              ("0x10000", f"{args.role}/firmware.bin")]
    personal_settings = args.role == "master" and (ROOT / "firmware" / "network-settings.bin").exists()
    if personal_settings:
        if (ROOT / "firmware" / "network-settings.bin").stat().st_size != 0x5000:
            parser.error("Taille de partition NVS personnelle incorrecte")
        images.insert(2, ("0x9000", "network-settings.bin"))
    command = [sys.executable, "-m", "esptool", "--chip", "esp32s2", "--port", port,
               "--baud", "460800", "--before", "default_reset", "--after", "hard_reset",
               "write_flash", "-z", "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "4MB"]
    for address, relative in images:
        file = ROOT / "firmware" / relative
        if hashlib.sha256(file.read_bytes()).hexdigest() != manifest["sha256"][relative]:
            parser.error(f"Fichier altere : {relative}. Reprendre le paquet complet.")
        command.extend([address, str(file)])
    settings_message = "Configuration reseau du kit personnel appliquee." if personal_settings else "Les parametres NVS existants sont conserves."
    print(f"Flash {args.role} sur {port}. {settings_message}", flush=True)
    if args.dry_run:
        print("DRY RUN : images verifiees, aucune ecriture USB.")
        print(command)
        return 0
    return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    sys.exit(main())
