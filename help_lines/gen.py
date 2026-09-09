#!/usr/bin/env python3
"""Assemble HELP.md from the measured node table plus the written entries, and
write src/satellite_help/help.def from the same entries in the same run.

The node table is produced by the throwaway enumerator, so the mark, the number
and the milestone are measured; only the prose is typed.

TWO ARTIFACTS OUT OF ONE RUN, WHICH IS THE POINT AND NOT A CONVENIENCE. HELP.md
is what a person reads in the repository and help.def is what `satellite.help`
prints inside a program; generating them separately would let the document and
the language disagree about the language, which is the drift DESIGN §4.6 exists
to remove."""
import os, sys, json

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)   # so `import topics` works from any directory
ROOT = os.path.dirname(HERE)
TSV  = os.path.join(HERE, "nodes.tsv")

def group_of(path):
    if path == "satellite":
        return "satellite"
    rest = path[len("satellite."):]
    out = []
    for c in rest:
        if c in ".(":
            break
        out.append(c)
    return "".join(out)

def load_nodes():
    rows = []
    with open(TSV) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            mark, path, num, ms, arity, recv, any_ = line.split("\t")
            rows.append(dict(mark=mark, path=path, num=num, ms=ms,
                             arity=int(arity), recv=int(recv), any=int(any_)))
    return rows

def load_entries():
    entries = {}
    for name in sorted(os.listdir(HERE)):
        if not name.startswith("entries_") or not name.endswith(".json"):
            continue
        with open(os.path.join(HERE, name)) as f:
            entries.update(json.load(f))
    return entries

HEADER = """# The help lines

**One entry for every node `words.def` numbers**, in trie order, which is the
order help walks them and the order the rows sit in the file.

Each entry is three things:

- the **mark, number and path**, which are measured rather than typed;
- the **`>` line**, which is what you type to bring the entry up. Consecutive
  entries share one, because asking about a path brings up that node **and the
  words underneath it together** — `satellite.help(satellite.main)` answers for
  `1 3` and `1 3 0` at once;
- **what it is**, in a few sentences, and then **a worked line**.

**Every worked line in this file has been run.** A script pulls each one out,
wraps it in a program, and executes it against the interpreter in this tree;
143 of them run and pass — **including help's own three**, which were marked
unrunnable until M18 built the thing they demonstrate. That is not a formality — writing these caught **five
statements that were plainly stated and plainly wrong**:

- `console.typed()` answers the line itself, or nothing, and not a yes-or-no.
- `floor`, `ceil`, `round` and `truncate` are number methods and refuse a float.
- `power` answers a float, so `digits` refuses its result.
- a quoted word inside `list.remove` is read as an option name, not a value.
- `satellite.returns` is optional rather than required, and no file in the tree
  uses it.

The mark comes from walking the trie with every module's handlers installed.
`H` means a handler row exists and a call to it runs today, which is **130 of
the 269**. A dot means nothing is behind it yet, which is the other **139**.

**A dot is not the same as undocumented, and that is the trap in this list.**
The front-end words — `include`, `capsule`, `main`, `return`, `statement`
and every type name — are all dotted, because the parser and the
resolver recognise them and they are never dispatched. They are the words the
language is written in. Hello world uses seven paths and exactly one of them,
`satellite.console.display`, is a handler row.

**Which is why a dot is not what help goes by.** M18 built `satellite.help`, and
what it names is what is **built** — a handler row, an assigner row, a front-end
word, or anything with one of those underneath it. That is **168 of the 269**,
against the 130 marked `H` here. `src/satellite_help/built.hpp` is the predicate
and DESIGN §4.6 carries the argument.

**Where a path belongs to a milestone nobody has started, the entry says which
milestone and shows no example.** Writing a worked line for
`satellite.network.https` would be inventing the language, which is the thing
help exists to stop.

The ten aliases carry no entry of their own. Each shares a node with a path
already listed, and help answers for the node.

---
"""

def main():
    nodes = load_nodes()
    entries = load_entries()

    # THE PARSE IS CHECKED AGAINST THE MEASUREMENT BEFORE EITHER IS USED.
    # words_def.py reads the registry to get each node's IDENTIFIER, which
    # help.def needs and nodes.tsv does not carry; nodes.tsv was produced by a
    # binary that linked the real trie. They agree on all 264 paths and numbers
    # or this stops -- a generator that quietly emitted rows against a parse it
    # had got wrong would put the error in a file nobody reads by hand.
    import words_def
    import defgen
    parsed = words_def.read()
    mine = {n.number: n.path for n in parsed.values()}
    theirs = {n["num"]: n["path"] for n in nodes}
    if mine != theirs:
        wrong = sorted(set(mine.items()) ^ set(theirs.items()))
        sys.exit("words.def parse disagrees with nodes.tsv: %r" % wrong[:4])
    missing = [n["num"] for n in nodes if n["num"] not in entries]
    if missing:
        sys.exit("no entry for %d nodes, first %r -- help.def must be complete"
                 % (len(missing), missing[:4]))
    written = defgen.write(parsed, entries, words_def.by_path(parsed))
    sys.stderr.write("help.def %d rows\n" % written)

    out = [HEADER]

    # THE TOPIC LISTING IS ASKED OF THE INTERPRETER AND NOT REBUILT HERE.  It
    # used to be assembled from nodes.tsv plus a hand-written front-end set --
    # a second implementation of M18's whole subject, free to disagree with the
    # first, and it did on the day help was built: the real walk names 14 topics
    # and the reconstruction named 13, because help now names itself.  topics.py
    # runs `satellite.help()` and this copies the answer in.
    import topics
    out.append("\n## What `satellite.help()` prints\n")
    out.append("**Taken from the interpreter in this tree, not reconstructed.**")
    out.append("`topics.py` runs the three-line program and copies the answer in,")
    out.append("so this block cannot say something the language does not.  The")
    out.append("topics are the built children of `satellite`, one to a line, each")
    out.append("indented one tab and separated by a blank line.\n")
    out.append("```")
    out.append(topics.render())
    out.append("```\n")
    out.append("---")
    last = None
    written = 0
    for n in nodes:
        g = group_of(n["path"])
        if g != last:
            out.append("\n## %s\n" % g)
            last = g
        ms = "  _%s_" % n["ms"] if n["ms"] else ""
        out.append("%s  `%s`  `%s`%s" % (n["mark"], n["num"], n["path"], ms))
        e = entries.get(n["num"])
        if e:
            written += 1
            out.append("> %s" % e["query"])
            out.append("")
            out.append(e["prose"].strip("\n"))
            if e.get("example"):
                out.append("")
                for line in e["example"].strip("\n").split("\n"):
                    out.append("    " + line if line.strip() else "")
        else:
            out.append(">")
        out.append("")
    text = "\n".join(out) + "\n"
    with open(os.path.join(ROOT, "HELP.md"), "w") as f:
        f.write(text)
    sys.stderr.write("nodes %d, entries written %d, still blank %d\n"
                     % (len(nodes), written, len(nodes) - written))

if __name__ == "__main__":
    main()
