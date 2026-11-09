#!/bin/zsh
set -e
cd -- "${0:A:h}"
if [[ ! -x .flash-venv/bin/python ]]; then
  python3 -m venv .flash-venv
fi
if ! .flash-venv/bin/python -c 'import esptool, serial, cryptography; from importlib.metadata import version; assert version("esptool") == "4.9.0"' 2>/dev/null; then
  print "Préparation de Tally Atelier (Internet nécessaire une seule fois)…"
  .flash-venv/bin/python -m pip install -r requirements-flash.txt
fi
.flash-venv/bin/python tally_app.py
