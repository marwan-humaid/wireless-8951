@echo off
if "%1"=="" (
    echo Usage: flash_esp.bat COMxx
    exit /b 1
)
copy esp32_firmware\keyboard_tx.cpp src\main.cpp
pio run -t upload --upload-port %1
