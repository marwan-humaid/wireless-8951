# wireless-8951

NRF24L01 wireless communication between an STC89C52RC 8051 microcontroller and an ESP32-DevKitC V4. Primary use case: wireless keyboard -- type on PC, characters appear on LCD via radio.

## Project Structure

```
wireless-8951/
+-- src/main.cpp              # ESP32 active firmware (PlatformIO/Arduino) - copy from esp32_firmware/
+-- platformio.ini            # PlatformIO config (esp32dev, RF24 library)
+-- keyboard_client.py        # Python keyboard client (PC -> ESP32 serial)
+-- esp32_firmware/
|   +-- keyboard_tx.cpp       # ESP32 keyboard forwarder (serial -> NRF24L01 TX)
|   +-- tx.cpp                # ESP32 NRF24L01 counter transmitter (test/reference)
|   +-- rx.cpp                # ESP32 NRF24L01 receiver (test/reference)
+-- STC89C52/
|   +-- main.c                # STC89C52RC keyboard RX: NRF24L01 -> LCD1602 display
|   +-- nrf24l01.h            # NRF24L01 driver (bit-banged SPI, adapted from CooledCoffee/nrf24l01)
|   +-- lcd1602.h             # LCD1602 driver (8-bit mode, HD44780)
|   +-- STC89C52.uvproj       # Keil uVision project
|   +-- STC89C52.uvopt        # Keil uVision options
|   +-- build.sh              # CLI build + flash script
+-- patch_stcgal.py           # Inverts stcgal DTR polarity for auto power-cycle
+-- requirements.txt          # Python dependencies (platformio, stcgal, pyserial)
+-- .gitignore
```

## Setup

```bash
py -m venv .venv
.venv/Scripts/pip install -r requirements.txt
.venv/Scripts/python patch_stcgal.py
```

The patch script inverts stcgal's DTR auto-reset polarity. Required for boards where the CH340G DTR circuit needs inverted logic to power-cycle the MCU. Idempotent -- safe to run multiple times.

## Wireless Keyboard Usage

```bash
# Flash ESP32 (keyboard forwarder)
cp esp32_firmware/keyboard_tx.cpp src/main.cpp
pio run -t upload --upload-port COMxx

# Flash STC89C52 (keyboard receiver + LCD)
# Build in Keil GUI, then:
.venv/Scripts/stcgal -P stc89 -p COMxx -a STC89C52/Objects/STC89C52.hex
# Power cycle the STC board after flashing

# Run keyboard client
.venv/Scripts/python keyboard_client.py COMxx   # ESP32's COM port
```

Protocol: byte 0 = character count (1-31), bytes 1-31 = ASCII characters. Supports backspace, Enter (new line / clear), Ctrl+L (clear screen).

## Hardware

### ESP32-DevKitC V4 (Keyboard Forwarder)

NRF24L01 wiring (VSPI):

| NRF24L01 | ESP32 GPIO                        |
| -------- | --------------------------------- |
| VCC      | 5V (breakout board has regulator) |
| GND      | GND                               |
| CE       | GPIO 4                            |
| CSN      | GPIO 5                            |
| SCK      | GPIO 18                           |
| MOSI     | GPIO 23                           |
| MISO     | GPIO 19                           |
| IRQ      | N/C                               |

### STC89C52RC (Keyboard Receiver + LCD)

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
cp esp32_firmware/keyboard_tx.cpp src/main.cpp   # for keyboard forwarder
pio run -t upload --upload-port COMxx
```

### STC89C52

Build in Keil GUI (recommended -- CLI builds can produce stale artifacts), then flash:

```bash
.venv/Scripts/stcgal -P stc89 -p COMxx -a STC89C52/Objects/STC89C52.hex
```

Power cycle the STC board after flashing (the NRF24L01 needs a clean power-on reset).

## Radio Config (must match on both sides)

- Address: {0x30, 0x30, 0x30, 0x30, 0x31} ("00001")
- Channel: 108
- Data rate: 1 Mbps
- CRC: 2-byte
- Auto-ack: disabled
- Payload: 32 bytes

## Known Issues

- **First SPI write drops on STC**: NRF24L01 needs ~5ms delay + dummy read before first register write after power-on. Handled in nrf24l01.h `_nrf_config()`.
- **NRF24L01 dirty-state after MCU-only reset**: After stcgal flashes, the MCU resets but the NRF24L01 stays powered in a dirty state. `_nrf_config()` forces power-down + FIFO flush + flag clear before configuring. A manual power cycle is still recommended.
- **Clone NRF24L01 modules don't support 250kbps**: `setDataRate(RF24_250KBPS)` silently fails, stays at 1Mbps.
- **Keil CLI builds can be stale**: `UV4.exe -b` (incremental build) sometimes uses cached objects. Use `-r` (rebuild) or build from Keil GUI.
- **STC COM port changes**: CH340G board may appear on different COM ports between connections. Check device manager.
