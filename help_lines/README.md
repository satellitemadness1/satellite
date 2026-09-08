# `help_lines/` — where `HELP.md` and `help.def` come from

**Both artifacts are generated and this is their source.** Edit the JSON, run
`gen.py`, and the document and the table the interpreter compiles in are
rewritten together; editing either directly loses the change the next time
anything is regenerated.

    nodes.tsv                 the measured node table -- mark, path, number,
                              milestone, arity, receiver binding
    entries_*.json            the written entries, keyed by word number
    words_def.py              reads src/satellite_words/words.def for the
                              identifiers help.def has to name nodes by
    defgen.py                 entries -> src/satellite_help/help.def
    gen.py                    nodes.tsv + entries -> HELP.md, and help.def
    topics.py                 runs `satellite.help()` for HELP.md's first block
    verify.py                 runs every worked line against ./satl

## Two artifacts out of one run, which is the point

`HELP.md` is what a person reads in the repository; `src/satellite_help/help.def`
is what `satellite.help` prints inside a program. **Generating them separately
would let the document and the language disagree about the language**, which is
the drift DESIGN §4.6 exists to remove — so `gen.py` writes both, and stops if
any node is missing an entry.

`help.def` is **not rewritten when nothing changed**. It is a prerequisite of
every object in the build and `topics.py` refuses to ask a `satl` older than it,
so a generator that stamped a new mtime on identical bytes would rebuild the
whole tree and then declare the binary stale anyway.

## Why the table is measured rather than typed

`nodes.tsv` is the output of a throwaway that installs every module's handlers
and walks the trie from node 1 to `kNodeCount`, asking
`eval::Handlers::table().find(id)` and `Assigners::table().find(id)` for each. So
the `H` mark, the arity and the receiver binding are what the dispatch tables
actually hold on the day it ran, not what a document claims.

Regenerate it by compiling an enumerator against `src/` — everything but
`programs/main.cpp`, `programs/cpu_level.cpp`, `programs/window_handover.cpp` and
`satellite_prompt/`, which carry a `main()` or a dependency on the window — and
linking it against the tree's own object files. **MILESTONES/M18.md** records the
last regeneration: 109 `H` rows against 155 dots, the three new ones being help's.

**This is not the same list as what help NAMES.** M18 built `satellite.help`, and
it names what is **built** — a handler row, an assigner row, a front-end word, or
anything with one of those underneath it, which is 144 of the 264 against the 109
marked here. `src/satellite_help/built.hpp` is the predicate.

## Why the first block of HELP.md is run rather than written

`topics.py` **used to reconstruct** the listing from `nodes.tsv` plus a
hand-written front-end set — a second implementation of M18's whole subject, free
to disagree with the first. It did, on the day help was built: the real walk
names 14 topics and the reconstruction named 13, because help now names itself.
It runs the three-line program instead, and refuses if `./satl` is older than
`help.def`. The front-end set it held now lives in `words.def`'s fourth list as
data, which is where PLAN M18 says it has to be.

## Why the examples are run

`verify.py` pulls each `example` out, wraps it into a whole program, and executes
it with `SATL_NO_WINDOW=1` and a closed stdin. An entry may say
`"expect": "refuses"` where the point of the path is that it refuses, `"skip"`
where the line is not runnable on its own, and `"stdin"` where it reads input.

**It caught five false statements while these were being written**, which is the
argument for keeping it rather than the argument for having had it:
`console.typed()` answers the line or nothing rather than a bool; `floor`,
`ceil`, `round` and `truncate` are number methods and refuse a float; `power`
answers a float; a quoted word inside `list.remove` is read as an option name
rather than as a value; and `satellite.returns` is optional and used by no file
in the tree. Every one of those was written down confidently and was wrong.

**LAYOUT.md now has a row for `HELP.md` and for `src/satellite_help/`.** This
directory is the generator rather than part of the interpreter and still has
none; whether it earns one is the author's call.
