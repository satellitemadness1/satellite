#!/usr/bin/env python3
# satellite/satellite_variable_string/check_strings16.py -- proves satellite_string
# (16-bit fast path, 32-bit slower path, the author's table) against Python.
#
# THE REFEREE IS INDEPENDENT OF THE C++:
#   - the character table is read from the COMMENT in satellite_string.hpp, the
#     words the author's order is written in, not from satellite_string.cpp;
#   - UTF-8 is Python's own strict codec;
#   - every expected answer is worked out here, then compared with what
#     build/string16_cases prints, line for line.
#
# THE CASES: every case of strings/check_strings.py (30,055: its section that
# builds them is executed as written, so they are the same cases, same seed),
# each checked for more than before -- the codes, fast() true exactly when every
# character is <= U+FFFF, and to_utf8 byte-identical -- then append, s.append(s),
# substring (into a fresh, a fast and a wide string, and into itself), clear,
# code_at, compare in code order, and decoding into a string already in use.
# It also runs build/string_table_check.
#
# Run from the top of the tree after building:  make build/string16_cases build/string_table_check
#                                               python3 satellite/satellite_variable_string/check_strings16.py

import os
import random
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.join(HERE, "..", "..")
HARNESS = os.environ.get("STRING16_CASES", os.path.join(TOP, "build", "string16_cases"))
TABLE_CHECK = os.environ.get("STRING_TABLE_CHECK", os.path.join(TOP, "build", "string_table_check"))
POSITION_PAST_THE_END, POSITIONS_BACKWARDS = 16, 17   # satellite/machine/machine_codes.hpp


def table_from_header():
    """Code -> ASCII character, read from satellite_string.hpp's comment."""
    text = open(os.path.join(HERE, "satellite_string.hpp"), encoding="utf-8").read()
    block = text[text.index("Every character is one code:"):text.index("128 up")]
    lines = [line[2:].strip() for line in block.splitlines()[1:] if line.startswith("//")]
    order = {}
    names = {"NUL": "\0", "space": " ", "tab": "\t", "newline": "\n"}
    for index, line in enumerate(lines):
        if not line:
            continue
        one = re.fullmatch(r"(\d+)\s+(NUL)", line)
        three = re.fullmatch(r"(\d+) (\d+) (\d+)\s+(\w+), (\w+), (\w+)", line)
        span = re.fullmatch(r"(\d+)-(\d+)\s+(.*)", line)
        if one:
            characters, first, last = [names[one.group(2)]], int(one.group(1)), int(one.group(1))
        elif three:
            characters = [names[three.group(k)] for k in (4, 5, 6)]
            first, last = int(three.group(1)), int(three.group(3))
        elif span:
            first, last, words = int(span.group(1)), int(span.group(2)), span.group(3)
            letters = re.fullmatch(r"(\S)-(\S)", words)
            if letters:
                characters = [chr(c) for c in range(ord(letters.group(1)), ord(letters.group(2)) + 1)]
            elif words.startswith("the other"):
                ranges = re.findall(r"0x([0-9A-F]{2})(?:-0x([0-9A-F]{2}))?", lines[index + 1])
                characters = [chr(c) for a, b in ranges for c in range(int(a, 16), int(b or a, 16) + 1)]
            else:
                characters = words.split()
        else:
            continue                                   # the "(0x01-0x08, ...)" line, read above
        if len(characters) != last - first + 1:
            sys.exit(f"header table: {line!r} names {len(characters)} characters for codes {first}-{last}")
        for offset, character in enumerate(characters):
            order[first + offset] = character
    if sorted(order) != list(range(128)) or sorted(ord(c) for c in order.values()) != list(range(128)):
        sys.exit("header table: codes 0-127 are not every ASCII character exactly once")
    return [ord(order[code]) for code in range(128)]


ASCII_OF_CODE = table_from_header()
CODE_OF_ASCII = {unicode: code for code, unicode in enumerate(ASCII_OF_CODE)}


