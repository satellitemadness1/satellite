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
| [M2.md](M2.md) | 2026-08-28 | The namespace trie, the path interner, and `satl --words`. |
| [M3.md](M3.md) | 2026-08-29 | The lexer. Committed a day late, with M4. |
| [M4.md](M4.md) | 2026-08-30 | The arena AST and the parser. `satl --unparse` round-trips. |
| [M4.5.md](M4.5.md) | 2026-08-30 | The `.satc` cache. |
| [M5.md](M5.md) | 2026-08-30 | The error reporter — codes, carets, notes, did-you-mean. |
| [M9.md](M9.md) | **not in this tree** | A review of `prototype/M9`, which is a draft. |
| [M10.md](M10.md) | **not in this tree** | A review of `prototype/M10`, likewise. |

**The last two rows are the reason this table has a column for it.** Both files
opened `**Landed 2026-08-29**` flat until 2026-08-30, which every other note here
means as *landed in `src/`* — and PLAN §8 lists both as milestones that have not
started. They review `prototype/`, which LAYOUT.md calls **"drafts, and never
sources"** with receipts. **Writing the commit down is what found it**: a
milestone with no commit is either unlanded or mis-labelled, and there is no
third answer.

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

**A tag would need no follow-up and is deliberately not the answer.** `git tag
M5` records the same fact where nobody reading the tree can see it, and this
directory exists precisely so that the account of a milestone is in the
repository rather than in its metadata. The hash is here for the same reason
every measurement in this tree sits beside the decision it justifies.
