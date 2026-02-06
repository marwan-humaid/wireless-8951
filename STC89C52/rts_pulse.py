import serial, time, sys
port = sys.argv[1] if len(sys.argv) > 1 else "COM13"
s = serial.Serial(port)
s.rts = True    # power off (RTS# goes low)
time.sleep(0.3)
s.rts = False   # power on (RTS# goes high)
time.sleep(0.1)
s.close()
