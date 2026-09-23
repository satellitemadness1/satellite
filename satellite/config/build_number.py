#!/usr/bin/env python3
# build_number.py -- raises arguments.build in satellite_config.hpp once for
# every build of satellite, and reads the author's rows for the checks.
#
#     python3 build_number.py <stamp> [--also <text>] -- <input file> ...   (make, before compiling)
#     python3 build_number.py <stamp> --verify                              (make, after linking)
#     python3 build_number.py <stamp> --same [--also <text>] -- <input> ... (one processor's build)
#     python3 build_number.py --print <row name>                            (check.sh)
#
# (the author, 2026-09-15) "a build number (start at... 0050) and increase the
# build number every single time that the application is built -- built right
# into the make file", kept in satellite_config.hpp as the row
#     arguments_vector.push_back({"arguments.build", 52, false, false});
#
# A REVISION IS A NEW COUNT OF BUILDS (the author, 2026-09-19): raising
# arguments.revision resets arguments.build to 1, so BUILD is "the Nth build of THIS
# revision" rather than of satellite for all time. The stamp records the revision each
# build was made under, and a make that sees a different one resets instead of raising.
# This is the ONE case that may write a build number BELOW the stamp's, so it is checked
# before the guard below that refuses exactly that -- see the note there.
#
# A BUILD IS A CHANGE TO WHAT SATELLITE IS MADE FROM. The stamp (.satellite_build,
# not in git) holds the number the last build used and a FINGERPRINT: a hash of
# every input file's contents (the build row's own number left out), plus the
# compiler and flags make passes with --also. The number rises only when that
# fingerprint changes. So a make with nothing to do, a retry after a failed or
# interrupted build, `make clean`, a touched file, an editor's swap file, or two
# makes at once never use a second number -- make still rebuilds what is missing.
# (Reviewed 2026-09-15: the first version compared timestamps and burned numbers
# in every one of those cases.)
#
# WHICH NUMBER A BUILD GETS, when the fingerprint has changed:
#   - the row equal to the stamp's number: raised by one;
#   - the row above it: the author set it by hand, and it is used as written;
#   - the row BELOW it: refused. That is an editor saving an old copy of the
#     config, and using it would give two builds one number.
# --verify, after linking, refuses a row that changed while the build ran.
#
# THE ROW IS A satellite_number (the author, 2026-09-16: "just make everything a
# satellite number"), so it has no ceiling -- but a C++ INTEGER LITERAL does. A row
# may be written plain (94) from -9,223,372,036,854,775,807 up to
# 9,223,372,036,854,775,807, a negative one with no u, or in quotes ("94",
# "99999999999999999999999999") at any length; a plain number past that is refused
# with the advice to quote it, as is a plain number with a leading 0 (C++ reads
# 0051 as octal, 41), in hex, or with digit separators. Inside quotes a leading 0
# is only a digit. Commented-out rows are ignored.

import datetime
import fcntl
import glob
import hashlib
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CONFIG = os.path.join(HERE, "satellite_config.hpp")
LONG_LONG_MAX = 2 ** 63 - 1
# The number the first build of a new revision gets. One, not zero: BUILD 0001 is a
# build that happened, and a released binary never shows a count of none.
FIRST_BUILD = 1

# A live row: push_back({"name", <number>, <flag>, <is_flag>}); the number spans group 2.
ROW = re.compile(r'push_back\(\s*\{\s*"([^"\n]*)"\s*,\s*([^,\n]*?)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}\s*\)')
DECIMAL = re.compile(r"-?[0-9]+[uUlL]*")
QUOTED = re.compile(r'"(-?[0-9]+)"')


def fail(message):
    sys.exit("build_number.py: satellite_config.hpp: " + message)


def comment_spans(text):
    """Every // and /* */ comment outside a string or character literal."""
    spans, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            spans.append((i, j))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            spans.append((i, j))
            i = j
        else:
            i += 1
    return spans


