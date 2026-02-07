import serial
import sys
import msvcrt

def main():
    if len(sys.argv) < 2:
        print("Usage: python keyboard_client.py COMxx")
        sys.exit(1)

    port = sys.argv[1]
    ser = serial.Serial(port, 115200, timeout=1)
    print(f"Connected to {port}. Type to send to LCD.")
    print("  Enter = new line / clear")
    print("  Backspace = delete")
    print("  Ctrl+L = clear screen")
    print("  Ctrl+C = quit")
    print()

    try:
        while True:
            if msvcrt.kbhit():
                ch = msvcrt.getch()
                if ch == b'\xe0' or ch == b'\x00':
                    msvcrt.getch()  # consume special key second byte
                    continue
                ser.write(ch)
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
    except KeyboardInterrupt:
        print("\nDisconnected.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
