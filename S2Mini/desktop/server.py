"""A loopback-only UI for configuring and flashing S2 Mini boards."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import secrets
import threading
from urllib.request import urlopen, Request
import hashlib
import time
import socket
import webbrowser
from .backend import ROOT, FlashJobs, ports
VERSION = "1.4.0"
INSTALLATION = hashlib.sha256(str(ROOT).encode()).hexdigest()[:12]


def create_server(port=0, simulate=False):
    jobs = FlashJobs(simulate)
    token = secrets.token_urlsafe(32)
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass  # Do not persist request bodies or configuration values.

        def send(self, status, body, mime="application/json; charset=utf-8"):
            raw = body.encode() if isinstance(body, str) else json.dumps(body, ensure_ascii=False).encode()
            self.send_response(status)
            self.send_header("Content-Type", mime)
            self.send_header("Content-Length", str(len(raw)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.send_header("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'")
            self.end_headers()
            self.wfile.write(raw)

        def allowed(self):
            return self.headers.get("Host") == f"127.0.0.1:{self.server.server_port}"

        def do_GET(self):
            if not self.allowed():
                self.send(403, {"error": "Accès local uniquement."}); return
            if self.path == "/api/state":
                manifest = json.loads((ROOT / "firmware/manifest.json").read_text())
                firmware = {role: manifest["sha256"].get(f"{role}/firmware.bin", "") for role in ("master","receiver","receiver-rgb","sony-c3")}
                self.send(200, {"app_id":"tally-atelier-v1", "version":VERSION, "installation":INSTALLATION, "firmware":firmware, "job": jobs.snapshot(), "ports": ports(simulate), "ports_sony": ports(simulate, "sony-c3"), "simulate": simulate, "token": token}); return
            paths = {"/": ("index.html", "text/html; charset=utf-8"),
                     "/app.js": ("app.js", "text/javascript; charset=utf-8"),
                     "/style.css": ("style.css", "text/css; charset=utf-8")}
            if self.path not in paths:
                self.send(404, {"error": "Introuvable"}); return
            filename, mime = paths[self.path]
            self.send(200, (ROOT / "desktop" / filename).read_text(), mime)

        def do_POST(self):
            if not self.allowed() or self.headers.get("Origin") not in (None, f"http://127.0.0.1:{self.server.server_port}") or not secrets.compare_digest(self.headers.get("X-Tally-Token", ""), token):
                self.send(403, {"error": "Recharger l'application pour autoriser cette action locale."}); return
            if self.path not in {"/api/check", "/api/flash", "/api/quit"}:
                self.send(404, {"error": "Introuvable"}); return
            if self.path == "/api/quit":
                if jobs.snapshot()["status"] == "running":
                    self.send(409, {"error": "Attendre la fin de l'opération avant de quitter."}); return
                self.send(200, {"ok": True})
                threading.Thread(target=self.server.shutdown, daemon=True).start()
                return
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 4096:
                    raise ValueError("Requête invalide.")
                data = json.loads(self.rfile.read(length))
                if not isinstance(data, dict):
                    raise ValueError("Réglages invalides.")
                jobs.start(data, self.path == "/api/check")
            except (ValueError, TypeError):
                # Validation messages are safe: they do not include field values.
                import sys
                exc = sys.exc_info()[1]
                message = str(exc) if type(exc) is ValueError else "Réglages invalides."
                self.send(400, {"error": message}); return
            self.send(202, {"ok": True})

    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    server.jobs = jobs
    return server


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8764)
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--simulate", action="store_true", help="Demonstrate the UI without writing to USB")
    args = parser.parse_args()
    try:
        server = create_server(args.port, args.simulate)
    except OSError:
        # Reopening the launcher should bring back the same application.
        url = f"http://127.0.0.1:{args.port}"
        try:
            with urlopen(url + "/api/state", timeout=2) as response:
                state = json.load(response)
            if state.get("app_id") != "tally-atelier-v1" or state.get("simulate") != args.simulate:
                raise ValueError("Other application")
        except Exception:
            raise SystemExit(f"Le port {args.port} est occupé. Fermer l'autre application ou choisir --port NUMERO.") from None
        print(f"Tally Atelier déjà ouverte : {url}", flush=True)
        if state.get("version") != VERSION or state.get("installation") != INSTALLATION:
            if state.get("job", {}).get("status") == "running":
                raise SystemExit("Une autre version termine un flash. Attendre sa fin puis relancer.")
            with urlopen(Request(url + "/api/quit", data=b"", headers={"X-Tally-Token": state["token"]}), timeout=3):
                pass
            for attempt in range(20):
                try:
                    server = create_server(args.port, args.simulate)
                    break
                except OSError:
                    time.sleep(.2)
            else:
                raise SystemExit("Fermer l’ancienne version de Tally Atelier puis relancer.")
        else:
            if not args.no_browser:
                webbrowser.open(url)
            return
    url = f"http://127.0.0.1:{server.server_port}"
    print(f"Tally Atelier : {url}", flush=True)
    print("Garder cette fenêtre ouverte. Ctrl+C pour quitter.", flush=True)
    if not args.no_browser:
        threading.Timer(.4, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
