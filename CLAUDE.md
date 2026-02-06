# wireless-8951

NRF24L01 wireless communication between an ESP32-DevKitC V4 (transmitter) and an STC89C52RC 8051 microcontroller (receiver).

## Project Structure

```
wireless-8951/
+-- src/main.cpp              # ESP32 NRF24L01 transmitter (PlatformIO/Arduino)
+-- platformio.ini            # PlatformIO config (esp32dev, RF24 library)
+-- STC89C52/
|   +-- main.c                # STC89C52RC firmware (Keil C51)
|   +-- STC89C52.uvproj       # Keil uVision project
|   +-- STC89C52.uvopt        # Keil uVision options
|   +-- build.sh              # CLI build + flash script
|   +-- rts_pulse.py          # RTS power-cycle helper (unused, kept for reference)
+-- patch_stcgal.py           # Inverts stcgal DTR polarity for auto power-cycle
+-- requirements.txt          # Python dependencies (platformio, stcgal)
+-- .gitignore
```

## Setup

```bash
py -m venv .venv
.venv/Scripts/pip install -r requirements.txt
.venv/Scripts/python patch_stcgal.py
```

The patch script inverts stcgal's DTR auto-reset polarity. Required for boards where the CH340G DTR circuit needs inverted logic to power-cycle the MCU. Idempotent -- safe to run multiple times.

## Hardware

### ESP32-DevKitC V4 (Transmitter)

NRF24L01 wiring (VSPI):

| NRF24L01 | ESP32 GPIO |
|----------|-----------|
| VCC      | 5V (breakout board has regulator) |
| GND      | GND       |
| CE       | GPIO 4    |
| CSN      | GPIO 5    |
| SCK      | GPIO 18   |
| MOSI     | GPIO 23   |
| MISO     | GPIO 19   |
| IRQ      | N/C       |

### STC89C52RC (Receiver)

- Crystal: 11.0592 MHz
- LEDs on P2.0-P2.7 (active low)
- USB serial via CH340G on COM13
- Keil C51 toolchain at `C:\Keil_v5\`

## Build & Flash

### ESP32

```bash
.venv/Scripts/activate
pio run -t upload          # build and flash
pio device monitor -b 115200
```

### STC89C52

```bash
STC89C52/build.sh
```

This runs Keil UV4 headless (`-j0 -b`) and flashes via stcgal with DTR auto power-cycle. No manual intervention needed.

To flash manually without the build script:

```bash
.venv/Scripts/stcgal -P stc89 -p COM13 -a STC89C52/Objects/STC89C52.hex
```

## Radio Config (must match on both sides)

- Address: `"00001"`
- Channel: 76
- Data rate: 250kbps
- CRC: 2-byte
- Payload: 28 bytes (4-byte counter + 24-byte message)
