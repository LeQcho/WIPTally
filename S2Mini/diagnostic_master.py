#!/usr/bin/env python3
"""Capture le journal du master sans modifier son firmware ni sa configuration."""
from pathlib import Path
import time
import serial
from serial.tools import list_ports

log_path = Path(__file__).resolve().parent / "diagnostic-master.log"
with log_path.open("w", encoding="utf-8") as log:
    def report(message):
        print(message, flush=True)
        log.write(message + "\n")
        log.flush()

    report("Diagnostic master : lecture USB pendant 60 secondes.")
    report("Tu peux laisser la carte fonctionner normalement.")
    report("Aucune modification du firmware ou des reglages.")
    deadline = time.monotonic() + 60
    connection = None
    messages = 0
    last_error = ""
    while time.monotonic() < deadline:
        if connection is None:
            ports = [p for p in list_ports.comports() if p.vid == 0x303A]
            if len(ports) != 1:
                error = "Brancher une seule carte ESP32-S2 en USB."
                if error != last_error:
                    report(error)
                    last_error = error
                time.sleep(0.3)
                continue
            try:
                connection = serial.Serial(ports[0].device, 115200, timeout=0.3)
                report(f"Port ouvert : {ports[0].device} ({ports[0].description})")
            except serial.SerialException as exc:
                error = str(exc)
                if error != last_error:
                    report("Ouverture impossible : " + error)
                    last_error = error
                time.sleep(0.3)
                continue
        try:
            line = connection.readline()
            if line:
                report(line.decode(errors="replace").strip())
                messages += 1
        except serial.SerialException:
            report("USB deconnecte ; attente du redemarrage de la carte...")
            connection.close()
            connection = None
    if connection is not None:
        connection.close()
    if not messages:
        report("Aucun message recu pendant la capture.")
    report("Capture terminee. Fichier : " + str(log_path))
