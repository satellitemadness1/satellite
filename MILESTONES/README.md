# `MILESTONES/` — what each milestone did, and the commit that did it

One file per milestone that has been built, written when it landed. **These are
reviews and not plans**: [PLAN.md](../PLAN.md) §8 says what a milestone *will*
be, and a file here says what it turned out to be — what was decided on the way,
what building it found, what it left open, and which mutations prove its test.

This file is an **index and holds no fact of its own**, the same shape as
[README.md](../README.md) over the documents and the `Makefile` over
`make_support/`. Every hash below lives in the note it points at.

| | landed | |
| --- | --- | --- |
| [M1.md](M1.md) | 2026-08-26 | `satl` exists and says how to use it. |
| [M1.5.md](M1.5.md) | 2026-08-27 | The window — `satl-term`, and `satl` handing itself over. |
| [M2.md](M2.md) | 2026-08-28 | The namespace trie, the path interner, and `satl --words`. |
| [M3.md](M3.md) | 2026-08-29 | The lexer. Committed a day late, with M4. |
| [M4.md](M4.md) | 2026-08-30 | The arena AST and the parser. `satl --unparse` round-trips. |
| [M4.5.md](M4.5.md) | 2026-08-30 | The `.satc` cache. |
| [M5.md](M5.md) | 2026-08-30 | The error reporter — codes, carets, notes, did-you-mean. |
| [M6.md](M6.md) | 2026-08-30 | The machine limits — `satellite_config.ini`, the pool, the watchdog, the three fact readers. |
| [M7.md](M7.md) | 2026-08-31 | Resolve — names to frame slots, the paths to their numbers, and `satl --resolve`. |
| [M8.md](M8.md) | 2026-08-31 | `satellite.variable.number` — the port, the explicit sign, and `satl --number`. |
| [M8.5.md](M8.5.md) | 2026-09-01 | The walkers keep their own stacks — four cycles, seven walkers, no depth. |
| [M9.md](M9.md) | 2026-09-01 | The value model and closure compilation — and an evaluator with no depth in it. |
| [M10.md](M10.md) | 2026-09-02 | The console with its own printer thread, `satellite.main`, and the first program that runs. |
| [M11.md](M11.md) | 2026-09-03 | Scalars and control flow — the string's sixteen methods, the module constants, and the Ctrl-C that stops a loop. |
| [M12.md](M12.md) | 2026-09-03 | The variant, and what "nothing" is — a state every type has, askable at last, and every refusal now names its method. |
| [M13.md](M13.md) | 2026-09-04 | The clock and the dice — twelve tier rows, `now` and `sleep`, the sixth value arm, and the whole-call road the compiler turned out not to have. |
| [M14.md](M14.md) | 2026-09-04 | The console's other half — the reader thread on its two-descriptor park, `input` in three shapes, `typed()`, the live facts, and the queue's escapes. The namespace is finished. |
| [M15.md](M15.md) | 2026-09-04 | The float, the rounding rule — half away from zero, delegated and ratified — and the retune: assignment to a dial is a write the next division observes. |
| [M16.md](M16.md) | 2026-09-06 | The containers and the search power — `list`, `map`, their thirty-four methods, and the empty list `satellite.main`'s parameter needed. |
| [M17.md](M17.md) | 2026-09-06 | Hello world — DESIGN §3 runs, held to the file by a test; the include's bare shape parses at last, and a spaceship's name stops being a variable. |
| [M22.md](M22.md) | 2026-09-07 | **Out of order, and §0 says why.** The prompt — `satl --repl` reads a line, a typed line becomes a program, and `satl-term`'s window stops closing when what exited was the prompt. |
| [M18.md](M18.md) | 2026-09-08 | `satellite.help` — the trie answering for itself: the `built()` predicate over four kinds, the language's first unevaluated argument, and 264 entries generated into the binary from the file that writes the document. |
| [M19.md](M19.md) | 2026-09-08 | Persistence — files and directories, and the fold that had been guessing. |
| [M19.5.md](M19.5.md) | 2026-09-08 / 09 | `satellite.variable.binary` and `satellite.variable.hex` — the width that makes it a type, and the four verbs. **Landed in two halves and the note carries both hashes.** |
| [M19.6.md](M19.6.md) | 2026-09-09 | The `.satc` written after resolve, and the option token — `0#down`, and a cache that stops deciding what it already decided. |
| [M20.md](M20.md) | 2026-09-11 | The machine's facts, in the language — `satellite.system`, the arguments object and its ten selectors, the live codes a program can read at last, and a watchdog floor a running program can move. |
| [M21.md](M21.md) | 2026-09-11 | A piece of QUAD, running — `Sky::decay` and `Rack::draw` written by hand, 40 activations of 40 against the double's 6, the seeded tier that made a replayable fractional draw sayable, and the float's first method. |
| [M23.md](M23.md) | 2026-09-12 | Threads — the language's first deferred call, three verbs, a `satellite.library` two walks share, and DESIGN §7.1's 1585-wrong-out-of-1600 run backwards at 1600 of 1600. **The first milestone to add no numbered path and invalidate no `.satc`.** |

