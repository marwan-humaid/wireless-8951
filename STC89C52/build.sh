#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT="$SCRIPT_DIR/STC89C52.uvproj"
HEX="$SCRIPT_DIR/Objects/STC89C52.hex"
LOG="$SCRIPT_DIR/build_log.txt"

# Build
"/c/Keil_v5/UV4/UV4.exe" -j0 -r "$PROJECT" -o "$LOG"
cat "$LOG"

# Flash (pass COM port as argument, e.g.: ./build.sh COM10)
if [ -n "$1" ]; then
  "$ROOT_DIR/.venv/Scripts/stcgal" -P stc89 -p "$1" -a "$HEX"
fi
