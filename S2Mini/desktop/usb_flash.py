"""Shared write and USB restart handling for Atelier and the direct flash tools."""
import subprocess
import threading
import time

from serial.tools import list_ports


AFTER_WRITE = ("--after", "watchdog_reset")
POWER_CYCLE = "Flash vérifié. Coupe puis rétablis l’alimentation USB de la carte pour la démarrer."


def run_write(command, log):
    output = []
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True, encoding="utf-8", errors="replace") as process:
        timer = threading.Timer(180, process.kill)
        timer.start()
        try:
            for line in process.stdout:
                line = line.rstrip()
                output.append(line)
                log(line)
            return process.wait(), output
        finally:
            timer.cancel()


def write_verified(exit_code, output, image_count):
    # USB can disappear during the watchdog reset after every image was verified.
    return exit_code == 0 or (
        "Hard resetting with a watchdog..." in output
        and output.count("Hash of data verified.") == image_count
    )


def same_usb_device(candidate, device, original):
    # Some OSes append the interface number to the physical USB location.
    original_location = (getattr(original, "location", None) or "").split(":", 1)[0]
    location = (getattr(candidate, "location", None) or "").split(":", 1)[0]
    if original_location and location:
        return location == original_location
    original_serial = (getattr(original, "serial_number", None) or "").replace(":", "").lower()
    serial = (getattr(candidate, "serial_number", None) or "").replace(":", "").lower()
    if original_serial and serial:
        return serial == original_serial
    return candidate.device == device


def restart_message(device, original, chip, exit_code, log):
    if chip == "esp32c3":
        # The C3 USB-Serial/JTAG PID is shared by the ROM and the application.
        return "Flash vérifié. Commande de redémarrage envoyée." if exit_code == 0 else POWER_CYCLE
    for _ in range(32):
        try:
            ports = list_ports.comports()
        except OSError:
            ports = ()
        for port in ports:
            if port.vid == 0x303a and port.pid == 0x80c2 and same_usb_device(port, device, original):
                log(f"Carte de retour en mode application USB sur {port.device}.")
                return "Flash vérifié. Carte redémarrée automatiquement."
        time.sleep(.25)
    return POWER_CYCLE
