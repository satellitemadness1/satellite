#!/usr/bin/env python3
"""What `satellite.help()` prints: the topics, one per line, indented one tab,
with a blank line between them.

A topic is a top-level word that is BUILT -- something under it has a handler,
or the parser and resolver recognise it and the language is written in it.  The
rest of words.def's top-level nodes are numbered and unreached, and help does
not name them: advertising a path nothing implements is the drift this whole
milestone exists to end."""
import json, glob, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))

# The front-end words: recognised by the parser or the resolver and never
# dispatched, so no handler row will ever say they are built.  This set has to
# be written down rather than inferred, or it drifts the first time a word moves.
FRONT_END = {"1 1", "1 2", "1 3", "1 13", "1 15"}

# satellite.returns `1 21` is deliberately NOT here.  It is a front-end word and
# it parses, but it is optional, no file in the tree uses it, and the author's
# instruction is that it is not how a capsule is written.  It stays reachable at
# satellite.help(satellite.returns) and is not offered as a topic.

def load(name):
    out = {}
    for path in sorted(glob.glob(os.path.join(HERE, name))):
        out.update(json.load(open(path)))
    return out

def render():
    nodes = [l.rstrip("\n").split("\t") for l in open(os.path.join(HERE, "nodes.tsv")) if l.strip()]
    entries = load("entries_*.json")

    built_under = set()
    for mark, path, num, ms, a, r, anyx in nodes:
        if mark in ("H", "A"):
            built_under.add(" ".join(num.split()[:2]))

    lines = []
    for mark, path, num, ms, a, r, anyx in nodes:
        parts = num.split()
        if len(parts) != 2:                      # top-level only: `1 N`
            continue
        if num not in built_under and num not in FRONT_END:
            continue
        prose = entries.get(num, {}).get("prose", "")
        first = prose.split("\n\n")[0].replace("\n", " ").strip()
        first = first.split(". ")[0].rstrip(".") + "."
        lines.append((path, first))

    width = max(len(p) for p, _ in lines)
    out = []
    out.append("satellite -- the topics that are built. Ask about any of them:")
    out.append("")
    out.append("    satellite.help(satellite.console)")
    out.append("")
    for path, first in lines:
        out.append("\t%-*s   %s" % (width, path, first))
        out.append("")
    out.append("    %d topics." % len(lines))
    return "\n".join(out)

if __name__ == "__main__":
    print(render())
