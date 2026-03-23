"""Patch stcgal to invert DTR/RTS auto-reset polarity.

Some dev boards (e.g. STC89C52RC with CH340G) need inverted DTR
to power-cycle correctly. Stock stcgal asserts high then low;
this patch flips it to low then high.

Run once after installing stcgal:
    .venv/Scripts/python patch_stcgal.py
"""

import importlib.util
import re
import sys

spec = importlib.util.find_spec("stcgal.protocols")
if spec is None or spec.origin is None:
    print("ERROR: stcgal not found. Install it first: pip install stcgal")
    sys.exit(1)

path = spec.origin

with open(path, "r") as f:
    src = f.read()

# Match the reset_device pattern regardless of whitespace style (spaces or tabs).
# Original: setRTS(True)...setDTR(True)...sleep...setRTS(False)...setDTR(False)
# Patched:  setRTS(False)...setDTR(False)...sleep...setRTS(True)...setDTR(True)

PATTERN = re.compile(
    r'(if resetpin == "rts":\s+self\.ser\.setRTS\()(True)'
    r'(\)\s+else:\s+self\.ser\.setDTR\()(True)'
    r'(\)[\s\S]*?time\.sleep\(0\.25\)[\s\S]*?'
    r'if resetpin == "rts":\s+self\.ser\.setRTS\()(False)'
    r'(\)\s+else:\s+self\.ser\.setDTR\()(False)(\))'
)

PATCHED_CHECK = re.compile(
    r'if resetpin == "rts":\s+self\.ser\.setRTS\(False\)'
    r'\s+else:\s+self\.ser\.setDTR\(False\)'
    r'[\s\S]*?time\.sleep\(0\.25\)[\s\S]*?'
    r'if resetpin == "rts":\s+self\.ser\.setRTS\(True\)'
    r'\s+else:\s+self\.ser\.setDTR\(True\)'
)

if PATCHED_CHECK.search(src):
    print("Already patched.")
    sys.exit(0)

match = PATTERN.search(src)
if not match:
    print("ERROR: Could not find expected code block in", path)
    print("stcgal version may be incompatible.")
    sys.exit(1)

# Swap True<->False in the matched groups
src = src[:match.start()] + (
    match.group(1) + "False" +
    match.group(3) + "False" +
    match.group(5) + "True" +
    match.group(7) + "True" + match.group(9)
) + src[match.end():]

with open(path, "w") as f:
    f.write(src)

print("Patched:", path)
print("DTR/RTS auto-reset polarity inverted.")
