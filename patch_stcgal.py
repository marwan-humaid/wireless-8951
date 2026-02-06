"""Patch stcgal to invert DTR/RTS auto-reset polarity.

Some dev boards (e.g. STC89C52RC with CH340G) need inverted DTR
to power-cycle correctly. Stock stcgal asserts high then low;
this patch flips it to low then high.

Run once after installing stcgal:
    .venv/Scripts/python patch_stcgal.py
"""

import importlib.util
import sys

spec = importlib.util.find_spec("stcgal.protocols")
if spec is None or spec.origin is None:
    print("ERROR: stcgal not found. Install it first: pip install stcgal")
    sys.exit(1)

path = spec.origin

with open(path, "r") as f:
    src = f.read()

# The full original (unpatched) block in reset_device():
#   assert pin -> sleep -> deassert pin
ORIGINAL = '''\
            if resetpin == "rts":
                self.ser.setRTS(True)
            else:
                self.ser.setDTR(True)

            time.sleep(0.25)

            if resetpin == "rts":
                self.ser.setRTS(False)
            else:
                self.ser.setDTR(False)'''

# The patched (inverted) block:
#   deassert pin -> sleep -> assert pin
PATCHED = '''\
            if resetpin == "rts":
                self.ser.setRTS(False)
            else:
                self.ser.setDTR(False)

            time.sleep(0.25)

            if resetpin == "rts":
                self.ser.setRTS(True)
            else:
                self.ser.setDTR(True)'''

if PATCHED in src:
    print("Already patched.")
    sys.exit(0)

if ORIGINAL not in src:
    print("ERROR: Could not find expected code block in", path)
    print("stcgal version may be incompatible.")
    sys.exit(1)

src = src.replace(ORIGINAL, PATCHED, 1)

with open(path, "w") as f:
    f.write(src)

print("Patched:", path)
print("DTR/RTS auto-reset polarity inverted.")
