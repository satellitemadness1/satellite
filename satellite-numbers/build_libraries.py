#!/usr/bin/env python3
# satellite-004/satellite-numbers/build_libraries.py -- builds every numbered library.
#
# EVERY FOLDER HERE IS ONE WORD, NAMED BY THE WORD (the author: "the name of the
# method will only exist in the directory structure of the files"):
#
#     satellite-numbers/<folder>/<folder>.satellite.cpp  ->  build/satellite-numbers/<numbers>.so
#
# The folder is the word's path EXACTLY as words/words.tsv writes it, arguments
# and all (the author, 2026-09-14: "it becomes satellite.variable.string.find(x)"):
#     satellite-numbers/satellite.variable.string.find(x)/satellite.variable.string.find(x).satellite.cpp
#     satellite-numbers/satellite.variable.string.substring(start, end)/...  (a space is fine)
# That is unique, because a word is numbered once per argument list.
#
# THE NUMBERS COME ONLY FROM words.tsv, never from the folder, so a library's
# file name always matches 004's numbering. A folder that is not a word is an
# error, not a silent skip.
#
# Why a script and not make rules: make reads "name(member)" as an archive member,
# and every word with arguments has brackets in its name.
#
# Rebuilds only what changed (the source or a shared header is newer than the
# .so), as many at once as make's -j -- AND EVERY LIBRARY WHEN THE COMPILER
# OR FLAGS CHANGED (PLAN M0.5). make hands this script its own CXX, CXXFLAGS and
# LDFLAGS as SATELLITE_CXX, SATELLITE_CXXFLAGS and SATELLITE_LDFLAGS
# (make_support/050-build.mk), and build/satellite-numbers/.built_with records the
# command the libraries there were built with. Before M0.5 it read CXX from the
# environment, hardcoded -O2 and rebuilt on times only, so satl and its libraries
# could come from two compilers with nothing saying so.
#
# LD_RUN_PATH IS NEVER PASSED TO THE COMPILER: GNU ld writes it into a library as
# an RPATH naming directories in one home (make_support/048-link.mk).
#
# A LIBRARY IS WRITTEN BESIDE ITS NAME AND RENAMED INTO IT, so a satl starting
# while this runs never dlopens half a file; and a .so no word in words.tsv names
# any more is removed, because satl loads every .so in the folder.
#
# Silent when there was nothing to build, so a second `make` says nothing.
#
# Run from satellite-004:  python3 satellite-numbers/build_libraries.py
# Written 2026-09-14.

import concurrent.futures
import glob
import os
import re
import shlex
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
# ONE PROCESSOR'S BUILD (make_support/055-cpus.mk) puts its libraries beside its own satl,
# build/cpu/<processor>/satellite-numbers/ -- satl loads the folder beside itself.
OUT = os.path.join(ROOT, os.environ.get("SATELLITE_NUMBERS_OUT") or os.path.join("build", "satellite-numbers"))
# A CXX of several words ("ccache clang++", "env clang++") is a command, not a name.
CXX = shlex.split(os.environ.get("SATELLITE_CXX") or os.environ.get("CXX") or "c++")
# The compiler's own first line, from make: clang-current is repointed at each
# rebuilt clang, so the same CXX string can be a different compiler.
CXX_VERSION = os.environ.get("SATELLITE_CXX_VERSION", "")
JOBS = int(os.environ["SATELLITE_JOBS"]) if os.environ.get("SATELLITE_JOBS", "").isdigit() else os.cpu_count()
CXXFLAGS = shlex.split(os.environ.get("SATELLITE_CXXFLAGS", "-std=c++20 -O2 -Wall -Wextra"))
LDFLAGS = shlex.split(os.environ.get("SATELLITE_LDFLAGS", ""))
FLAGS = [*CXXFLAGS, "-shared", "-fPIC", *LDFLAGS]
BUILT_WITH = os.path.join(OUT, ".built_with")
ENVIRONMENT = {name: value for name, value in os.environ.items() if name != "LD_RUN_PATH"}
# EVERY HEADER A LIBRARY READS, or a change to it rebuilds nothing: machine_facts.hpp was
# missing, and moving arguments.cores onto physical cores left all 32 fact libraries
# answering the old way from a build that reported success (the error sweep, 2026-09-25).
# `grep -ho '#include "[^"]*"' satellite-numbers/*/*.satellite.cpp satellite-numbers/*.hpp`
# lists them.
SHARED_HEADERS = [os.path.join(HERE, "number_row.hpp"), os.path.join(ROOT, "satellite", "machine", "machine_codes.hpp"),
                  os.path.join(ROOT, "strings", "string_method.hpp"), os.path.join(HERE, "directory_words.hpp"),
                  os.path.join(HERE, "machine_facts.hpp"), os.path.join(HERE, "feedback_book.hpp"),
                  os.path.join(ROOT, "satellite", "config", "machine_probe.hpp"),
                  os.path.join(ROOT, "satellite", "config", "config_file.hpp"),
                  os.path.join(ROOT, "satellite", "machine", "filesystems.hpp")]


