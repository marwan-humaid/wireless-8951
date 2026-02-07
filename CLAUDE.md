# wireless-8951

NRF24L01 wireless communication between an STC89C52RC 8051 microcontroller (transmitter) and an ESP32-DevKitC V4 (receiver).

## Project Structure

```
wireless-8951/
+-- src/main.cpp              # ESP32 active firmware (PlatformIO/Arduino) - copy tx.cpp or rx.cpp here
+-- platformio.ini            # PlatformIO config (esp32dev, RF24 library)
+-- esp32_firmware/
|   +-- tx.cpp                # ESP32 NRF24L01 transmitter test
|   +-- rx.cpp                # ESP32 NRF24L01 receiver (prints to serial + LED blink)
+-- STC89C52/
|   +-- main.c                # STC89C52RC NRF TX + LCD1602 status display (Keil C51)
|   +-- nrf24l01.h            # NRF24L01 driver (bit-banged SPI, adapted from CooledCoffee/nrf24l01)
|   +-- lcd1602.h             # LCD1602 driver (8-bit mode, HD44780)
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

### ESP32-DevKitC V4 (Receiver)

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

External LED on GPIO 21 (ESP32 DevKitC V4 has no built-in LED on GPIO 2).

### STC89C52RC (Transmitter)

- Crystal: 11.0592 MHz
- LCD1602: data on P0, EN=P2.7, RS=P2.6, RW=P2.5
- NRF24L01: CE=P1.2, CSN=P1.3, SCK=P1.7, MOSI=P1.1, MISO=P1.6 (470R series resistors on SPI lines)
- USB serial via CH340G (COM port varies, check device manager)
- Keil C51 toolchain at `C:\Keil_v5\`
- No extra capacitor needed on NRF24L01 VCC/GND (dev board supply is sufficient)

## Build & Flash

### ESP32

Copy the desired firmware to src/main.cpp before building (PlatformIO compiles all .cpp in src/):

```bash
cp esp32_firmware/rx.cpp src/main.cpp   # for receiver
pio run -t upload --upload-port COMxx
pio device monitor -b 115200
```

### STC89C52

```bash
STC89C52/build.sh
```

This runs Keil UV4 headless (`-j0 -b`) and flashes via stcgal with DTR auto power-cycle. Build script has COM13 hardcoded; flash manually for other ports:

```bash
.venv/Scripts/stcgal -P stc89 -p COMxx -a STC89C52/Objects/STC89C52.hex
```

## Radio Config (must match on both sides)

- Address: {0x30, 0x30, 0x30, 0x30, 0x31} ("00001")
- Channel: 108
- Data rate: 1 Mbps
- CRC: 2-byte
- Auto-ack: disabled
- Payload: 32 bytes

## Known Issues

- **First SPI write drops on STC**: NRF24L01 needs ~5ms delay + dummy read before first register write after power-on. Handled in nrf24l01.h `_nrf_config()`.
- **Clone NRF24L01 modules don't support 250kbps**: `setDataRate(RF24_250KBPS)` silently fails, stays at 1Mbps.
