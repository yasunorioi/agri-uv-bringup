"""Strip the bogus `#include "String.h"` line from DFRobot's UV library.

The vendor header ships a leftover `#include "String.h"` (title-cased,
which doesn't exist on any Arduino core — Arduino's WString class comes
in via <Arduino.h>). This pre-build hook rewrites the two offending
files in .pio/libdeps once, in place, before compilation kicks off.
"""

Import("env")
import os
import re

TARGETS = [
    "DFRobot_UVIndex240370Sensor.h",
    "DFRobot_UVIndex240370Sensor.cpp",
]

libdeps = os.path.join(
    env["PROJECT_DIR"], ".pio", "libdeps", env["PIOENV"],
    "DFRobot_UVIndex240370Sensor",
)

pattern = re.compile(r'^\s*#\s*include\s+"String\.h".*\n', re.MULTILINE)

for name in TARGETS:
    path = os.path.join(libdeps, name)
    if not os.path.isfile(path):
        continue
    with open(path, "r", encoding="utf-8") as fh:
        text = fh.read()
    patched = pattern.sub("", text)
    if patched != text:
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(patched)
        print(f"[patch_dfrobot] stripped String.h include from {name}")
