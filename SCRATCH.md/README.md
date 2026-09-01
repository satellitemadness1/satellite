# `SCRATCH.md/` — the things that do not last forever

A folder, not a file, and the `.md` in its name is deliberate: it sorts next to the
four permanent documents it is the opposite of.

**Everything in here is temporary and is meant to be deleted.** Nothing in this
folder decides anything. If a fact in here matters after this week, it is in the
wrong file — move it to whichever of the four owns it:

| | holds |
| --- | --- |
| [DESIGN.md](../DESIGN.md) | what the language **is** — permanent |
| [PLAN.md](../PLAN.md) | how it gets **built** — permanent |
| [LAYOUT.md](../LAYOUT.md) | every file in the tree and what it is for |
| [WORD_NUMBERS.md](../WORD_NUMBERS.md) | **the numbering** — the authority over every number in the language |

The test for whether something belongs here rather than there: *does it stop being
true when the work it describes is finished?* A session's open questions stop being
true. A measured startup floor does not.

## What is in here now

| file | what it is | delete when |
| --- | --- | --- |
| [SESSION.md](SESSION.md) | where the work stands, what is decided, what is in flight, and what only the user can answer | its "not yet in a permanent document" and "questions" lists are both empty |
| [MILESTONE.md](MILESTONE.md) | every piece of specified work that belongs to **no milestone** in PLAN §8 — **121 of the 222** numbered paths among them *(the "50 of the 215" this row used to say was two audits and one renumbering out of date)* | every row has a milestone in PLAN §8, moved there rather than copied |
| [NO_LIMITS.md](NO_LIMITS.md) | **The author's rule that the language has no limits, and the four walkers that broke it.** §5 IS BUILT — M8.5, 2026-09-01 — so what is left is §8's four questions. What the 8 MiB, the 20,000 and the 2,000 each actually are — only one is a limit and it was added at M7 — every recursive walker in the tree, the measured depth at which each command dies, the plan file by file, and what changes in DESIGN §7.5, PLAN §2.5 and M9 | **§8 is empty.** The walkers keep their own stacks, nothing segfaults at any depth, and the permanent documents carry it — all true since M8.5; the four open questions are not |
| [PORTING.md](PORTING.md) | what `satellite_number` and `satellite_string` actually are in the first satellite, and what has to be decided before copying them. **Half spent**: §4 and §5 landed at M6, and the number half landed at M8 on 2026-08-31 with all four of its §6 questions answered in PLAN §6.1 — three of them differently from what §6 predicted, which is why those rows are struck through rather than edited | **§3 is all that is left and it is M9's.** Delete when `satellite_string`'s live codes 95–100 decode to real values instead of `<threads>`, and PLAN §6 records what changed |
| [FIRST_NOTE.md](FIRST_NOTE.md) | the ledger for converting `plans/madness/first_note.txt` into the permanent documents, with the original preserved verbatim | done — kept only until someone has read it once |
| [WORD_NUMBERS_ORIGINAL.md](WORD_NUMBERS_ORIGINAL.md) | **the account of the numbering** — the author's 36 hand-written lines preserved verbatim, what each became, the two `?`s resolved, and where the numbering stands in code. Handed over 2026-08-28; §1 is theirs and uneditable, the rest is mine | the history in it stops being useful — **not** when the rewrite is accepted, which already happened |

**Three rows were deleted on 2026-08-31 and the files with them**, which is the
only thing this folder can do that proves it is working. `WORD_SURFACE.md` and
`M6_STATE.md` had both said **CONDITION MET — deletable now** in this table since
the day their conditions were met, and `M7_START.md`'s condition was *"M7 lands
and `MILESTONES/M7.md` carries whatever of it turned out to be true"* — which it
does: the draft done-when it offered is in PLAN §8 with two of its clauses
corrected, the reading of `prototype/M6/` is M7.md §3.6 and §4.3, the stale
comment it found in `words_runtime.hpp` is fixed, and the two recursion bounds it
separated are separated in DESIGN §7.5 itself.

**A row that says "deletable now" and stays is this folder failing at its one
job.** The condition is the whole mechanism; leaving the file behind after it is
met turns a temporary record into a permanent one that nothing maintains, which
is what §"Why this folder exists" says the cost of is a decision made twice.

## Why this folder exists

Because a conversation is not a record. Work that lives only in a transcript is
lost the moment the transcript is cleared, and the cost is not the typing — it is
that a decision gets made twice, differently, and nobody notices which one the code
followed. This folder is where work waits between being done and being written
down properly.

It is the same argument the rest of the tree already makes: the `Makefile` is an
index over `make_support/`, `install.sh` is an index over `install_support/`, and
`README.md` holds no fact of its own. One place per fact, and a name that says which
place it is.
