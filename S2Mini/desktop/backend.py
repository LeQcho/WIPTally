"""Local configuration and flash jobs. No network credentials are logged or saved."""
import csv
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import threading
import time
from serial.tools import list_ports
from .usb_flash import AFTER_WRITE, restart_message, run_write, write_verified

ROOT = Path(__file__).resolve().parents[1]
ROLES = {"master", "receiver", "receiver-rgb", "sony-c3"}
RGB_LEVELS = {"25": 64, "50": 130, "75": 191, "100": 255}


def chip_for(role):
    return "esp32c3" if role == "sony-c3" else "esp32s2"


def integer(value, name):
    if isinstance(value, bool) or not str(value).isascii() or not str(value).isdigit():
        raise ValueError(f"{name} : saisir un entier de 1 à 40.")
    number = int(value)
    if not 1 <= number <= 40:
        raise ValueError(f"{name} : choisir une valeur de 1 à 40.")
    return number


def validate(data):
    role = data.get("role")
    if role not in ROLES:
        raise ValueError("Choisir Master, Récepteur ou Tally Sony.")
    result = {"role": role, "port": str(data.get("port", ""))}
    if role == "sony-c3":
        if data.get("board") != "lolin_c3_mini":
            raise ValueError("Sony nécessite une LOLIN C3 Mini Bluetooth LE. La S2 Mini est incompatible.")
        name = data.get("sony_name", "ILCE-7M4")
        target = data.get("sony_mac", "")
        if not isinstance(name, str) or not name.strip() or len(name.encode()) > 32 or any(ord(x) < 32 for x in name):
            raise ValueError("Le nom Bluetooth Sony doit contenir de 1 à 32 octets.")
        if not isinstance(target, str) or (target and not re.fullmatch(r"(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}", target)):
            raise ValueError("Adresse Bluetooth invalide : utiliser le format AA:BB:CC:DD:EE:FF, ou laisser vide.")
        pin = integer(data.get("sony_pin", 4), "GPIO des LED")
        if pin not in {3, 4, 5, 6, 7, 10}:
            raise ValueError("GPIO Sony : choisir 3, 4, 5, 6, 7 ou 10. GPIO18 de la S2 Mini ne convient pas.")
        count = integer(data.get("sony_pixels", 2), "Nombre de LED")
        if count > 16:
            raise ValueError("Choisir de 1 à 16 LED pour Sony.")
        result.update(board="lolin_c3_mini", sony_name=name.strip(), sony_mac=target.lower(), sony_pin=pin, sony_pixels=count)
        result["rgb_order"] = data.get("rgb_order", "RGB")
        if result["rgb_order"] not in {"RGB", "GRB"}:
            raise ValueError("Choisir l'ordre des couleurs RGB ou GRB.")
    elif role == "master":
        ssid, password = data.get("ssid", ""), data.get("password", "")
        if not password and data.get("open_wifi") is not True:
            raise ValueError("Saisir le mot de passe Wi-Fi. Cocher Réseau ouvert uniquement si le réseau n'a réellement pas de mot de passe.")
        if not isinstance(ssid, str) or not ssid.strip() or len(ssid.encode("utf-8", errors="replace")) > 32 or any(ord(x)<32 for x in ssid):
            raise ValueError("Le nom du Wi-Fi doit contenir de 1 à 32 octets UTF-8.")
        if not isinstance(password, str) or any(ord(x)<32 for x in password) or (password and not 8 <= len(password.encode("utf-8", errors="replace")) <= 63):
            raise ValueError("Le mot de passe Wi-Fi doit contenir de 8 à 63 octets, ou être vide pour un réseau ouvert.")
        try:
            address = ipaddress.IPv4Address(data.get("atem", ""))
        except ipaddress.AddressValueError:
            raise ValueError("Saisir une adresse IPv4 valide pour l'ATEM.") from None
        if address.is_unspecified or address.is_multicast or int(address) == 0xffffffff:
            raise ValueError("L'adresse ATEM doit désigner un appareil du réseau local.")
        result.update(ssid=ssid, password=password, atem=str(address), inputs=integer(data.get("inputs", 8), "Nombre de caméras"))
        level = RGB_LEVELS.get(str(data.get("brightness", "50")))
        if level is None:
            raise ValueError("Choisir une luminosité de départ de 25, 50, 75 ou 100 %.")
        result["rgb_level"] = level
    else:
        result["camera"] = integer(data.get("camera", 1), "Numéro de caméra")
        result["rgb_order"] = data.get("rgb_order", "RGB")
        if result["rgb_order"] not in ("RGB", "GRB"):
            raise ValueError("Choisir l'ordre des couleurs RGB ou GRB.")
        if role == "receiver-rgb":
            level = RGB_LEVELS.get(str(data.get("rgb_brightness", "50")))
            if level is None:
                raise ValueError("Choisir une luminosité de 25, 50, 75 ou 100 %.")
            result["rgb_level"] = level
    return result


