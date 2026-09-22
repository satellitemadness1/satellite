#!/usr/bin/env python3
# satellite-004/words/make_words.py -- builds satellite-004's word numbers.
#
# 004'S NUMBERING, SETTLED 2026-09-14. The author: "Just settle on a particular
# number for each of the commands, just remove GUI commands." Settled as:
#
#   004's numbers = 003 06's words in 003's order, minus the removed words,
#   numbered first-available at every level (below).
#
# Why 003's numbers and not new ones: all 371 were already checked against
# words.def one by one, they already follow the author's scheme for methods (a
# string's methods are 1 6 1 1, 1 6 1 2, ...), and the new numbers mentioned
# that day disagreed with each other (number was "variable.2" and "1 5 2";
# capsule and console were both 1 2).
#
# THE SCHEME IS FIRST AVAILABLE NUMBER, AT EVERY LEVEL (the author, 2026-09-14):
# "first_available_number.first_available_number.first_available_number ...
# that's all the numbering scheme is." So after removing words, each parent's
# children are numbered again 1, 2, 3 ... in 003's order, with no gaps; a bare
# shape keeps 0. satellite.window was 1 24, so satellite.constructor moves from
# 1 25 to 1 24. This was done ONCE, before 004 wrote any number into any
# program or .satc. FROM HERE ON THE NUMBERS ARE FROZEN: a word removed later
# leaves its number empty forever, and a new word takes the next free number.
#
# WHY GENERATED AND NEVER TYPED: one mistyped digit would make a program call the
# wrong library. The rows are read from 003's own compiled registry by walking
# `satl --words`, which answers from the words.def the binary was built from.
#
# 004'S OWN WORDS ARE APPENDED, NEVER MIXED IN. words_004.tsv, next to this script,
# lists every word 004 added after the renumbering -- `numbers<TAB>path`, in the
# order they were added -- and they go on the END of words.tsv, after every word
# that came from 003. That is not tidiness: a word's 16-bit code is 4097 plus its
# ROW (satellite/bytecode/make_word_codes.py), so a word inserted anywhere else
# would move the code of every word after it. Each one must take the NEXT FREE
# number under its parent -- first available, the author's scheme -- and anything
# else stops this script rather than writing a table with a gap or a clash. The
# first was satellite.variable.percentage, 1 6 16 (the author, 2026-09-17).
#
# WRITES, next to this script:
#   words_003.tsv        every 003 06 word and its number, for reference (371 rows)
#   words.tsv            004's words: numbers <TAB> path <TAB> 003's number
#   satellite_words.hpp  004's words as a C++ table, the removed words, and the
#                        next free number under every word
#
# Run from anywhere:  python3 satellite-004/words/make_words.py
# The deepest word has 6 numbers, which is why a row holds 6 inline; a next-free
# number under a 6-deep word is 7 long.

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
# 003 07's satl, archived in old_versions/second_satellite/ since 2026-09-15.
SATL = os.path.normpath(os.path.join(HERE, "..", "old_versions", "second_satellite", "satl"))

# Removed from 004, with the author's reason. A path here removes the word AND
# every word under it.
REMOVED = {
    "satellite.window": "GUI commands removed (the author, 2026-09-14)",
    "satellite.variable.window": "GUI commands removed (the author, 2026-09-14)",
}

# `satl --words` prints its children as "  <path>  <n> <n> <n>". A path may hold
# spaces -- set(k, v) -- so the path ends at the run of two or more spaces.
# Read through a pipe, never /dev/null: /dev/null makes satl hand the console over.
ROW = re.compile(r"^  (satellite.*?)\s{2,}((?:\d+ ?)+)$")
NEXT = re.compile(r"the next number free under it is ((?:\d+ ?)+?)(?:,|$)")


def words_of(path):
    out = subprocess.run([SATL, "--words", path], capture_output=True, text=True).stdout
    children = []
    for line in out.splitlines():
        match = ROW.match(line.rstrip())
        if match and match.group(1) != path:
            children.append((match.group(1), [int(n) for n in match.group(2).split()]))
    free = NEXT.search(out)
    return children, ([int(n) for n in free.group(1).split()] if free else None)  # (003's next free, unused for 004)


