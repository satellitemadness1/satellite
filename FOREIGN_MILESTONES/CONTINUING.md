# Continuing this folder

**Read this before adding anything.** It is the conventions and the remaining
work, written so a session with none of the original conversation can carry on
without reinventing the rules or renumbering the milestones.

**Written 2026-09-12.** [README.md](README.md) is what the folder is for; this is
how to keep writing it.

---

## 1. The rules, in order of how much damage breaking them does

### 1.1 Three answers, and the third is real

Every foreign construct gets exactly one:

| answer | when | example |
|---|---|---|
| **satellite says it** | it exists today — give the line to type | `println!` → `satellite.console.display(x)` |
| **a milestone number** | it does not exist yet | `&&` → M28 |
| **never** | it will not exist, and say why | `mov eax, 1` → no inline assembly, none planned |

**"Never" is an answer, not a gap.** Saying outright that satellite has no inline
assembly respects a reader more than an invented number they would wait for.
Do not reach for a milestone to avoid saying no.

### 1.2 A milestone named in a message is a promise

> **A MILESTONE NAMED IN AN ERROR MESSAGE MUST BE DEFINED BEFORE THE MESSAGE
> SHIPS.** The reader is being told to wait. They are entitled to know what for.

This is the rule the whole folder exists to keep. `src/error_reporter/foreign.cpp`
names milestone numbers in text a user sees; every one must have a file in
[SATELLITE/](SATELLITE/). Check it mechanically before committing:

```sh
cd /home/madness/code/cxx/satellite
grep -oE '"M[0-9]+"' src/error_reporter/foreign.cpp | tr -d '"' | sort -u > /tmp/named
ls FOREIGN_MILESTONES/SATELLITE/ | grep -oE '^M[0-9]+' | sort -u > /tmp/have
comm -23 /tmp/named /tmp/have        # MUST be empty
```

The reverse direction (a file nothing names) is fine — M29 is one.

**The origin of this rule is a real failure.** S0721 said "a later milestone"
about M23, which had shipped three weeks earlier; the message was truthful about
the handler table and false about the language, and it sent its reader to the
milestone docs to look for finished work. See
[../ERROR_HANDLING.md](../ERROR_HANDLING.md) §1.

### 1.3 A language folder catalogues; SATELLITE/ is the work

`CXX23/M20.md` is a **position in a list of C++ features**. `SATELLITE/M28-...`
is a **promise satellite has made**. Only the second kind may appear in
`foreign.cpp`. Do not let the two numbering systems touch.

### 1.4 Draft standards do not get promises

`CXX26/` is written against a working paper. Nothing in it goes into
`foreign.cpp` until the paper is in a published standard — a promise against a
draft is one satellite cannot keep on its own schedule.

### 1.5 Write a page when the feature has a complication, not to fill the table

**This is the one that decides how much of the remaining work is worth doing.**

An index row that says `std::vector` → `satellite.container.list<T>` is already
the complete answer. A page restating it adds nothing and costs a reader a click.

The four CXX23 pages that exist earned it:

- [CXX23/M9.md](CXX23/M9.md) — pointers are not *missing*, they are *replaced*, and
  the page exists to say that a container does NOT follow the same rule
- [CXX23/M46.md](CXX23/M46.md) — writing it uncovered the threading gap
- [CXX23/M1.md](CXX23/M1.md) — `argc` is gone and the return value is not a status
- [CXX23/M3.md](CXX23/M3.md) — the format-string loss is real and worth stating

**So: page it when the honest answer needs more than one line, a reader would
otherwise carry a wrong assumption across, or writing it teaches you something
about satellite.** Otherwise leave the index row to do its job.

---

## 2. Numbers that are taken

**Satellite milestones, M27–M45** — all defined, all in [SATELLITE/](SATELLITE/)
except M41–M45 which live in [CXX26/](CXX26/) beside the features that asked for
them.

| | | | |
|---|---|---|---|
| M27 break/continue | M28 `&&` `\|\|` | M29 method chaining | M30 switch |
| M31 user generics | M32 capsule as value | M33 catching a refusal | M34 constexpr |
| M35 structured bindings | M36 ranges | M37 ternary | M38 operator overloading |
| M39 overloading/defaults | M40 locks and atomics | M41 reflection | M42 contracts |
| M43 senders/hazard | M44 `#embed` | M45 string encoding | |

