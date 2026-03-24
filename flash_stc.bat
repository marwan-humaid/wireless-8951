@echo off
if "%1"=="" (
    echo Usage: flash_stc.bat COMxx
    exit /b 1
)
C:\Keil_v5\UV4\UV4.exe -j0 -r STC89C52\STC89C52.uvproj
.venv\Scripts\stcgal -P stc89 -p %1 -a STC89C52\Objects\STC89C52.hex
echo Power-cycle the STC board now.
