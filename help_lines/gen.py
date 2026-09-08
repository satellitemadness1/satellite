#!/usr/bin/env python3
"""Assemble SCRATCH.md/HELP_LINES.md from the measured node table plus the
written entries.  The node table is produced by the throwaway enumerator, so
the mark, the number and the milestone are measured; only the prose is typed."""
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
140 of them run and pass. That is not a formality — writing these caught **five
statements that were plainly stated and plainly wrong**:

- `console.typed()` answers the line itself, or nothing, and not a yes-or-no.
- `floor`, `ceil`, `round` and `truncate` are number methods and refuse a float.
- `power` answers a float, so `digits` refuses its result.
- a quoted word inside `list.remove` is read as an option name, not a value.
- `satellite.returns` is optional rather than required, and no file in the tree
  uses it.

The mark comes from walking the trie with every module's handlers installed.
`H` means a handler row exists and a call to it runs today, which is **106 of
the 264**. A dot means nothing is behind it yet, which is the other **158**.

**A dot is not the same as undocumented, and that is the trap in this list.**
The front-end words — `include`, `capsule`, `main`, `return`, `statement`
and every type name — are all dotted, because the parser and the
resolver recognise them and they are never dispatched. They are the words the
language is written in. Hello world uses seven paths and exactly one of them,
`satellite.console.display`, is a handler row.

**Where a path belongs to a milestone nobody has started, the entry says which
milestone and shows no example.** Writing a worked line for
`satellite.network.https` would be inventing the language, which is the thing
help exists to stop.

The nine aliases carry no entry of their own. Each shares a node with a path
already listed, and help answers for the node.

---
"""

def main():
    nodes = load_nodes()
    entries = load_entries()
    out = [HEADER]

    # THE TOPIC LISTING IS PART OF THIS DOCUMENT AND NOT A SECOND FILE.  It is
    # what `satellite.help()` prints, it is generated from the same entries the
    # rest of the file holds, and a second file would be free to disagree with
    # them -- which is the drift M18 exists to end, reproduced in the document
    # that describes it.
    import topics
    out.append("\n## What `satellite.help()` prints\n")
    out.append("The topics that are built, one to a line, each indented one tab")
    out.append("and separated by a blank line.  Generated from the entries below,")
    out.append("so it cannot name a topic this file does not describe.\n")
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
