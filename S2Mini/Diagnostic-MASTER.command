#!/bin/zsh
set -e
cd -- "${0:A:h}"
if [[ ! -x .flash-venv/bin/python ]]; then
  python3 -m venv .flash-venv
  .flash-venv/bin/python -m pip -q install -r requirements-flash.txt
fi
.flash-venv/bin/python diagnostic_master.py
read -r "?Appuie sur Entree pour fermer..."
