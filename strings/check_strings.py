#!/usr/bin/env python3
# satellite-004/strings/check_strings.py -- proves satellite_string against Python.
#
# Every UTF-8 case is decoded twice: by satellite_string (through build/string_cases)
# and by Python's strict codec. They must agree on valid/invalid, on every code
# point, and on the byte offset where an invalid sequence starts. Cases: every
# edge of the Unicode table, several scripts, known-bad sequences, and 30,000
# random byte strings (fixed seed, so a failure repeats). It also checks encoding
# of values that are not characters, and the .sati bit form.
#
# Run from satellite-004 after building build/string_cases:  python3 strings/check_strings.py

import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HARNESS = os.path.join(HERE, "..", "build", "string_cases")

utf8_cases = [
    b"", b"hello",
    "мир".encode(),                      # Russian
    "你好世界".encode(),                # Chinese
    "مرحبا".encode(),          # Arabic
    "नमस्ते".encode(),    # Hindi
    "\U0001F30D\U0001F680".encode(),                    # emoji
    "é".encode(),                                 # e + combining accent
    "\u0000".encode(), b'a"b\\c\nd',
]
for cp in [0x7F, 0x80, 0x7FF, 0x800, 0xD7FF, 0xE000, 0xFFFD, 0xFFFF, 0x10000, 0x10FFFF]:
    utf8_cases.append(chr(cp).encode())
utf8_cases += [bytes.fromhex(h) for h in [
    "c080",       # overlong NUL
    "c1bf",       # overlong
    "e08080",     # overlong
    "e09fbf",     # overlong
    "eda080",     # surrogate U+D800
    "edbfbf",     # surrogate U+DFFF
    "f0808080",   # overlong
    "f48fbfbf",   # U+10FFFF, valid
    "f4908080",   # above U+10FFFF
    "f5808080", "ff", "fe", "80", "bf",
    "c2", "e0a0", "f09080",   # cut short
    "c2c2", "e0a0c0", "41c380c3", "efbbbf41",
]]
rng = random.Random(4)
for _ in range(30000):
    if rng.random() < 0.5:
        pool = [rng.randint(0, 255) for _ in range(rng.randint(1, 12))]
    else:
        cp = rng.choice([rng.randint(0x20, 0x7E), rng.randint(0x80, 0x7FF),
                         rng.randint(0x800, 0xD7FF), rng.randint(0x10000, 0x10FFFF)])
        pool = list(chr(cp).encode())
        if rng.random() < 0.3:
            pool[rng.randrange(len(pool))] = rng.randint(0x80, 0xFF)
    utf8_cases.append(bytes(pool))


def python_answer(b):
    try:
        return "ok " + ",".join("%x" % ord(c) for c in b.decode("utf-8", "strict"))
    except UnicodeDecodeError as error:
        return "bad %d" % error.start


encode_cases = [[0x41], [0xD800], [0xDFFF], [0x110000], [0x41, 0xDC00], [0x10FFFF], [0xFFFFFFFF]]


def python_encode(cps):
    for i, cp in enumerate(cps):
        if cp > 0x10FFFF or 0xD800 <= cp <= 0xDFFF:
            return "bad %d" % i
    return "ok " + "".join(chr(c) for c in cps).encode().hex()


bit_cases = [
    ("0" * 32, "ok 0"),
    (format(0x416, "032b"), "ok 416"),
    ("0" * 31, "bad 0"),
    ("0" * 31 + "2", "bad 31"),
    (format(0xD800, "032b"), "bad 0"),
    (format(0x110000, "032b"), "bad 0"),
    ("0" * 32 + format(0x1F30D, "032b"), "ok 0,1f30d"),
]

lines = (["U " + b.hex() for b in utf8_cases]
         + ["C " + ",".join("%x" % c for c in cps) for cps in encode_cases]
         + ["B " + bits for bits, _ in bit_cases])
expected = ([python_answer(b) for b in utf8_cases]
            + [python_encode(c) for c in encode_cases]
            + [want for _, want in bit_cases])
got = subprocess.run([HARNESS], input="\n".join(lines) + "\n", capture_output=True, text=True).stdout.splitlines()

wrong = [(lines[i], expected[i], got[i] if i < len(got) else "<missing>")
         for i in range(len(lines)) if i >= len(got) or got[i] != expected[i]]
valid = sum(1 for e in expected[:len(utf8_cases)] if e.startswith("ok"))
print(f"{len(lines)} cases ({len(utf8_cases)} UTF-8: {valid} valid, {len(utf8_cases) - valid} invalid; "
      f"{len(encode_cases)} encode; {len(bit_cases)} bits)")
for case, want, have in wrong[:10]:
    print(f"  MISMATCH {case[:60]}  python: {want}  satellite: {have}")
print("all agree with Python" if not wrong else f"{len(wrong)} disagree")
sys.exit(1 if wrong else 0)