def settings_rows(config):
    if config["role"] == "sony-c3":
        return [("tally-sony", "namespace", "", ""),
                ("name", "data", "string", config["sony_name"]),
                ("target", "data", "string", config["sony_mac"]),
                ("pin", "data", "u8", config["sony_pin"]),
                ("pixels", "data", "u8", config["sony_pixels"]),
                ("level", "data", "u8", 20),
                ("rgb_order", "data", "u8", int(config["rgb_order"] == "RGB"))]
    if config["role"] == "master":
        return [("tally-master", "namespace", "", ""),
                ("ssid", "data", "string", config["ssid"]),
                ("password", "data", "string", config["password"]),
                ("atem", "data", "string", config["atem"]),
                ("inputs", "data", "u8", config["inputs"]),
                ("rgb_level", "data", "u8", config["rgb_level"]),
                ("demo", "data", "u8", 0)]
    rows = [("tally-rx", "namespace", "", ""), ("camera", "data", "u8", config["camera"])]
    if config["role"] == "receiver-rgb":
        rows.append(("rgb_order", "data", "u8", int(config["rgb_order"] == "RGB")))
        rows.append(("rgb_level", "data", "u8", config["rgb_level"]))
    return rows


def make_settings(config, directory):
    source, target = directory / "settings.csv", directory / "settings.bin"
    with source.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["key", "type", "encoding", "value"])
        writer.writerows(settings_rows(config))
    if os.name != "nt":
        source.chmod(0o600)
    run = subprocess.run([sys.executable, str(ROOT / "desktop/vendor/nvs_partition_gen.py"),
                          "generate", str(source), str(target), "0x5000"],
                         capture_output=True, text=True, timeout=30)
    if run.returncode or not target.is_file() or target.stat().st_size != 0x5000:
        # Generator failures might include input data. Never relay them to the UI.
        raise ValueError("Impossible de préparer les réglages. Relancer l'application pour vérifier ses dépendances.")
    if os.name != "nt":
        target.chmod(0o600)
    return target


def images(role):
    if role not in ROLES:
        raise ValueError("Profil de firmware inconnu.")
    manifest = json.loads((ROOT / "firmware/manifest.json").read_text())
    result = []
    for address, name in [("0x0" if role == "sony-c3" else "0x1000", f"{role}/bootloader.bin"), ("0x8000", f"{role}/partitions.bin"),
                          ("0xe000", "boot_app0.bin"), ("0x10000", f"{role}/firmware.bin")]:
        path = ROOT / "firmware" / name
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != manifest["sha256"].get(name):
            raise ValueError(f"Firmware manquant ou modifié : {name}. Reprendre le kit complet.")
        result.append((address, path))
    return result


def ports(simulate=False, role="master"):
    if simulate:
        board = "C3 Mini" if role == "sony-c3" else "S2 Mini"
        return [{"device": "SIMULATION", "description": f"{board} de démonstration · aucune écriture USB"}]
    supported = {0x1001} if role == "sony-c3" else {2, 0x80c2}
    return [{"device": p.device, "description": p.description} for p in list_ports.comports()
            if p.vid == 0x303a and p.pid in supported]