**M22 sits between M17 and M18 in this table and that is the point of the date
column.** It was built on 2026-09-07, before M18 through M21, at the author's
direction — M18's `satellite.help` is meant to answer about a program's
variables *after it has run*, which needs somewhere to type the question, and
the prompt was M22. A store built at M18 would have had its only reader here,
which is the no-consumer shape PLAN M2 forbids. **PLAN §8's build order is
unchanged**; M22.md §0 is the record that it was departed from, and M18's own
entry already allowed for one of the two moving.

***AND IT HAPPENED AGAIN AT M19.5, IN THE SAME DIRECTION, THREE WEEKS LATER.***
That row read **"Half of it — `satellite.variable.hex` is the half still owed"**
from 2026-09-08 until 2026-09-09, and the second half had landed in `cf04994`
the morning after. M19.5.md's own opening said so in its first line — *"Landed
in two halves: binary 2026-09-08, hex 2026-09-09"* — and the row above it did
not. **Found by a reader asking which milestone was next**, which is the one
question this table exists to answer and the one it was answering wrongly.

**THE M16 PARAGRAPH BELOW DIAGNOSED THIS EXACTLY AND THE FIX DID NOT HOLD.** Its
rule is *write the row in the commit that lands the work*, and M19.5's second
half landed in a commit that touched `words.def`, three sources and a test, and
not this file. The convention section below has a mechanism for a note that is
owed something — the word `PENDING`, and a grep that finds it — and **an index
row has no equivalent**, because a row is never incomplete, only wrong. That is
the shape of both failures: an index holds no fact of its own and therefore has
nothing to check it.

***The M16 row said "not in this tree — a review of `prototype/M10`" until
2026-09-06, and it was stale for the length of a milestone.*** The file it
points at was replaced that morning by the review of the real M16 —
`1fb1df7`, the containers — and its own first paragraph says so. **The row was
not**, so this table went on telling a reader that the tree's most recent
milestone was a draft review, while the note beside it said the opposite.
**That is this file's own rule failing in the one direction it was not written
to catch**: the rule below is *a milestone with no commit is unlanded or
mis-labelled, a milestone with no note is invisible*, and this was a milestone
with both whose INDEX ROW had not moved. An index holds no fact of its own and
therefore has nothing to check it — so what catches it is the same discipline
that catches everything else here, which is writing the row in the commit that
lands the work. *(Found at M17, on the way to adding the row below it.)*

**The numbers in this table are PLAN §8's, and §8 renumbered on 2026-08-30.**
M1 through M5 did not move, because a number written into the history is a fact
rather than a label. The last two rows did: they were `M9.md` and `M10.md` —
**and both names have now been taken back by real milestones**, M9 on 2026-09-01
and M10 on 2026-09-02, which is the renumbering finishing the job of reusing an
old name for a new thing. That is safe here and would not be in `words.def`: a note is looked up by
what it says, not by its position, which is the same distinction `errors.def`
draws between a code and a word number. **`M11.md` went round the same loop
twice**: renamed from the old `M9.md` at the renumber to hold the review of
`prototype/M9`, and taken back by the real M11 on 2026-09-03 — its §9 keeps
what the review concluded, and the full text is in its history. The remaining
review row is `prototype/`'s, over the draft in `prototype/M10`, **whose
directory name is frozen** — a draft is dated by construction. `M1.5.md` was `M11.A`.
PLAN §8's opening carries the whole old-to-new table and it is the key to every
document written before that day.

