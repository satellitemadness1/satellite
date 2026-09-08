#!/usr/bin/env python3
"""Read `src/satellite_words/words.def` and answer the three things a
generator needs about a node: its identifier, its path text and its number.

WHY THIS IS PARSED RATHER THAN MEASURED. `nodes.tsv` is the output of a
throwaway that LINKS the interpreter, so it can say which rows have a handler
-- but it carries no identifiers, and `help.def` has to name every node by the
same `NodeId` enumerator words.def declares. Parsing the registry is the only
way to get that name without a second list of them, and a second list of 269
identifiers is exactly the drift M18 exists to end.

The parse is checked rather than trusted: gen.py diffs the paths and numbers
this produces against nodes.tsv, which was measured by a binary, and stops if
they disagree.
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
DEF = os.path.join(os.path.dirname(HERE), "src", "satellite_words", "words.def")

ROW = re.compile(r'^SAT_NODE\(\s*(\w+)\s*,\s*(\w+)\s*,\s*"((?:[^"\\]|\\.)*)"\s*,'
                 r'\s*(SAT_NUMBERED|SAT_BARE)\s*\)')


class Node:
    def __init__(self, ident, parent, text, kind, order):
        self.ident = ident
        self.parent = parent
        self.text = text
        self.kind = kind
        self.order = order          # 1-based position in the file == PathId
        self.number = ""            # "1 5 1"
        self.path = ""              # "satellite.console.display"

    @property
    def spelling(self):
        return self.text.split("(", 1)[0]

    @property
    def arguments(self):
        i = self.text.find("(")
        return "" if i < 0 else self.text[i:]


def read():
    """Every node, in file order, keyed by identifier. Order is the PathId."""
    nodes = {}
    order = 0
    with open(DEF) as f:
        for line in f:
            m = ROW.match(line.strip())
            if not m:
                continue
            order += 1
            nodes[m.group(2)] = Node(m.group(2), m.group(1), m.group(3),
                                     m.group(4), order)

    # THE NUMBER IS A POSITION AMONG SIBLINGS -- WORD_NUMBERS §1.2, and a bare
    # `()` child is position 0 rather than a piece of notation (§1.3). So the
    # count starts at 0 for a parent whose first child is bare and at 1 for one
    # whose children all carry spellings, which is what makes `1 5 1` display
    # and `1 5 0` the shape with no arguments.
    children = {}
    for n in sorted(nodes.values(), key=lambda n: n.order):
        if n.parent == "NONE":
            n.number = "1"
            n.path = n.text
            continue
        parent = nodes[n.parent]
        if n.kind == "SAT_BARE":
            nth = 0
        else:
            nth = children.get(n.parent, 0) + 1
            children[n.parent] = nth
        n.number = parent.number + " " + str(nth)
        n.path = parent.path + ("." if n.spelling else "") + n.text
    return nodes


def by_number(nodes):
    return {n.number: n for n in nodes.values()}


def by_path(nodes):
    """Path text -> node, INCLUDING the bare spelling of a call shape.

    `satellite.console.input` is not a node -- `1 5 2` is `input()` -- but the
    trie walks the bare spelling to the `()` child (words_walk.hpp), so a help
    query written the way a person types it has to land there too. The first
    row to claim a bare spelling keeps it, which is the same first-wins the
    walk has because position 0 is the bare shape.
    """
    out = {}
    for n in sorted(nodes.values(), key=lambda n: n.order):
        out.setdefault(n.path, n)
        if n.arguments:
            out.setdefault(n.path[:n.path.find("(")].rstrip("."), n)
    return out


if __name__ == "__main__":
    nodes = read()
    print("%d nodes" % len(nodes))
    for n in sorted(nodes.values(), key=lambda n: n.order)[:8]:
        print("  %-28s %-12s %s" % (n.path, n.number, n.ident))
