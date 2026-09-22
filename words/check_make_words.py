#!/usr/bin/env python3
# satellite-004/words/check_make_words.py -- make_words.py against rows typed by hand.
#
# words_004.tsv is typed by hand, and make_words.py is all that stands between a
# mistyped digit and a word number frozen forever. Each case below runs the REAL
# make_words.py walking 003's REAL satl, in a copy of their folders under
# build/check_make_words/, so the committed tables are never rewritten.
#
# THE FIRST CASE IS THE COMMITTED words_004.tsv, and it must regenerate every table
# byte-identical to the committed ones. That is what makes the rest trustworthy:
# the copy is proven to be the real thing before any row is judged in it.
#
# 003's satl is gitignored (old_versions/second_satellite/satl); without it this
# says so and exits 2, and check.sh counts that as skipped, not passed.
#
#     python3 words/check_make_words.py [make_words.py to check]
#
# Exit 0 every case agrees, 1 a case disagrees, 2 no 003 satl.

import concurrent.futures
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SATL = os.path.join(ROOT, "old_versions", "second_satellite", "satl")
SCRIPT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(HERE, "make_words.py")
WORK = os.path.join(ROOT, "build", "check_make_words")
COMMITTED = open(os.path.join(HERE, "words_004.tsv"), encoding="utf-8").read()
TABLES = ("words.tsv", "words_003.tsv", "satellite_words.hpp")


def next_under(parent):
    """The next free number under a word, read from the committed words.tsv."""
    taken = [0]
    for line in open(os.path.join(HERE, "words.tsv"), encoding="utf-8"):
        numbers = [int(n) for n in line.split("\t")[0].split()]
        if numbers[:-1] == list(parent):
            taken.append(numbers[-1])
    return max(taken) + 1


# THE NEXT FREE NUMBER UNDER satellite.variable, READ, NEVER TYPED. These cases used
# to write 1 6 17 by hand as "the next free number", and on 2026-09-18
# satellite.variable.infinity took it (SATELLITE_INFINITY.md, INF-1): "a gap" stopped
# being a gap and failed, and two others went on passing for a reason that was no
# longer the one they named. Read from the table, no word taking a number can do that.
NEXT = next_under((1, 6))

# (what it proves, rows after the committed ones, what make_words.py must say)
# A REFUSAL is its exit 1 and a sentence of make_words.py's own -- never a traceback.
# An ACCEPTANCE is exit 0 with the rows written at the end of words.tsv as typed.
REFUSED = [
    ("numbers under another word than the name says",
     "1 5 11\tsatellite.variable.foo\n", "satellite.variable.foo is numbered under 1 5, which is satellite.console"),
    ("an empty path", f"1 6 {NEXT}\t\n", "write numbers, a tab, then the path"),
    ("a path with a trailing space", f"1 6 {NEXT}\tsatellite.variable.percentage \n", "write numbers, a tab, then the path"),
    ("numbers that are not numbers", "1 6 x\tsatellite.variable.y\n", "write numbers, a tab, then the path"),
    ("a word under a bare shape", "1 6 4 0 1\tsatellite.variable.number().x\n",
     "cannot go under satellite.variable.number(), which holds no words"),
    ("a bare shape given a number", "1 6 16 1\tsatellite.variable.percentage()\n",
     "satellite.variable.percentage() is a bare shape, which keeps 0: 1 6 16 0"),
    ("a 0 that is not a bare shape", "1 6 16 0\tsatellite.variable.percentage(x)\n",
     "is not a bare shape, so it cannot keep 0"),
    ("a gap", f"1 6 {NEXT + 1}\tsatellite.variable.skipped\n", f"must take the next free number, 1 6 {NEXT}"),
    ("a path that is already a word", f"1 6 {NEXT}\tsatellite.variable.percentage\n", "is already a word"),
    ("numbers that are already a word", "1 6 16\tsatellite.variable.other\n", "is already a word"),
]
ACCEPTED = [
    ("a bare shape at 0, then a word and its child",
     "1 6 16 0\tsatellite.variable.percentage()\n1 6 16 1\tsatellite.variable.percentage.round\n"
     "1 6 16 1 1\tsatellite.variable.percentage.round.up\n"),
    ("an argument form under the word itself", "1 6 16 1\tsatellite.variable.percentage(x)\n"),
    # 1 5 11 AND NOT 1 5 10 SINCE 2026-09-22: GTK-17 put satellite.console.new at
    # 1 5 10, the first word 004 has put under satellite.console, and these two
    # cases had named that number as free. The next milestone that adds
    # a word under satellite.console takes 1 5 11 and moves these again.
    ("a word numbered under its own parent", "1 5 11\tsatellite.console.foo\n"),
]


def run(name, rows):
    """make_words.py in its own copy; answers (exit, stderr, the copy's words folder)."""
    tree = os.path.join(WORK, name)
    shutil.rmtree(tree, ignore_errors=True)
    words = os.path.join(tree, "words")
    os.makedirs(words)
    os.makedirs(os.path.join(tree, "old_versions", "second_satellite"))
    os.symlink(SATL, os.path.join(tree, "old_versions", "second_satellite", "satl"))
    shutil.copy(SCRIPT, os.path.join(words, "make_words.py"))
    with open(os.path.join(words, "words_004.tsv"), "w", encoding="utf-8") as tsv:
        tsv.write(COMMITTED + rows)
    # stdin is a PIPE, never /dev/null: /dev/null makes 003's satl hand the console over.
    done = subprocess.run([sys.executable, os.path.join(words, "make_words.py")], stdin=subprocess.PIPE,
                          capture_output=True, text=True)
    return done.returncode, done.stderr.strip(), words


def main():
    if not os.access(SATL, os.X_OK):
        print(f"check_make_words.py: skipped -- no 003 satl at {SATL}")
        return 2

    cases = [("committed", "")] + [(f"refused_{k}", rows) for k, (_, rows, _) in enumerate(REFUSED)] + \
            [(f"accepted_{k}", rows) for k, (_, rows) in enumerate(ACCEPTED)]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        answers = dict(zip((name for name, _ in cases), pool.map(lambda case: run(*case), cases)))

    failed = 0

    def expect(what, agrees, detail):
        nonlocal failed
        print(("  ok    " if agrees else "  FAIL  ") + what + ("" if agrees else " -- " + detail))
        failed += not agrees

    code, err, words = answers["committed"]
    same = code == 0 and all(open(os.path.join(words, table), "rb").read() ==
                             open(os.path.join(HERE, table), "rb").read() for table in TABLES)
    expect("make_words.py regenerates the committed tables byte-identical", same, f"exit {code}: {err}")

    for k, (what, _, said) in enumerate(REFUSED):
        code, err, _ = answers[f"refused_{k}"]
        expect("make_words.py refuses " + what, code == 1 and err.startswith("make_words.py: words_004.tsv line ")
               and said in err, f"exit {code}: {err.splitlines()[-1] if err else '(nothing on stderr)'}")

    for k, (what, rows) in enumerate(ACCEPTED):
        code, err, words = answers[f"accepted_{k}"]
        written = open(os.path.join(words, "words.tsv"), encoding="utf-8").read() if code == 0 else ""
        wanted = "".join(row + "\t\n" for row in rows.splitlines())
        expect("make_words.py accepts " + what, code == 0 and written.endswith(wanted),
               f"exit {code}: {err.splitlines()[-1] if err else 'the rows are not at the end of words.tsv'}")

    print(f"{len(cases)} cases, {failed} failed")
    return 1 if failed else 0


sys.exit(main())
