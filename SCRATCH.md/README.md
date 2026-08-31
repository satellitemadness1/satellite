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
| [WORD_SURFACE.md](WORD_SURFACE.md) | every `satellite.*` path that exists in the first satellite or is promised by this one, and which of five sources each was found in | **CONDITION MET 2026-08-28 — deletable now.** `words.def` is written and was checked against it by walking all 95 paths; 85 resolve and the other ten are each correctly absent, which the file's own header lists one by one |
| [PORTING.md](PORTING.md) | what `satellite_number` and `satellite_string` actually are in the first satellite, and what has to be decided before copying them | the port lands and PLAN §6 records what changed |
| [M6_STATE.md](M6_STATE.md) | where M6 got to when the session that built it was stopped: what is done and green, the three things that are not, and every file it added or changed | **CONDITION MET 2026-08-31 — deletable now.** All three are done and each is recorded where it belongs: the hash is in MILESTONES/M6.md's first line, the startup table is in `make_support/040-sources.mk`, and the installed binary has answered `--limits` and been killed by its own watchdog. What doing them found is M6.md §9 |
| [FIRST_NOTE.md](FIRST_NOTE.md) | the ledger for converting `plans/madness/first_note.txt` into the permanent documents, with the original preserved verbatim | done — kept only until someone has read it once |
| [WORD_NUMBERS_ORIGINAL.md](WORD_NUMBERS_ORIGINAL.md) | **the account of the numbering** — the author's 36 hand-written lines preserved verbatim, what each became, the two `?`s resolved, and where the numbering stands in code. Handed over 2026-08-28; §1 is theirs and uneditable, the rest is mine | the history in it stops being useful — **not** when the rewrite is accepted, which already happened |

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
