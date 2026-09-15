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
# .so), all at once on every hardware thread.
#
# Run from satellite-004:  python3 satellite-numbers/build_libraries.py
# Written 2026-09-14.

import concurrent.futures
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "build", "satellite-numbers")
CXX = os.environ.get("CXX", "g++")
FLAGS = ["-std=c++20", "-O2", "-Wall", "-Wextra", "-shared", "-fPIC"]
SHARED_HEADERS = [os.path.join(HERE, "number_row.hpp"), os.path.join(ROOT, "satellite", "machine", "machine_codes.hpp"),
                  os.path.join(ROOT, "strings", "string_method.hpp")]


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

    def build(job):
        folder, source, target = job
        if os.path.exists(target) and os.path.getmtime(target) >= max(os.path.getmtime(source), newest_header):
            return folder, None
        result = subprocess.run([CXX, *FLAGS, source, "-o", target], capture_output=True, text=True)
        return folder, (result.stderr.strip() or None) if result.returncode != 0 or result.stderr.strip() else None

    failed = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
        for folder, problem in pool.map(build, jobs):
            if problem:
                failed += 1
                print(f"--- {folder}\n{problem}")
    print(f"{len(jobs)} libraries, {failed} with errors or warnings")
    sys.exit(1 if failed else 0)


main()