def code_of(unicode):
    return CODE_OF_ASCII[unicode] if unicode < 128 else unicode


def shown(text):
    codes = [code_of(ord(c)) for c in text]
    return "codes=%s fast=%d" % (",".join("%x" % c for c in codes) or "-", all(ord(c) <= 0xFFFF for c in text))


def hex_or_dash(data):
    return data.hex() or "-"


# ---- every case of strings/check_strings.py, built by its own code ----
source = open(os.path.join(TOP, "strings", "check_strings.py"), encoding="utf-8").read()
old = {"__file__": os.path.join(TOP, "strings", "check_strings.py")}
exec(compile(source[:source.index("\nlines = (")], "strings/check_strings.py", "exec"), old)
utf8_cases, encode_cases, bit_cases = old["utf8_cases"], old["encode_cases"], old["bit_cases"]
if len(utf8_cases) + len(encode_cases) + len(bit_cases) != 30055:
    sys.exit("strings/check_strings.py no longer has 30,055 cases; read it before trusting this check")

lines, expected, kinds = [], [], []


def case(kind, line, want):
    kinds.append(kind)
    lines.append(line)
    expected.append(want)


def decode_answer(data):
    try:
        text = data.decode("utf-8", "strict")
        return "ok %s utf8=%s" % (shown(text), hex_or_dash(data))
    except UnicodeDecodeError as error:
        return "bad %d %s" % (error.start, shown(data[:error.start].decode("utf-8", "strict")))


for data in utf8_cases:
    case("U decode", "U " + hex_or_dash(data), decode_answer(data))
for cps in encode_cases:
    old_answer = old["python_encode"](cps)
    if old_answer.startswith("bad"):
        refused = int(old_answer.split()[1])
        want = "bad %d %s" % (refused, shown("".join(chr(c) for c in cps[:refused])))
    else:
        text = "".join(chr(c) for c in cps)
        want = "ok fast=%d utf8=%s" % (all(c <= 0xFFFF for c in cps), hex_or_dash(text.encode()))
    case("C append_code", "C " + ",".join("%x" % c for c in cps), want)
for bits, want in bit_cases:
    case("B bits", "B " + bits, want)

# ---- the new cases ----
E, R, W = chr(0x1F30D), chr(0x1F680), chr(0x10FFFF)   # two emoji and the last character: wide
pool = ["", "a", "b", "z", "A", "0", "!", " ", "\t", "\n", chr(0), chr(0x7F), chr(0x80), "hello", "Hello, World!",
        chr(0x43C) + chr(0x438) + chr(0x440), chr(0x4F60) + chr(0x597D), chr(0xFFFF), chr(0xE000), chr(0x7FF) + chr(0x800),
        chr(0x10000), E, "a" + R + "b", "ab" + E + "cd" + R, "wide at the end " + W, E + " wide at the start",
        chr(0xFFFF) + chr(0x10000), "ab", "abc"]
rng = random.Random(16)
valid = [b.decode() for b in utf8_cases if old["python_answer"](b).startswith("ok")]
extra = rng.sample(valid, 300)


def utf8_hex(text):
    return hex_or_dash(text.encode())


for text in pool + extra:
    case("AA s.append(s)", "AA " + utf8_hex(text), shown(text + text))
    case("K clear", "K " + utf8_hex(text), "cleared size=0 empty=1 fast=1 then " + shown("a"))
for left in pool:
    for right in pool:
        case("A append", "A %s %s" % (utf8_hex(left), utf8_hex(right)), shown(left + right))
for _ in range(1000):
    left, right = rng.choice(valid), rng.choice(valid)
    case("A append", "A %s %s" % (utf8_hex(left), utf8_hex(right)), shown(left + right))


def compare_answer(left, right):
    a, b = [code_of(ord(c)) for c in left], [code_of(ord(c)) for c in right]
    order = (a > b) - (a < b)                           # Python lists: code by code, a prefix first
    return "compare=%d equal=%d not_equal=%d less=%d" % (order, order == 0, order != 0, order < 0)