def folder_of(path):
    return path


def main():
    numbers_of = {}
    for line in open(os.path.join(ROOT, "words", "words.tsv")):
        numbers, path = line.rstrip("\n").split("\t")[:2]
        numbers_of[folder_of(path)] = numbers.replace(" ", ".")

    jobs, unknown = [], []
    for folder in sorted(os.listdir(HERE)):
        source = os.path.join(HERE, folder, folder + ".satellite.cpp")
        if not os.path.isdir(os.path.join(HERE, folder)):
            continue
        if folder not in numbers_of:
            unknown.append(folder)
            continue
        if not os.path.isfile(source):
            unknown.append(folder + " (no " + folder + ".satellite.cpp)")
            continue
        jobs.append((folder, source, os.path.join(OUT, numbers_of[folder] + ".so")))
    if unknown:
        sys.exit("build_libraries.py: not a word in words/words.tsv: " + ", ".join(unknown))

    os.makedirs(OUT, exist_ok=True)
    newest_header = max(os.path.getmtime(h) for h in SHARED_HEADERS)

    command = shlex.join([*CXX, *FLAGS]) + ("  [" + CXX_VERSION + "]" if CXX_VERSION else "")
    try:
        with open(BUILT_WITH) as recorded:
            same_compiler = recorded.read() == command
        why = "" if same_compiler else " (a different compiler or flags)"
    except OSError:
        same_compiler, why = False, ""
    if not same_compiler:
        # Recorded as unknown FIRST, so an interrupted rebuild is a rebuild again.
        if os.path.exists(BUILT_WITH):
            os.remove(BUILT_WITH)

    wanted = {os.path.basename(target) for _, _, target in jobs}
    for stale in sorted(set(os.listdir(OUT)) - wanted):
        if stale.endswith(".so") or stale.endswith(".so.tmp"):
            if os.path.isdir(os.path.join(OUT, stale)) and not os.path.islink(os.path.join(OUT, stale)):
                sys.exit(f"build_libraries.py: {os.path.join(OUT, stale)} is a folder, not a library, and satl would "
                         f"refuse to start over it; remove it")
            os.remove(os.path.join(OUT, stale))
            print(f"build_libraries.py: removed {stale}, which no word in words/words.tsv names")

    def build(job):
        folder, source, target = job
        if same_compiler and os.path.exists(target) and \
                os.path.getmtime(target) >= max(os.path.getmtime(source), newest_header):
            return folder, False, None
        temporary = target + ".tmp"
        try:
            result = subprocess.run([*CXX, *FLAGS, source, "-o", temporary], capture_output=True, text=True,
                                    env=ENVIRONMENT)
        except OSError as error:
            return folder, True, "cannot run %s: %s" % (shlex.join(CXX), error.strerror)
        problem = result.stderr.strip() or ("exit status %d" % result.returncode if result.returncode else None)
        # A LIBRARY THAT WARNED IS NOT KEPT (review of M0.5): kept, it was newer than
        # its source, and the next make said nothing. Left out, the next make builds it
        # again and says the warning again.
        if problem is None:
            os.replace(temporary, target)
        elif os.path.exists(temporary):
            os.remove(temporary)
        return folder, True, problem

    failed = built = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=JOBS) as pool:
        for folder, compiled, problem in pool.map(build, jobs):
            built += compiled
            if problem:
                failed += 1
                print(f"--- {folder}\n{problem}")
    if not failed:
        with open(BUILT_WITH, "w") as record:
            record.write(command)
    if built or failed:
        print(f"{len(jobs)} libraries, {built} built{why}, "
              f"{failed} with errors or warnings")
    sys.exit(1 if failed else 0)


main()
