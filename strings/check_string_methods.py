#!/usr/bin/env python3
# satellite-004/strings/check_string_methods.py -- 004's string methods against 003's.
#
# Each case runs twice: through 004's method library (build/string_methods, which
# loads build/satellite-numbers/ like the interpreter does) and through 003's
# real `satl` in a one-line program. The answers must match; where 003 refuses,
# 004 must refuse too. Then a few Unicode cases, where 004 is expected to count
# CHARACTERS (003 counted bytes, so those are checked against Python instead).
#
# Run from satellite-004 after `make libraries build/string_methods`.
# Written 2026-09-14.
#
# AND THEN THROUGH THE INTERPRETER (M16, 2026-09-26): the same cases as programs run by
# build/satl, whose string methods are the language's own (bytecode/string_calls.cpp), not
# these libraries. Positions there count from 1, so at(n) is written at(n + 1) and
# substring(s, e) is substring(s + 1, e) -- the same character, the same piece. That satl
# runs with a HOME of its own, so its satellite.log is never the person's.

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SATL = os.path.normpath(os.path.join(ROOT, "old_versions", "second_satellite", "satl"))   # 003 07, archived 2026-09-15
S = "satellite.variable.string."

# (word, receiver, text arguments, positions, how 003 writes the call)
cases = [
    ("size", "Hello", [], []), ("size", "", [], []), ("empty", "", [], []), ("empty", "x", [], []),
    ("find(x)", "bolt,nut,washer", ["nut"], []), ("find(x)", "bolt", ["nut"], []), ("find(x)", "aaa", ["a"], []),
    ("contains(x)", "bolt,nut,washer", ["nut"], []), ("contains(x)", "bolt", ["x"], []), ("contains(x)", "bolt", [""], []),
    ("substring(start, end)", "Hello, World!", [], [0, 5]), ("substring(start, end)", "Hello", [], [2, 5]),
    ("substring(start, end)", "Hello", [], [5, 5]), ("substring(start, end)", "Hello", [], [3, 2]),
    ("substring(start, end)", "Hello", [], [0, 6]), ("substring(start, end)", "Hello", [], [-1, 2]),
    ("starts_with(x)", "error: bad", ["error"], []), ("starts_with(x)", "ok", ["error"], []), ("starts_with(x)", "ab", ["abc"], []),
    ("ends_with(x)", "program.satl", [".satl"], []), ("ends_with(x)", "a", ["ba"], []), ("ends_with(x)", "a", [""], []),
    ("lower", "HeLLo 42!", [], []), ("upper", "HeLLo 42!", [], []),
    ("split(separator)", "a,b,c", [","], []), ("split(separator)", "a,,b", [","], []), ("split(separator)", ",a,", [","], []),
    ("split(separator)", "abc", [""], []), ("split(separator)", "a--b--c", ["--"], []),
    ("trim", "   Ada   ", [], []), ("trim", "\tAda\t", [], []), ("trim", "    ", [], []),
    ("replace(a, b)", "a,b,c", [",", " and "], []), ("replace(a, b)", "aaa", ["a", "aa"], []),
    ("replace(a, b)", "abc", ["", "x"], []), ("replace(a, b)", "abc", ["z", "x"], []),
    ("append(x)", "Hello", [", World!"], []), ("clear", "Hello", [], []),
    ("at(n)", "Hello", [], [0]), ("at(n)", "Hello", [], [4]), ("at(n)", "Hello", [], [5]), ("at(n)", "Hello", [], [-1]),
    ("resolved", "plain text", [], []), ("string", "Ada", [], []),
]


def satl_call(word, receiver, texts, positions):
    name = word.split("(")[0]
    args = ", ".join([f'"{t}"' for t in texts] + [str(p) for p in positions])
    body = f'    satellite.variable.string s = "{receiver}"\n'
    if name in ("append", "clear"):
        body += f"    s.{name}({args})\n    satellite.console.display(s)\n"
    else:
        body += f"    satellite.console.display(s.{name}({args}))\n"
    program = ("satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n{\n"
               + body + "}\n")
    with tempfile.NamedTemporaryFile("w", suffix=".satl", delete=False) as f:
        f.write(program)
    result = subprocess.run([SATL, f.name], capture_output=True, text=True)
    os.unlink(f.name)
    return "REFUSED" if result.returncode != 0 else result.stdout.rstrip("\n")


US = "\x1f"   # field separator a tab inside the text cannot collide with
def case_line(w, r, t, p):
    return US.join([S + w, r, "|".join([str(len(t))] + t), ",".join(map(str, p))])


