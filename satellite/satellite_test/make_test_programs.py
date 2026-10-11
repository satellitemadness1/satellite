#!/usr/bin/env python3
# satellite/satellite_test/make_test_programs.py -- puts satellite.test's programs inside satl.
#
# The author, 2026-10-06: "We should write it into the satl binary, so you can type
# "satellite.test.full()" and satellite.test.speed() satellite.test.loss()". So the programs
# those words run are not files a user needs: they are text in the binary, written out to a
# private folder only for the moment a child satl runs one (test_run.hpp).
#
# READS, next to this script:
#   programs/manifest.tsv   one row a program: name, kind, what, exit_code, phrase, twin
#                           (a # line is a comment; an empty field is empty). name is the
#                           program's path under programs/, and the child is given its last part
#   programs/<name>         each program the manifest names
#   programs/<name less .satl>.out   an answers program's every line, exactly as a right run
#                           prints them -- one check a line, each beginning `ok `
# WRITES, next to this script:
#   test_programs.cpp       kTestPrograms[], one entry a row, in the manifest's order
#
# GENERATED AND NEVER TYPED, as words.tsv is: check.sh runs this and asks for the committed
# file byte for byte, so a program edited without running this cannot slip into a build.
#
# Run from anywhere:  python3 satellite/satellite_test/make_test_programs.py

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PROGRAMS = os.path.join(HERE, "programs")
KINDS = {"loss", "speed", "answers", "refused"}
# A raw string ends at )satl" -- a program holding that would end it early, so it is refused.
DELIMITER = "satl"
PASSES = "    satellite.variable.number passes = "
# THE FIRST LINE INSIDE satellite.main, whatever main takes -- and with satellite.capsule left off,
# for the program that is refused for leaving it off: the word rewrites its number, and the first
# such line anywhere is the one it rewrites (test_run.cpp's with_passes).
MAIN_OPENS = re.compile(r"\n(?:satellite\.capsule )?satellite\.main\([^\n]*\)[ \t]*\n\{[ \t]*\n" + re.escape(PASSES) +
                        r"[0-9]+\n")


def fail(why):
    sys.stderr.write("make_test_programs.py: " + why + "\n")
    sys.exit(1)


def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def raw_string(text, whose):
    if ")" + DELIMITER + '"' in text:
        fail(whose + " holds )" + DELIMITER + '", which would end its raw string')
    return 'R"%s(%s)%s"' % (DELIMITER, text, DELIMITER)


def read(path):
    with open(path, encoding="utf-8") as file:
        return file.read()


def main():
    rows = []
    seen = set()
    with open(os.path.join(PROGRAMS, "manifest.tsv"), encoding="utf-8") as manifest:
        for number, line in enumerate(manifest, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            fields = line.split("\t")
            if len(fields) != 6:
                fail("manifest.tsv line %d has %d fields, not 6" % (number, len(fields)))
            name, kind, what, exit_code, phrase, twin = fields
            where = "manifest.tsv line %d (%s)" % (number, name)
            if kind not in KINDS:
                fail("%s: kind %r is not one of %s" % (where, kind, sorted(KINDS)))
            if not what:
                fail(where + " says nothing of what it covers")
            if (kind == "speed") != bool(twin):
                fail(where + ": a speed program names its C++ twin, and nothing else does")
            if (kind == "refused") != bool(exit_code or phrase):
                fail(where + ": a refused program names its exit code and phrase, and nothing else does")
            if kind == "refused" and (not phrase or not exit_code.isdigit() or int(exit_code) == 0):
                fail(where + ": a refused program needs an exit code above 0 and a phrase")
            # A PHRASE IS MATCHED AGAINST THE REPORT WITH ITS LINES JOINED AND ITS SPACES RUN TOGETHER
            # (test_calls.cpp's flattened), so one with two spaces, or a space at an end, never matches.
            if phrase and ("  " in phrase or phrase != phrase.strip()):
                fail(where + ": its phrase has two spaces together, or a space at an end, so it could never match")
            base = os.path.basename(name)
            if not base.endswith(".satl") or base in seen:
                fail(where + ": every program is a .satl, and no two share the last part of their name")
            seen.add(base)
            path = os.path.join(PROGRAMS, name)
            if not os.path.isfile(path):
                fail(where + " names a program that is not in programs/")
            text = read(path)
            opened = MAIN_OPENS.search(text)
            if opened is None or text.find("\n" + PASSES) != opened.start() + opened.group(0).find("\n" + PASSES):
                fail(where + ": its first line inside satellite.main is not `" + PASSES + "<N>`, or one comes before it")
            expected = None
            if kind == "answers":
                out = os.path.splitext(path)[0] + ".out"
                if not os.path.isfile(out):
                    fail(where + ": an answers program has its right run's lines beside it, as " +
                         os.path.basename(out))
                expected = read(out)
                lines = expected.split("\n")
                if not expected.endswith("\n") or any(not l.startswith("ok ") for l in lines[:-1]):
                    fail(os.path.basename(out) + ": every line of a right run begins `ok `, and the last ends")
            rows.append((base, kind, what, text, expected, int(exit_code or 0), phrase, twin, name))

    # EVERY TEST HAS SOMETHING TO RUN: with no loss program, loss() would print one number and
    # answer true (the review, 2026-10-06).
    for kind in sorted(KINDS):
        if not any(r[1] == kind for r in rows):
            fail("manifest.tsv has no %s program, so its test would check nothing" % kind)

    out = []
    out.append("// satellite/satellite_test/test_programs.cpp -- GENERATED by make_test_programs.py from")
    out.append("// programs/manifest.tsv and programs/. Do not edit: edit those and run it again.")
    out.append("")
    out.append('#include "test_programs.hpp"')
    out.append('#include "speed_twins.hpp"')
    out.append("")
    out.append("namespace satellite004 {")
    out.append("")
    out.append("const TestProgram kTestPrograms[] = {")
    for base, kind, what, text, expected, exit_code, phrase, twin, name in rows:
        out.append("    {%s, TestKind::%s, %s," % (c_string(base), kind, c_string(what)))
        out.append("     " + raw_string(text, name) + ",")
        out.append("     " + (raw_string(expected, name + "'s .out") if expected is not None else "nullptr") + ",")
        out.append("     %d, %s, %s}," % (exit_code, c_string(phrase), twin if twin else "nullptr"))
    out.append("};")
    out.append("")
    out.append("const std::size_t kTestProgramCount = sizeof kTestPrograms / sizeof kTestPrograms[0];")
    out.append("")
    out.append("} // namespace satellite004")
    out.append("")
    with open(os.path.join(HERE, "test_programs.cpp"), "w", encoding="utf-8") as written:
        written.write("\n".join(out))
    kinds = {k: sum(1 for r in rows if r[1] == k) for k in sorted(KINDS)}
    checks = sum(r[4].count("\n") for r in rows if r[1] == "answers") + kinds["refused"]
    print("test_programs.cpp: %d programs (%s), %d checks for full()" %
          (len(rows), ", ".join("%d %s" % (kinds[k], k) for k in sorted(KINDS)), checks))


if __name__ == "__main__":
    main()