compare_pairs = [(l, r) for l in pool for r in pool] + [(rng.choice(valid), rng.choice(valid)) for _ in range(1000)]
compare_pairs += [("a" + E, "ab"), (chr(0xFFFF), chr(0x10000)), (chr(0x10000), chr(0xFFFF)), ("abc", "ab"), ("", E)]
by_code = [chr(ASCII_OF_CODE[code]) for code in range(128)]
compare_pairs += [(by_code[k], by_code[k + 1]) for k in range(127)] + [(by_code[k + 1], by_code[k]) for k in range(127)]
for left, right in compare_pairs:
    case("M compare", "M %s %s" % (utf8_hex(left), utf8_hex(right)), compare_answer(left, right))
# The header's own example, which the lines above include: a < b < z < A < 0 < ! < space.
chain = ["a", "b", "z", "A", "0", "!", " "]
chain_lines = [len(lines)]
for left, right in zip(chain, chain[1:]):
    case("M the header's order", "M %s %s" % (utf8_hex(left), utf8_hex(right)), "compare=-1 equal=0 not_equal=1 less=1")
chain_lines.append(len(lines))

for text in pool + extra[:40]:
    size = len(text)
    for index in list(range(size + 2)) + [18446744073709551615]:
        want = ("ok %x" % code_of(ord(text[index]))) if index < size else "bad %d" % POSITION_PAST_THE_END
        case("P code_at", "P %s %d" % (utf8_hex(text), index), want)
    # start runs one past where end can be, so some pairs are both backwards and
    # past the end: the answer is then 17, backwards, checked first as in 003.
    for start in list(range(size + 3)) + [18446744073709551615]:
        for end in list(range(size + 2)) + [18446744073709551615]:
            if start > end:
                code = POSITIONS_BACKWARDS
            elif end > size:
                code = POSITION_PAST_THE_END
            else:
                code = 0
            for before in ["", "x", E]:
                want = ("ok " + shown(text[start:end])) if code == 0 else "bad %d %s" % (code, shown(before))
                case("S substring", "S %s %d %d %s" % (utf8_hex(text), start, end, utf8_hex(before)), want)
            want = ("ok " + shown(text[start:end])) if code == 0 else "bad %d %s" % (code, shown(text))
            case("SS s.substring(.., s)", "SS %s %d %d" % (utf8_hex(text), start, end), want)
# Long text: the cases above are at most a few characters, and the decoder takes
# ASCII 8 bytes at a time and the encoder 16 codes at a time. Runs of every length
# from 0 to 40 between other characters, so every block boundary is met; a third
# of them with one byte overwritten, so a refusal lands inside and after a run.


def a_character():                                    # any width, never a surrogate
    return chr(rng.choice([rng.randint(0, 0x7F), rng.randint(0x80, 0x7FF), rng.randint(0x800, 0xD7FF),
                           rng.randint(0xE000, 0xFFFF), rng.randint(0x10000, 0x10FFFF)]))


def long_text():
    pieces = []
    for _ in range(rng.randint(1, 12)):
        pieces.append("".join(chr(rng.randint(0x20, 0x7E)) for _ in range(rng.randint(0, 40))))
        pieces.append(a_character())
    return "".join(pieces)


long_texts = [long_text() for _ in range(3000)]
long_texts += ["".join(chr(rng.randint(0, 0x7F)) for _ in range(length)) for length in range(0, 70)]   # ASCII only
long_texts += ["x" * length + E + "y" * length for length in range(0, 40)]                             # one wide in the middle
long_bytes = []
for text in long_texts:
    data = bytearray(text.encode())
    if data and rng.random() < 1 / 3:
        data[rng.randrange(len(data))] = rng.randint(0x80, 0xFF)
    long_bytes.append(bytes(data))
for data in long_bytes:
    case("U decode, long", "U " + hex_or_dash(data), decode_answer(data))
