# CPEG 300 - Embedded Systems

Wireless communication between an STC89C52RC (8051) and an ESP32 using NRF24L01 2.4 GHz radio modules. The ESP32 acts as a USB-to-radio bridge, forwarding keystrokes from a PC to the STC, which displays them on a 16x2 LCD.

## Boards used
- STC89C52RC (Prechin C51 development board)
- ESP32-DevKitC V4

## Demo
https://github.com/user-attachments/assets/df030393-71dc-4c5a-94b4-641df4fe7c1f

## Stress Test
https://github.com/user-attachments/assets/01c4b866-cc2d-494c-893d-67a41dd353ce

## Prerequisites

- Python 3.x
- [Keil uVision 5 C51](https://www.keil.com/) (evaluation version, 2048-byte code limit)
- [PlatformIO CLI](https://platformio.org/)
- Two NRF24L01 modules
- USB cables for both boards

## Setup

```bash
# Create virtual environment and install dependencies
py -m venv .venv
.venv/Scripts/pip install -r requirements.txt

# Patch stcgal DTR polarity for the Prechin board
.venv/Scripts/python patch_stcgal.py
```

## Flash ESP32

```bash
# Copy keyboard forwarder firmware and flash
cp esp32_firmware/keyboard_tx.cpp src/main.cpp
pio run -t upload --upload-port COMxx
```

Replace `COMxx` with the ESP32's COM port (check Device Manager).

## Flash STC89C52

Build in Keil GUI, then flash via CLI:

```bash
.venv/Scripts/stcgal -P stc89 -p COMxx -a STC89C52/Objects/STC89C52.hex
```

Or use the build script (builds and flashes in one step):

```bash
cd STC89C52
bash build.sh COMxx
```

Power-cycle the STC board after flashing so the NRF24L01 gets a clean reset.

## Run

```bash
# Start the keyboard client (connects to ESP32's COM port)
.venv/Scripts/python keyboard_client.py COMxx

# With debug output from ESP32:
.venv/Scripts/python keyboard_client.py COMxx --debug
```

Type on the PC and characters appear on the LCD wirelessly.

## Radio Configuration

Must match on both boards:

| Parameter | Value |
|-----------|-------|
| Address | `{0x30, 0x30, 0x30, 0x30, 0x31}` |
| Channel | 108 |
| Data rate | 1 Mbps |
| CRC | 2-byte |
| Auto-ack | Disabled |
| Payload | 32 bytes |
