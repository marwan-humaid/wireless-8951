"""
Diagnostic: monitor ESP32 (TX) and STC (RX) serial output simultaneously,
send test characters, and report packet delivery stats.

Test 1: Reset STC via inverted DTR, wait for boot, test delivery
Test 2: Without reset, test again to see if send_ack() corrupts RX state
"""
import serial
import threading
import time
import re

ESP32_PORT = "COM14"
ESP32_BAUD = 115200
STC_PORT = "COM15"
STC_BAUD = 57600

INTER_CHAR_DELAY = 1.0


def reader(ser, output_list, label, stop_event):
    try:
        while not stop_event.is_set():
            raw = ser.readline()
            if raw:
                ts = time.perf_counter()
                line = raw.decode("ascii", errors="replace").strip()
                if line:
                    output_list.append((ts, line))
                    print(f"  [{label}] {line}")
    except serial.SerialException as e:
        print(f"  [{label}] Serial error: {e}")


def reset_stc(ser):
    """Reset STC via inverted DTR (board uses inverted logic):
    DTR=False (HIGH) = assert reset, DTR=True (LOW) = release."""
    print("Resetting STC (inverted DTR pulse)...")
    ser.dtr = False   # assert reset (HIGH on this board)
    time.sleep(0.25)
    ser.dtr = True    # release reset (LOW on this board)
    time.sleep(0.03)


def analyze(esp32_lines, stc_lines, label):
    print(f"\n=== {label} ===\n")

    tx_events = {}
    ack_results = {}
    for ts, line in esp32_lines:
        m = re.match(r"TX seq=([0-9A-Fa-f]+) cnt=(\d+) data=(.*)", line)
        if m:
            seq = m.group(1)
            tx_events[seq] = {"ts": ts, "cnt": int(m.group(2)), "data": m.group(3)}
        m = re.match(r"\s*ACK after (\d+) retries", line)
        if m:
            for seq in reversed(list(tx_events.keys())):
                if seq not in ack_results:
                    ack_results[seq] = {"retries": int(m.group(1)), "ts": ts}
                    break
        m = re.match(r"\s*FAILED seq=([0-9A-Fa-f]+)", line)
        if m:
            ack_results[m.group(1)] = {"retries": 15, "failed": True, "ts": ts}

    rx_events = {}
    for ts, line in stc_lines:
        m = re.match(r"PKT seq=([0-9A-Fa-f]+) cnt=([0-9A-Fa-f]+) data=(.*)", line)
        if m:
            rx_events[m.group(1)] = {"ts": ts, "cnt": m.group(2), "data": m.group(3)}

    print(f"ESP32 TX: {len(tx_events)} | STC RX: {len(rx_events)}")
    total_retries = 0
    failed = 0
    for seq, tx in tx_events.items():
        ack = ack_results.get(seq, {"retries": 0})
        retries = ack.get("retries", 0)
        total_retries += retries
        is_failed = ack.get("failed", False)
        if is_failed:
            failed += 1
        rx = rx_events.get(seq)
        rx_status = "OK" if rx else "MISSING"
        retry_str = f"retry={retries}" if retries > 0 else "1st"
        fail_str = " FAIL" if is_failed else ""
        print(f"  seq={seq} '{tx['data']}' {retry_str}{fail_str} -> {rx_status}")

    if tx_events:
        print(f"\n  Retries: {total_retries} | Failed: {failed} | "
              f"Delivery: {len(rx_events)}/{len(tx_events)} ({100*len(rx_events)/len(tx_events):.0f}%)")


def send_chars(esp32_ser, chars):
    print(f"\n--- Sending '{chars}' ---\n")
    for i, ch in enumerate(chars):
        print(f"  [SEND] {ch} (#{i})")
        esp32_ser.write(ch.encode("ascii"))
        time.sleep(INTER_CHAR_DELAY)
    time.sleep(3)


def main():
    print(f"ESP32: {ESP32_PORT} | STC: {STC_PORT}\n")

    # Open ports - don't auto-assert DTR on STC
    stc_ser = serial.Serial(STC_PORT, STC_BAUD, timeout=0.1, dsrdtr=True)
    esp32_ser = serial.Serial(ESP32_PORT, ESP32_BAUD, timeout=0.1)

    # === TEST 1: Reset STC, wait for boot ===
    print("=" * 50)
    print("TEST 1: Reset STC via inverted DTR, send 5 chars")
    print("=" * 50)

    reset_stc(stc_ser)
    esp32_ser.reset_input_buffer()
    stc_ser.reset_input_buffer()

    print("Waiting 5s for STC boot (LCD init + NRF init)...")
    time.sleep(5)
    esp32_ser.reset_input_buffer()
    stc_ser.reset_input_buffer()

    esp32_lines_1 = []
    stc_lines_1 = []
    stop1 = threading.Event()
    t1e = threading.Thread(target=reader, args=(esp32_ser, esp32_lines_1, "ESP32", stop1), daemon=True)
    t1s = threading.Thread(target=reader, args=(stc_ser, stc_lines_1, "STC", stop1), daemon=True)
    t1e.start()
    t1s.start()

    send_chars(esp32_ser, "ABCDE")
    stop1.set()
    t1e.join(timeout=2)
    t1s.join(timeout=2)
    analyze(esp32_lines_1, stc_lines_1, "TEST 1: AFTER FRESH RESET")

    # === TEST 2: No reset, test again ===
    print("\n" + "=" * 50)
    print("TEST 2: NO reset, send 5 more chars")
    print("=" * 50)

    esp32_ser.reset_input_buffer()
    stc_ser.reset_input_buffer()

    esp32_lines_2 = []
    stc_lines_2 = []
    stop2 = threading.Event()
    t2e = threading.Thread(target=reader, args=(esp32_ser, esp32_lines_2, "ESP32", stop2), daemon=True)
    t2s = threading.Thread(target=reader, args=(stc_ser, stc_lines_2, "STC", stop2), daemon=True)
    t2e.start()
    t2s.start()

    send_chars(esp32_ser, "FGHIJ")
    stop2.set()
    t2e.join(timeout=2)
    t2s.join(timeout=2)
    analyze(esp32_lines_2, stc_lines_2, "TEST 2: WITHOUT RESET (after ACK switching)")

    esp32_ser.close()
    stc_ser.close()


if __name__ == "__main__":
    main()
