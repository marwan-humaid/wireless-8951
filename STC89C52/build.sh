#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$SCRIPT_DIR/STC89C52.uvproj"
LOG="$SCRIPT_DIR/build_log.txt"

"/c/Keil_v5/UV4/UV4.exe" -j0 -r "$PROJECT" -o "$LOG"
cat "$LOG"
