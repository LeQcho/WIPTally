#!/bin/zsh
set -e
set -o pipefail
cd -- "${0:A:h}"
if [[ ! -x .flash-venv/bin/python ]]; then
  python3 -m venv .flash-venv
fi
.flash-venv/bin/python -m pip -q install -r requirements-flash.txt
.flash-venv/bin/python flash.py receiver-rgb 2>&1 | tee flash-receiver-rgb.log
read -r "?Appuie sur Entree pour fermer..."