class FlashJobs:
    def __init__(self, simulate=False):
        self.simulate = simulate
        self.lock = threading.Lock()
        self.job = {"status": "idle", "lines": [], "message": "Prêt à préparer une carte."}

    def snapshot(self):
        with self.lock:
            return {**self.job, "lines": list(self.job["lines"])}

    def log(self, line):
        with self.lock:
            self.job["lines"] = (self.job["lines"] + [line])[-350:]

    def start(self, data, check_only=False):
        config = validate(data)
        if not check_only and config["port"] not in {p["device"] for p in ports(self.simulate, config["role"])}:
            raise ValueError("Brancher une LOLIN C3 Mini et sélectionner son port USB (S2 incompatible)." if config["role"] == "sony-c3" else "Brancher une S2 Mini et sélectionner son port USB.")
        images(config["role"])
        with self.lock:
            if self.job["status"] == "running":
                raise ValueError("Une opération est déjà en cours sur la carte.")
            self.job = {"status": "running", "lines": [], "message": "Préparation des réglages…",
                        "check_only": check_only, "role": config["role"]}
        threading.Thread(target=self._run, args=(config, check_only), daemon=True).start()

    def _run(self, config, check_only):
        try:
            with tempfile.TemporaryDirectory(prefix="tally-config-") as directory:
                settings = make_settings(config, Path(directory))
                self.log("Réglages préparés. Sommes de contrôle des firmwares vérifiées.")
                if config["role"] == "sony-c3":
                    self.log("Sony REC expérimental · LOLIN C3 Mini uniquement. Validation sur caméra encore nécessaire.")
                    self.log("Le flash réinitialise l'appairage Sony ; appairer la caméra après le redémarrage.")
                if check_only:
                    self.log("Vérification réussie : aucune écriture USB.")
                elif self.simulate:
                    for line in ["SIMULATION : connexion à la carte cible…", "SIMULATION : écriture du firmware et des réglages…", "SIMULATION : données vérifiées."]:
                        self.log(line)
                        time.sleep(0.15)
                else:
                    flash_message = self._flash(config, settings)
            # Temporary CSV and binary, including the Wi-Fi password, are removed.
            with self.lock:
                self.job.update(status="success", message="Vérification réussie — aucune écriture USB." if check_only else
                                "Simulation terminée — aucune écriture USB." if self.simulate else
                                flash_message)
        except Exception as exc:
            # Show only controlled messages; never stringify arbitrary generator data.
            message = str(exc) if isinstance(exc, ValueError) else "Opération interrompue. Vérifier le câble USB et relancer."
            self.log(message)
            with self.lock:
                self.job.update(status="error", message=message)

    def _flash(self, config, settings):
        chip, device = chip_for(config["role"]), config["port"]
        original = next((port for port in list_ports.comports() if port.device == device), None)
        base = [sys.executable, "-u", "-m", "esptool", "--chip", chip, "--port", device,
                "--baud", "460800", "--before", "default_reset", *AFTER_WRITE,
                "write_flash", "-z", "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "4MB"]
        firmware = images(config["role"])
        firmware.insert(2, ("0x9000", settings))
        command = base + [str(value) for pair in firmware for value in pair]
        for attempt in range(2):
            code, output = run_write(command, self.log)
            if write_verified(code, output, len(firmware)):
                return restart_message(device, original, chip, code, self.log)
            wrote = any("Writing at" in line or "Flash will be erased" in line for line in output)
            # Native USB re-enumerates on the first reset. Retry only before any
            # erase/write and only if the originally selected port is still here.
            usb_transition = any("Device not configured" in x or "Could not configure port" in x for x in output)
            if attempt == 0 and not wrote and usb_transition:
                self.log("La carte change de mode USB ; nouvelle connexion…")
                for _ in range(15):
                    if config["port"] in {p["device"] for p in ports(role=config["role"])}:
                        break
                    time.sleep(.3)
                else:
                    raise ValueError("La carte n'est pas réapparue sur le port choisi. Actualiser les ports et relancer.")
                continue
            raise ValueError("Flash non terminé. Rebrancher la carte et réessayer. Si nécessaire, la placer en mode téléchargement avec BOOT et RST.")