def live_rows(text):
    spans = comment_spans(text)
    rows = []
    for match in ROW.finditer(text):
        if any(start <= match.start() < end for start, end in spans):
            continue
        line = text.count("\n", 0, match.start()) + 1
        literal = match.group(2)
        quoted = QUOTED.fullmatch(literal)
        if quoted:
            digits = quoted.group(1)
        else:
            if not DECIMAL.fullmatch(literal):
                fail('line %d: %s is written %s; write it as decimal digits, in quotes when there are many'
                     % (line, match.group(1), literal))
            digits = literal.rstrip("uUlL")
            if re.fullmatch(r"-?0[0-9]+", digits):
                fail("line %d: %s is written %s, which C++ reads as octal; write it without the leading 0"
                     % (line, match.group(1), digits))
            # A NEGATIVE ROW MUST KEEP ITS MINUS IN C++ TOO (the review of b68d1a7,
            # 2026-09-17). The row's constructor takes any integer type since that
            # commit, so -1ULL and -4u are unsigned there and arrived as
            # 18446744073709551615 and 4294967292, with this script reading -1 and -4
            # and no check refusing either. -9223372036854775808 has no literal at
            # all: it is 9223372036854775808 negated, too large to be signed, so the
            # same happened -- it reads in quotes.
            if digits.startswith("-") and re.search("[uU]", literal):
                fail("line %d: %s is written %s, which C++ reads as unsigned, losing the minus; write it without the u"
                     % (line, match.group(1), literal))
            if not -LONG_LONG_MAX <= int(digits) <= LONG_LONG_MAX:
                fail('line %d: %s is %s, longer than a C++ integer can be written; write it in quotes: "%s"'
                     % (line, match.group(1), digits, digits))
        rows.append({"name": match.group(1), "number": int(digits), "span": match.span(2), "quoted": bool(quoted),
                     "flag": match.group(3) == "true", "is_flag": match.group(4) == "true", "line": line})
    return rows


def build_row(rows):
    found = [row for row in rows if row["name"] == "arguments.build"]
    if len(found) != 1:
        fail('needs exactly one live row arguments_vector.push_back({"arguments.build", <number>, false, false}); '
             "found %d" % len(found))
    if found[0]["number"] < 0:
        fail("line %d: arguments.build cannot be negative" % found[0]["line"])
    return found[0]


def revision_row(rows):
    """arguments.revision, or None when the author has not put one in the config.

    Unlike arguments.build this row is never written by this script -- it is the
    author's to set. It is only read, and only to notice that it changed."""
    found = [row for row in rows if row["name"] == "arguments.revision"]
    return found[0]["number"] if len(found) == 1 else None


def read_config():
    try:
        with open(CONFIG, encoding="utf-8", newline="") as config:
            return config.read()
    except OSError as error:
        fail("cannot be read: %s" % error.strerror)


def write_atomically(path, text):
    handle, temporary = tempfile.mkstemp(dir=os.path.dirname(os.path.abspath(path)), prefix="." + os.path.basename(path) + ".")
    with os.fdopen(handle, "w", encoding="utf-8", newline="") as out:
        out.write(text)
    if os.path.exists(path):
        os.chmod(temporary, os.stat(path).st_mode & 0o7777)
    os.replace(temporary, path)


def fingerprint(inputs, also, text):
    digest = hashlib.sha256(also.encode())
    row = build_row(live_rows(text))
    config_without_number = text[:row["span"][0]] + "#" + text[row["span"][1]:]
    for path in sorted({os.path.relpath(os.path.abspath(p), ROOT) for p in inputs}):
        digest.update(b"\0" + path.encode() + b"\0")
        path = os.path.join(ROOT, path)
        if path == CONFIG:
            digest.update(config_without_number.encode())
            continue
        try:
            with open(path, "rb") as source:
                digest.update(source.read())
        except OSError as error:
            sys.exit("build_number.py: %s: %s" % (path, error.strerror))
    return digest.hexdigest()


def read_stamp(stamp):
    """(number, fingerprint, revision).

    A stamp from the first version holds only a number; one from before the revision
    reset holds a number and a fingerprint. A missing revision reads as None, which
    means "do not reset" -- the first make after this change records the revision it
    finds and resets nothing, so upgrading the stamp never costs a build number."""
    try:
        words = open(stamp).read().split()
    except (OSError, UnicodeDecodeError):
        return None, None, None
    if not words or not words[0].isdigit():
        return None, None, None
    revision = None
    if len(words) > 2 and re.fullmatch(r"-?[0-9]+", words[2]):
        revision = int(words[2])
    return int(words[0]), (words[1] if len(words) > 1 else None), revision


