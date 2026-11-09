@echo off
setlocal
cd /d "%~dp0"
if not exist .flash-venv\Scripts\python.exe (
  py -3 -m venv .flash-venv
  if errorlevel 1 goto failed
)
.flash-venv\Scripts\python.exe -c "import esptool,serial,cryptography; from importlib.metadata import version; assert version('esptool') == '4.9.0'" >nul 2>&1
if errorlevel 1 (
  echo Preparation de Tally Atelier...
  .flash-venv\Scripts\python.exe -m pip install -r requirements-flash.txt
  if errorlevel 1 goto failed
)
.flash-venv\Scripts\python.exe tally_app.py
goto end
:failed
echo Installer Python 3 puis relancer. Internet est necessaire a la premiere preparation.
pause
:end