**The review rows are the reason this table has a column for it.** Both files
opened `**Landed 2026-08-29**` flat until 2026-08-30, which every other note here
means as *landed in `src/`* — and PLAN §8 lists both as milestones that have not
started. They review `prototype/`, which LAYOUT.md calls **"drafts, and never
sources"** with receipts. **Writing the commit down is what found it**: a
milestone with no commit is either unlanded or mis-labelled, and there is no
third answer.

**And M1.5 is the same finding upside down, which the rule as written does not
catch.** It had no note here, no date in this table, and every clause of its
done-when true since 2026-08-27 — it was sitting twenty-third in §8's build order
being counted as work still to do. **A milestone with no commit is mis-labelled;
a milestone with no note is invisible**, and looking for the first is what
eventually found the second. The fix is the same one line in the same place: this
table now has a row for every milestone that has landed, and the date column is
what makes a missing one visible.

## The convention

A landed milestone's note opens with

    **Landed <date>** (`<hash>`).

and the hash is the commit that landed **the work**, not the one that added the
note — M1 landed on 2026-08-26 and its review was written three days later.
Where one commit landed two milestones, both notes name it and say so.

**A note cannot carry the hash of the commit that contains it**, which is not a
flaw to design around but a fact to know before it is discovered. Two shapes
work and only one is honest: writing the hash in a **follow-up** commit, or
leaving it out. Leaving it out is what happened to M3, M4, M4.5 and M5, and it
was invisible until somebody asked which commit landed which — so the follow-up
is the shape, and it is one line in one file.

**M15.md opened with the same `PENDING` and for the same reason** — it is the
tenth note written under this convention, and `grep PENDING MILESTONES/M*.md`
answered it and nothing else until its follow-up landed the same sitting —
which is the one hash this file cannot name, being IN it: the rule at the top
of this section, demonstrated by the file that states it. *(M11.md's said the same
until its follow-up landed on 2026-09-03 in `e435c96`; M12.md's on 2026-09-04
in `89f081a`; M13.md's and M14.md's the same day in `db510bf` — one commit
naming two notes, M3-and-M4's precedent one layer up; M10.md's on 2026-09-03 in
`636ab00`; M9.md's on 2026-09-01 in `16aa46a`; M8.md's and M7.md's on
2026-08-31 in `1f70002` and `3dca061` — one line in one file each.)*

**M6.md opened `**Landed 2026-08-30** (`PENDING`)` and that word was deliberate.**
It is the first note written knowing this rule, so rather than omitting the hash
— which is what made the other four invisible — it named the hole. `grep PENDING
MILESTONES/M*.md` lists every note still owed its follow-up, which "leaving it
out" could never do — **over the notes and not over this directory**, because
this file has to spell the word in order to describe it and would otherwise be a
permanent false positive in its own check. **That grep answers nothing today**,
which is the shape it is meant to have: at most one note owed a follow-up at
any time, and none once the sitting ends. M6's follow-up landed on 2026-08-31,
M7's and M8's the same day, M9's on 2026-09-01, M10's and M11's on 2026-09-03,
and M12's through M15's on 2026-09-04, so the convention has now been used end
to end **ten times** — a note that named its own hole, and a commit that
filled it.

*(**M17.md briefly carried the word twice**, on the afternoon it was written:
once here in the opening, and once in its §5 for a `make startup` that had not
been run yet because the tree was being edited in parallel. The measurement was
taken the same sitting, so §5 no longer says it and this grep means one thing
again. Recorded because it is the first time the check could have answered two
questions at once, and the fix was to take the measurement rather than to
qualify the word.)*

**A tag would need no follow-up and is deliberately not the answer.** `git tag
M5` records the same fact where nobody reading the tree can see it, and this
directory exists precisely so that the account of a milestone is in the
repository rather than in its metadata. The hash is here for the same reason
every measurement in this tree sits beside the decision it justifies.
