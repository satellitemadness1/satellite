# `help_lines/` — where `HELP.md` comes from

**The markdown is generated and this is its source.** Edit the JSON, run
`gen.py`, and the document is rewritten; editing the document directly loses the
change the next time anything is regenerated.

    nodes.tsv                 the measured node table -- mark, path, number,
                              milestone, arity, receiver binding
    entries_*.json            the written entries, keyed by word number
    gen.py                    nodes.tsv + entries -> HELP.md
    verify.py                 runs every worked line against ./satl

## Why the table is measured rather than typed

`nodes.tsv` is the output of a throwaway that installs every module's handlers
and walks the trie from node 1 to `kNodeCount`, asking
`eval::Handlers::table().find(id)` for each. So the `H` mark, the arity and the
receiver binding are what the dispatch table actually holds on the day it ran,
not what a document claims. **This walk is also the shape M18 needs** — PLAN
§8's `built()` predicate is this plus the front-end word set, which does not
exist as data yet.

Regenerate it by rebuilding the enumerator against every source outside
`src/programs/` and `src/satellite_prompt/`, which are the two that carry a
`main()` and a dependency on it.

## Why the examples are run

`verify.py` pulls each `example` out, wraps it into a whole program, and
executes it with `SATL_NO_WINDOW=1` and a closed stdin. An entry may say
`"expect": "refuses"` where the point of the path is that it refuses, `"skip"`
where the line is not runnable on its own, and `"stdin"` where it reads input.

**It caught four false statements while these were being written**, which is the
argument for keeping it rather than the argument for having had it:
`console.typed()` answers the line or nothing rather than a bool; `floor`,
`ceil`, `round` and `truncate` are number methods and refuse a float; `power`
answers a float; and a quoted word inside `list.remove` is read as an option
name rather than as a value. Every one of those was written down confidently and
was wrong.

**LAYOUT.md does not have a row for this directory.** It is scaffolding for M18
rather than part of the interpreter, and whether it earns one is the author's
call.