for text in long_texts[:500]:
    case("C append_code, long", "C " + (",".join("%x" % ord(c) for c in text) or "-"),
         "ok fast=%d utf8=%s" % (all(ord(c) <= 0xFFFF for c in text), utf8_hex(text)))
for _ in range(1000):
    left, right = rng.choice(long_texts), rng.choice(long_texts)
    case("A append, long", "A %s %s" % (utf8_hex(left), utf8_hex(right)), shown(left + right))
    case("AA s.append(s), long", "AA " + utf8_hex(left), shown(left + left))
    start = rng.randint(0, len(left))
    end = rng.randint(start, len(left))
    case("SS s.substring(.., s), long", "SS %s %d %d" % (utf8_hex(left), start, end), "ok " + shown(left[start:end]))
    case("S substring, long", "S %s %d %d %s" % (utf8_hex(left), start, end, utf8_hex(rng.choice(["", "x", E]))),
         "ok " + shown(left[start:end]))
for _ in range(1000):
    text = rng.choice(long_texts)
    cut = rng.randint(0, len(text))
    other = text[:cut] + a_character() + text[cut + 1:]                                               # differs at `cut`, or longer
    for left, right in ((text, other), (other, text), (text, text[:cut])):
        case("M compare, long", "M %s %s" % (utf8_hex(left), utf8_hex(right)), compare_answer(left, right))

for before in ["", "x", E]:
    for data in utf8_cases[:41] + rng.sample(utf8_cases, 2000) + long_bytes[:300]:
        case("UP decode into a used string", "UP %s %s" % (utf8_hex(before), hex_or_dash(data)), decode_answer(data))

# ---- run ----
harness = subprocess.run([HARNESS], input="\n".join(lines) + "\n", capture_output=True, text=True)
got = harness.stdout.splitlines()
wrong = [i for i in range(len(lines)) if i >= len(got) or got[i] != expected[i]]
old_count = len(utf8_cases) + len(encode_cases) + len(bit_cases)
valid_count = sum(1 for want in expected[:len(utf8_cases)] if want.startswith("ok"))
print(f"{len(lines)} operations; the {old_count} of strings/check_strings.py ({len(utf8_cases)} UTF-8: "
      f"{valid_count} valid, {len(utf8_cases) - valid_count} invalid; {len(encode_cases)} encode; {len(bit_cases)} bits)")
print(f"table read from satellite_string.hpp: codes 0-127 = {''.join(repr(chr(u))[1:-1] for u in ASCII_OF_CODE[:63])} ...")
for kind in dict.fromkeys(kinds):
    total = kinds.count(kind)
    bad = sum(1 for i in wrong if kinds[i] == kind)
    print(f"  {kind:32} {total:6}  {'all agree' if bad == 0 else str(bad) + ' DISAGREE'}")
wide_decodes = sum(1 for want in expected[:len(utf8_cases)] if " fast=0 " in want)
print(f"  ({wide_decodes} of the UTF-8 cases decode wide; a < b < z < A < 0 < ! < space: "
      f"{'held' if not any(chain_lines[0] <= i < chain_lines[1] for i in wrong) else 'BROKEN'})")
for i in wrong[:10]:
    print(f"  MISMATCH {lines[i][:70]}\n    python:    {expected[i][:120]}\n    satellite: {(got[i] if i < len(got) else '<missing>')[:120]}")
if harness.returncode != 0:
    print(f"the harness exited {harness.returncode}: {harness.stderr[:200]}")

table = subprocess.run([TABLE_CHECK], capture_output=True, text=True)
print("build/string_table_check:", table.stdout.strip().splitlines()[-1] if table.stdout.strip() else "<no output>",
      f"(exit {table.returncode})")
failed = bool(wrong) or harness.returncode != 0 or table.returncode != 0
print("all agree with Python and the header" if not failed else "FAILED")
sys.exit(1 if failed else 0)
