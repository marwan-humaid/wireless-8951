@echo off
py -m venv .venv
.venv\Scripts\pip install -r requirements.txt
.venv\Scripts\python patch_stcgal.py
echo Setup complete.
pause