lines = [case_line(w, r, t, p) for w, r, t, p in cases]
unicode_cases = [("size", "мир", [], [], "3"), ("at(n)", "你好", [], [1], "好"),
                 ("upper", "мир abc", [], [], "мир ABC"),
                 ("split(separator)", "\U0001F30D\U0001F680", [""], [], "[\U0001F30D, \U0001F680]")]
lines += [case_line(w, r, t, p) for w, r, t, p, _ in unicode_cases]

got = subprocess.run([os.path.join(ROOT, "build", "string_methods")], input="\n".join(lines) + "\n",
                     capture_output=True, text=True, cwd=ROOT).stdout.split("\n")
wrong = 0
for i, (w, r, t, p) in enumerate(cases):
    want = satl_call(w, r, t, p)
    have = got[i]
    same = (want == "REFUSED" and have.startswith("REFUSED")) or want == have
    if not same:
        wrong += 1
        print(f"  DIFFERENT  {w}  receiver {r!r} args {t or p}:  003 {want!r}   004 {have!r}")
for j, (w, r, t, p, want) in enumerate(unicode_cases):
    have = got[len(cases) + j]
    if have != want:
        wrong += 1
        print(f"  UNICODE    {w}  {r!r}:  expected {want!r}   004 {have!r}")
print(f"{len(cases)} cases against 003's satl, {len(unicode_cases)} Unicode cases: "
      + ("all match" if wrong == 0 else f"{wrong} differ"))

SATL004 = os.path.join(ROOT, "build", "satl")
HOME004 = tempfile.mkdtemp(prefix="string_methods_home_")
RUNTIME004 = tempfile.mkdtemp(prefix="string_methods_xdg_")
os.chmod(RUNTIME004, 0o700)
ENV004 = {k: v for k, v in os.environ.items() if k not in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "GDK_BACKEND")}
ENV004.update(HOME=HOME004, XDG_RUNTIME_DIR=RUNTIME004, SATL_NO_WINDOW="1")


def quoted(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\t", "\\t") + '"'


def walker_call(word, receiver, texts, positions):
    name = word.split("(")[0]
    if name == "at":
        positions = [p + 1 for p in positions]
    if name == "substring":
        positions = [positions[0] + 1, positions[1]]
    args = ", ".join([quoted(t) for t in texts] + [str(p) for p in positions])
    body = f"    satellite.variable.string s = {quoted(receiver)}\n"
    if name in ("append", "clear"):
        body += f"    s.{name}({args})\n    satellite.console.display(s)\n"
    else:
        body += f"    satellite.console.display(s.{name}({args}))\n"
    program = ("satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n" + body +
               "    satellite.return(satellite)\n}\n")
    with tempfile.NamedTemporaryFile("w", suffix=".satl", delete=False, encoding="utf-8") as f:
        f.write(program)
    result = subprocess.run([SATL004, f.name], capture_output=True, text=True, env=ENV004,
                            stdin=subprocess.DEVNULL, timeout=20)
    os.unlink(f.name)
    if result.returncode != 0:
        return "REFUSED"
    shown = result.stdout.rstrip("\n")
    if shown.startswith("{") and shown.endswith("}"):     # 004 shows {"a", "b"} where 003 shows [a, b]
        shown = "[" + ", ".join(re.findall(r'"((?:[^"\\]|\\.)*)"', shown[1:-1])) + "]"
    return shown


# 004's upper() is every language's (string_case.hpp), where the 32-bit library is 003's a-z.
walker_unicode = [("size", "мир", [], [], "3"), ("at(n)", "你好", [], [1], "好"),
                  ("upper", "мир abc", [], [], "МИР ABC"),
                  ("split(separator)", "\U0001F30D\U0001F680", [""], [], "[\U0001F30D, \U0001F680]")]
walker_wrong = 0
for w, r, t, p in cases:
    want, have = satl_call(w, r, t, p), walker_call(w, r, t, p)
    if want != have:
        walker_wrong += 1
        print(f"  WALKER     {w}  receiver {r!r} args {t or p}:  003 {want!r}   004's satl {have!r}")
for w, r, t, p, want in walker_unicode:
    have = walker_call(w, r, t, p)
    if have != want:
        walker_wrong += 1
        print(f"  WALKER UNICODE  {w}  {r!r}:  expected {want!r}   004's satl {have!r}")
print(f"{len(cases)} cases and {len(walker_unicode)} Unicode cases through build/satl, positions from 1: "
      + ("all match" if walker_wrong == 0 else f"{walker_wrong} differ"))
shutil.rmtree(HOME004, ignore_errors=True)
shutil.rmtree(RUNTIME004, ignore_errors=True)
sys.exit(1 if wrong or walker_wrong else 0)