def main():
    arguments = sys.argv[1:]
    if arguments[:1] == ["--print"] and len(arguments) == 2:
        for row in live_rows(read_config()):
            if row["name"] == arguments[1]:
                print(("true" if row["flag"] else "false") if row["is_flag"] else row["number"])
                return
        sys.exit("build_number.py: satellite_config.hpp has no live row %s" % arguments[1])
    if len(arguments) == 2 and arguments[1] == "--verify":
        used, _, _ = read_stamp(arguments[0])
        row = build_row(live_rows(read_config()))["number"]
        if used is not None and row != used:
            fail("arguments.build changed from %d to %d while the build ran (an editor saved an older copy?); "
                 "the binary may show either number -- run make again" % (used, row))
        return

    # --same: ONE PROCESSOR'S BUILD (make_support/055-cpus.mk) never raises the number. It
    # is BUILD N for that processor, so it asks whether the inputs are still exactly what
    # the ordinary BUILD N was made from, writes nothing, and refuses when they are not.
    same = len(arguments) >= 2 and arguments[1] == "--same"
    if same:
        del arguments[1]
    also = ""
    if len(arguments) >= 3 and arguments[1] == "--also":
        also = arguments[2]
        del arguments[1:3]
    if len(arguments) < 2 or arguments[1] != "--":
        sys.exit(__doc__ if __doc__ else "usage: see the top of build_number.py")
    stamp, inputs = arguments[0], arguments[2:]
    inputs += glob.glob(os.path.join(ROOT, "satellite-numbers", "*", "*.satellite.cpp"))

    with open(stamp + ".lock", "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        text = read_config()
        rows = live_rows(text)
        row = build_row(rows)
        revision = revision_row(rows)
        used, recorded, was = read_stamp(stamp)
        current = fingerprint(inputs, also, text)

        # THE REVISION CHANGED, so the count starts again. Asked before anything else
        # because this is the one path allowed to write a number BELOW the stamp's, and
        # the guard underneath refuses exactly that for every other reason.
        #
        # `was is None` means a stamp written before this rule existed: record the
        # revision, reset nothing. Otherwise the first make after upgrading would read a
        # missing revision as "different" and throw the count away for nothing.
        #
        # Raising the revision always changes the fingerprint too -- the row is in the
        # config, and fingerprint() blanks out only arguments.build -- so a revision
        # bump is always a real build. The test below does not lean on that.
        restarted = was is not None and revision is not None and revision != was

        if current == recorded and row["number"] == used and not restarted:
            return
        if same:
            fail(("no ordinary build has been made yet" if used is None else
                  "the sources changed since BUILD %s was made" % str(used).zfill(4)) +
                 ", and a processor's build never makes a new number -- run make first, then build the processors")

        if restarted:
            number = FIRST_BUILD
            written = '"%d"' % number if row["quoted"] else str(number)
            text = text[:row["span"][0]] + written + text[row["span"][1]:]
            write_atomically(CONFIG, text)
            print("satellite: REVISION %s -- BUILD restarts at %s"
                  % (str(revision).zfill(2), str(number).zfill(4)))
        else:
            if used is not None and row["number"] < used:
                fail("arguments.build is %d, but build %d was already used (an editor saved an older copy?); "
                     "write a number above %d, or delete .satellite_build to start over" % (row["number"], used, used))

            number = row["number"]
            if used is None or number == used:
                number += 1
                # Written back the way it was written, and quoted once it outgrows a
                # C++ integer: the row is a satellite_number, so it never stops rising.
                written = '"%d"' % number if row["quoted"] or number > LONG_LONG_MAX else str(number)
                text = text[:row["span"][0]] + written + text[row["span"][1]:]
                write_atomically(CONFIG, text)

        # THE REVISION IS RECORDED WHETHER OR NOT IT MOVED -- that is what makes the next
        # make able to tell. A config with no arguments.revision writes "-", which reads
        # back as None and resets nothing.
        write_atomically(stamp, "%d %s %s\n"
                         % (number, fingerprint(inputs, also, text),
                            "-" if revision is None else revision))
        if not restarted:
            print("satellite: BUILD %s (%s)" % (str(number).zfill(4), datetime.date.today().isoformat()))


if __name__ == "__main__":
    main()