def removed(path):
    bare = re.sub(r"\(.*\)$", "", path)
    return any(bare == gone or bare.startswith(gone + ".") for gone in REMOVED)


def write_rows(hpp, name, rows, width):
    for path, numbers in rows:
        padded = numbers + [0] * (width - len(numbers))
        hpp.write(f'    {{"{path}", {{{", ".join(map(str, padded))}}}, {len(numbers)}}},\n')


def main():
    if not os.access(SATL, os.X_OK):
        sys.exit(f"make_words.py: no satl at {SATL} -- build 003 first")
    rows, next_free, pending, seen = [("satellite", [1])], {}, ["satellite"], {"satellite"}
    while pending:
        path = pending.pop(0)
        children, free = words_of(path)
        if free:
            next_free[path] = free
        for child, numbers in children:
            rows.append((child, numbers))
            walkable = re.sub(r"\(.*\)$", "", child)       # find(x) -> find, to ask for its children
            if walkable not in seen and not child.endswith("()"):
                seen.add(walkable)
                pending.append(walkable)
    rows.sort(key=lambda row: row[1])

    with open(os.path.join(HERE, "words_003.tsv"), "w") as tsv:
        for path, numbers in rows:
            tsv.write(" ".join(map(str, numbers)) + "\t" + path + "\n")

    dropped = [(path, numbers) for path, numbers in rows if removed(path)]
    new_of = {(1,): [1]}                      # 003 number -> 004 number
    used = {}                                 # 003 parent -> numbers handed out under it
    kept, old_of = [], {}
    for path, numbers in rows:
        if removed(path) or len(numbers) == 1:
            continue
        parent = tuple(numbers[:-1])
        if parent not in new_of:
            sys.exit(f"make_words.py: {path} has no parent row {parent}")
        if numbers[-1] == 0:
            last = 0                          # the bare shape keeps 0
        else:
            used[parent] = used.get(parent, 0) + 1
            last = used[parent]
        new = new_of[parent] + [last]
        new_of.setdefault(tuple(numbers), new)
        kept.append((path, new))
        old_of[path] = numbers
    kept.insert(0, ("satellite", [1]))
    old_of["satellite"] = [1]

    # 004's own words, appended in the order they were added (see the header).
    #
    # THE FILE IS TYPED BY HAND, so every row is checked against the table it joins
    # (the review of ab01a74, 2026-09-17, found each of these getting through):
    #   - numbers, a tab, and a path with no blanks at either end -- an empty path
    #     or an editor's trailing space was a second word with a code of its own;
    #   - the parent NUMBERS must be the word the PATH names as its parent, so
    #     `1 5 11 satellite.variable.foo` is refused: 1 5 is satellite.console, and
    #     that one mistyped digit is what this file exists to stop. An argument
    #     form sits under the word itself (string(x) is under string), as in 003;
    #   - nothing goes under a bare shape -- `number()` holds no words;
    #   - a bare shape keeps 0, and only a bare shape does: in 003's table the last
    #     number is 0 exactly when the path is its parent's and then (). 004 had
    #     refused every 0, and told the author to write 1 instead.
    # Codes already given out are not guarded here: an insert still moves every
    # code after it, and SATC.md section 2's word-list digest is what makes that
    # loud, where files are read back (PROGRESS.md).
    def refuse(why):
        sys.exit(f"make_words.py: words_004.tsv line {line_number}: {why}")

    taken = {tuple(numbers): path for path, numbers in kept}
    paths = {path for path, _ in kept}
    added = os.path.join(HERE, "words_004.tsv")
    for line_number, line in enumerate(open(added, encoding="utf-8") if os.path.exists(added) else [], 1):
        if not line.strip():
            continue
        fields = line.rstrip("\n").split("\t")
        if len(fields) != 2 or not re.fullmatch(r"[0-9]+(?: [0-9]+)*", fields[0]) or \
                not fields[1] or fields[1] != fields[1].strip():
            refuse("write numbers, a tab, then the path")
        numbers, path = [int(n) for n in fields[0].split()], fields[1]
        parent = tuple(numbers[:-1])
        under = [n[-1] for n in taken if len(n) == len(numbers) and n[:-1] == parent and n[-1] != 0]
        if path in paths or tuple(numbers) in taken:
            refuse(f"{path} {fields[0]} is already a word")
        if parent not in taken:
            refuse(f"{path} has no parent word {' '.join(map(str, parent))}")
        if taken[parent].endswith(")"):
            refuse(f"{path} cannot go under {taken[parent]}, which holds no words")
        bare = re.sub(r"\(.*\)$", "", path)
        if taken[parent] not in (bare.rpartition(".")[0], bare):
            refuse(f"{path} is numbered under {' '.join(map(str, parent))}, which is {taken[parent]}")
        if numbers[-1] == 0 and path != taken[parent] + "()":
            refuse(f"{path} is not a bare shape, so it cannot keep 0 -- only {taken[parent]}() does")
        if numbers[-1] != 0 and path == taken[parent] + "()":
            refuse(f"{path} is a bare shape, which keeps 0: {' '.join(map(str, parent))} 0")
        if numbers[-1] != 0 and numbers[-1] != max(under, default=0) + 1:
            refuse(f"{path} must take the next free number, "
                   f"{' '.join(map(str, list(parent) + [max(under, default=0) + 1]))}")
        kept.append((path, numbers))
        old_of[path] = []
        taken[tuple(numbers)] = path
        paths.add(path)
    # the next free number under every kept word: one past what it hands out today
    children = {}
    for path, new in kept:
        if len(new) > 1 and new[-1] != 0:
            children[tuple(new[:-1])] = max(children.get(tuple(new[:-1]), 0), new[-1])
    next_free = {path: new + [children.get(tuple(new), 0) + 1] for path, new in kept if not path.endswith(")")}
    deepest = max(len(numbers) for _, numbers in kept)
    with open(os.path.join(HERE, "words.tsv"), "w") as tsv:
        for path, numbers in kept:
            tsv.write(" ".join(map(str, numbers)) + "\t" + path + "\t" + " ".join(map(str, old_of[path])) + "\n")

    with open(os.path.join(HERE, "satellite_words.hpp"), "w") as hpp:
        hpp.write("#pragma once\n")
        hpp.write("// satellite-004's word numbers. GENERATED by make_words.py -- never edit by hand.\n")
        hpp.write("// First available number at every level, renumbered once without the removed words\n")
        hpp.write("// (settled 2026-09-14) and frozen from then on. make_words.py says why.\n\n")
        hpp.write("namespace satellite004 {\n\n")
        hpp.write(f"inline constexpr unsigned int kWordDepth = {deepest};\n")
        hpp.write(f"inline constexpr unsigned int kNextFreeDepth = {deepest + 1};\n\n")
        hpp.write("struct WordRow {\n    const char *path;\n    unsigned long long int numbers[kWordDepth];\n    unsigned int depth;\n};\n\n")
        hpp.write("inline constexpr WordRow kWords[] = {\n")
        write_rows(hpp, "kWords", kept, deepest)
        hpp.write("};\n\n// Removed words, with the numbers they had in 003 06.\n")
        hpp.write("inline constexpr WordRow kRemovedWords[] = {\n")
        write_rows(hpp, "kRemovedWords", dropped, deepest)
        hpp.write("};\n\n// The next free number under each word: where a new child word goes.\n")
        hpp.write("struct NextFree {\n    const char *under;\n    unsigned long long int numbers[kNextFreeDepth];\n    unsigned int depth;\n};\n\n")
        hpp.write("inline constexpr NextFree kNextFree[] = {\n")
        write_rows(hpp, "kNextFree", [(p, next_free[p]) for p, _ in kept if p in next_free], deepest + 1)
        hpp.write("};\n\n} // namespace satellite004\n")

    print(f"003 06: {len(rows)} words | 004: {len(kept)} words, {len(dropped)} removed "
          f"({', '.join(p for p, _ in dropped)}) | deepest {deepest} numbers")


main()
