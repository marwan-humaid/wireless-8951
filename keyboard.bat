@echo off
if "%1"=="" (
    echo Usage: keyboard.bat COMxx [--debug]
    exit /b 1
)
.venv\Scripts\python keyboard_client.py %*
