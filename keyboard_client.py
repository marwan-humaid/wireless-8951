import serial
import sys
import msvcrt
import time

def main():
    if len(sys.argv) < 2:
        print("Usage: python keyboard_client.py COMxx [--debug]")
        sys.exit(1)

    port = sys.argv[1]
    debug = '--debug' in sys.argv
    ser = serial.Serial(port, 115200, timeout=1)
    print(f"Connected to {port}. Type to send to LCD.")
    print("  Enter = new line / clear")
    print("  Backspace = delete")
    print("  Ctrl+L = clear screen")
    print("  Ctrl+C = quit")
    if debug:
        print("  Debug output enabled")
    print()

    try:
        while True:
            # Read and display ESP32 debug output
            if ser.in_waiting:
                line = ser.readline().decode('ascii', errors='replace').rstrip()
                if line and debug:
                    print(f"\n  [ESP32] {line}", end='', flush=True)

            if msvcrt.kbhit():
                buf = b''
                while msvcrt.kbhit() and len(buf) < 31:
                    ch = msvcrt.getch()
                    if ch == b'\xe0' or ch == b'\x00':
                        msvcrt.getch()  # consume special key second byte
                        continue
                    buf += ch
                    # Local echo
                    if ch == b'\r':
                        print()
                    elif ch == b'\x08':
                        print('\b \b', end='', flush=True)
                    elif ch == b'\x0c':
                        print('[CLR]')
                    else:
                        try:
                            print(ch.decode('ascii'), end='', flush=True)
                        except UnicodeDecodeError:
                            pass
                if buf:
                    ser.write(buf)
                    time.sleep(0.03)  # give ESP32 time to transmit
            else:
                time.sleep(0.005)  # avoid busy-waiting
    except KeyboardInterrupt:
        print("\nDisconnected.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