**The next free satellite milestone is M46.**

**Error codes** are a different registry and live in
`src/error_reporter/errors.def`. `S0734` is *proposed* in ERROR_HANDLING.md §4.2
and **not minted**. `S15xx` is taken by the static analyser. Confirm against
`satl --errors` before writing any number — the first draft of §4.2 claimed
S0722 was free and it was not.

---

## 3. What is written and what is not

| folder | catalogued | pages written | state |
|---|---|---|---|
| [SATELLITE/](SATELLITE/) | — | 14 | **complete** for everything `foreign.cpp` names |
| [CXX23/](CXX23/) | 56 features | 4 | index complete, pages sparse **by rule 1.5** |
| [CXX26/](CXX26/) | 22 features | 5 | **complete** — every link resolves |
| [PYTHON/](PYTHON/) | 20 rows | 0 | index complete, no pages |
| [JAVA/](JAVA/) | 20 rows | 0 | index complete, no pages |
| [RUST/](RUST/) | 20 rows | 0 | index complete, no pages |
| [ASM/](ASM/) | — | 0 | README is the whole answer, deliberately |

**Every index carries the right answer in every row.** The folders with no pages
are not unfinished in the sense that matters — a reader gets the correct answer
from the table. Pages are the elaboration.

### The remaining work, in the order it is worth doing

1. **Older C++ standards folded into CXX23/** — the user's stated next step:
   support C++98/11/14/17 features in pieces, adapting each to satellite style
   as its milestone is built. The index rows exist; add `(C++11)`, `(C++17)`
   markers so a reader knows which standard introduced what they typed.
2. **PYTHON/, JAVA/, RUST/ pages** — the user's stated order after C++. Apply
   rule 1.5 hard: most rows do not need one. The ones that do are where a
   reader carries a wrong assumption across, and each index already flags its
   own: Python's `self` (row 16), Java's garbage collection vs refcounting
   (row 10), Rust's `Arc`/`Mutex` (row 18).
3. **More `foreign.cpp` rows** as the tables grow. 128 today.

---

## 4. Touching `foreign.cpp`

`src/error_reporter/foreign.{hpp,cpp}` is the code half. Two things will bite:

**ORDER MATTERS — longest and most distinctive first.** `::` and `->` are the
LAST two rows on purpose; they appear inside almost every C++ line, so a table
matching them early answers "every path is dotted" to every C++ question. This
was a real bug: `std::optional<int> x;` answered `.` until they were moved.

**THE GUARDS KEEP IT QUIET, AND THEY ARE THE WHOLE SAFETY ARGUMENT.** It runs on
every rendered diagnostic:

- a line containing `satellite.` is never matched — the author is already here
- a line starting `//` is never matched — a comment is not code
- `#` is a comment only with a space after it, so `#include` and `#[derive(...)]`
  still match (killing those was the guard's first bug)

**Verify after any change:** zero matches across the whole of `example/`, and
against a real program if one is to hand.

```sh
for f in example/*.satl; do ./satl --check "$f" 2>&1 | grep -c 'that looks like'; done
```

Anything non-zero is a false positive and the row that caused it is too generic.

---

## 5. Working alongside another session

The tree builds in-tree, so two `make`s corrupt each other's objects.

```sh
mkdir /home/madness/code/cxx/satellite/.make.lock   # fails if held
# ... build ...
rmdir /home/madness/code/cxx/satellite/.make.lock
```

**The lock covers EDITING tracked source, not just running make** — an in-tree
build reads sources the whole way through. Both sessions learned this the
expensive way on 2026-09-12: each edited the tree under the other's running
build, and the result was a test run that proved nothing and was thrown away.
Write to a scratchpad and apply once the lock is free.

**`clang++ -fsyntax-only -Isrc <file>` is safe under the lock.** It writes no
object files and no build output, and it catches most of what a build would.

**Commit with explicit pathspecs only.** Never `git commit -a`, never
`git checkout -- .`. The other session's uncommitted work is not recoverable by
git and a careless checkout destroys it.
