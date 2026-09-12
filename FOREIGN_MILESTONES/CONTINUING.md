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
| [CXX23/](CXX23/) | 318 features | 266 | rows 57–318 each have a page; rows 1–56 still have 4 of 56; 45 rows are **open** |
| [CXX26/](CXX26/) | 22 features | 5 | **complete** — every link resolves |
| [PYTHON/](PYTHON/) | 20 rows | 0 | index complete, no pages |
| [JAVA/](JAVA/) | 20 rows | 0 | index complete, no pages |
| [RUST/](RUST/) | 20 rows | 0 | index complete, no pages |
| [ASM/](ASM/) | — | 0 | README is the whole answer, deliberately |

**Every index carries the right answer in every row.** The folders with no pages
are not unfinished in the sense that matters — a reader gets the correct answer
from the table. Pages are the elaboration.

### The remaining work, in the order it is worth doing

1. **Talk through CXX23's 45 open rows with the author.** Done 2026-09-12:
   the catalogue went from 56 rows to 318, back to C++98, with a *since* column
   on rows 57 onward. The author asked for every C++ feature to have its own
   entry, so **rule 1.5 now governs how long a page is, not whether it exists**,
   for C++. Rows that need a decision are marked `open` and listed at the foot
   of [CXX23/README.md](CXX23/README.md). Each should become one of the three
   real answers. Several share a decision (program namespaces, constructor
   arguments, `enum`).
1a. **Write the 52 missing pages for rows 1–56**, so that every index link
   resolves, as CXX26 already does. Also add *since* markers to those rows.
2. **PYTHON/, JAVA/, RUST/ pages** — the user's stated order after C++. Apply
   rule 1.5 hard: most rows do not need one. The ones that do are where a
   reader carries a wrong assumption across, and each index already flags its
   own: Python's `self` (row 16), Java's garbage collection vs refcounting
   (row 10), Rust's `Arc`/`Mutex` (row 18).
3. **More `foreign.cpp` rows** as the tables grow. 128 today.
4. **Corrections found 2026-09-12 that belong to the interpreter session**, not
   this folder, and so are recorded rather than edited:
   - `foreign.cpp`'s `elif` row says there is no `else if`. There is:
     `satellite.statement.else satellite.statement.if (...)` runs ([CXX23/M143](CXX23/M143.md)).
   - `!` works; M28's page and CXX23 row 20 had it as missing ([CXX23/M131](CXX23/M131.md)).
   - `foreign.cpp`'s M40 notes (copy-on-write containers, atomic refcounts) and
     [SATELLITE/M40](SATELLITE/M40-locks-and-atomics.md) / [CXX23/M46](CXX23/M46.md)
     (corrupted lists, non-atomic refcounts) contradict each other. One is wrong.
   - `README.md` says `tests/foreign_test` enforces the promise rule. No such
     test exists; §1.2's shell snippet is the only check.

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

---

## 6. How a language catalogue gets written, and the next one: Java

**Written 2026-09-12, after CXX23 went from 56 rows to 318, for a session
starting with none of that conversation.**

### 6.1 What the author wants, in their words

The author's request was to *"figure out what C++ features do not yet have
milestones, and add all of those features as milestones — there should be like
200 or more"*. **The same is now wanted for JAVA/**: every Java feature gets a
numbered row and its own page. Rule 1.5 decides how long a page is, not whether
it exists. Work **only inside FOREIGN_MILESTONES/**, because another window is
changing the interpreter.

**PLAN.md's M-numbers are a different series.** The author said so directly. Do
not confuse PLAN M27 (the network) with SATELLITE/M27 (break), or with a catalogue
position like JAVA/M27.

### 6.2 The method that worked for CXX23

1. Read the existing index; its rows keep their numbers and new rows append.
2. Enumerate the language by version, and by area (syntax, types, OOP,
   generics, lambdas, exceptions, the standard library by package, concurrency).
3. **Check every satellite claim before writing it.** `./satl --words` prints
   all 367 paths; `satellite.help(path)` inside a probe program says what one
   does; a probe `.satl` file in the scratchpad, run with `./satl file.satl`,
   settles behaviour. Running `satl` is read-only and safe while the other
   window builds. Never edit `src/`.
4. Five answers: **says it**, **partial**, **M<n>** (only an existing
   SATELLITE/ or CXX26/ milestone, M27–M45; never invent one), **never** (with
   the reason), **open** (needs the author's decision; listed at the foot of the
   index for a later conversation).
5. Generate pages and index rows from one data file:
   [tools/gen_cxx23.py](tools/gen_cxx23.py) is the CXX23 one. Copy it to
   `tools/gen_java.py`, replace the entries, and set `OUT` and `FIRST`. It refuses to
   overwrite a page. Its `link()` turns bare M27–M45 into milestone links and
   leaves `[M4](M4.md)` catalogue links alone. Escape `|` in table cells.
6. Merge the generated rows into the index, extend its "milestones this folder
   asks for" table, list the open rows, and update §3 above.

### 6.3 Facts already checked on 2026-09-12 (re-probe if the interpreter has moved)

**Works:** unary minus; `!`; `%`; `== != < <= > >=` on numbers; `==`/`!=` on
strings; `else` directly followed by `if`; bare `{ }` blocks with C++-style
shadowing; declarations in a `for` header; `return` from inside a loop;
recursion (exact big results); `satellite.library.x` globals; list of lists;
`l[i] = v`; escapes `\n \t \"`; binary literal `b0101`; hex literal `xFF`;
decimal literals; a number assigned to a string or float converts; `+` joins a
number onto a string; `s.to_number()`; a variant holds any type and
`holding()` names it (`number`, `string`); an unassigned variable displays
`nothing`; `satellite.system.environment("HOME")`; `list.sort()` on strings.

**Refused:** `+=`, `++`, `&`, `<<` as operators; `0xFF`, `0b…`, `1'000`, `'a'`,
`R"(…)"`; `[1, 2, 3]` list literals; `for (T x : l)`; `do`; `;` at line end;
`int a = 1, b = 2` shape; `/* */`; a capsule inside a block (S0213); a spacesuit
inside a spacesuit (S0207); constructor parameters and two superclasses on a
spacesuit (both read as a superclass list, S0201); declaring one name twice
(S0242); `<` on strings (S0712); `m.get` on a missing key (S0726); division by
zero, floats too (S0601); `x.power(a, b)` (it takes one argument, S0722).

**Semantics:** `7 / 2` is `3.5` and `1 / 3` goes to `division_digits`;
`substring(start, end)` takes an end (`"abcdef".substring(2, 4)` is `"cd"`);
assigning a list copies it, and so does passing one to a capsule (CXX23/M9),
while a spacesuit is a shared handle; a discarded return value and an unused
variable are both silent; `satellite.time.sleep(n)` is milliseconds;
`satellite.system.delete(x)` removes a file or an empty directory; `list.search("an")`
over `apple, banana` answered `[]`, so it is not substring search.

**Not in the word tree:** trigonometry/log, gcd, popcount-style bit ops,
bitwise and/or/xor methods, creating/renaming directories, running a process,
stderr, seek/binary file I/O, enums, type aliases, program namespaces,
`this`, weak references, a monotonic clock.

### 6.4 Java, specifically

[JAVA/README.md](JAVA/README.md) has 20 rows against Java SE 21 (LTS). Keep
them as JAVA/M1–M20 and append from M21. Use a *since* column with Java
versions (1.0, 5, 8, 17, 21), since records, sealed classes, pattern-matching
`switch`, text blocks, `var`, streams and virtual threads each arrived at a
known release. Java cases that are not in CXX23 and need their own thinking:
garbage collection versus refcount cycles (row 10), `null` and
`NullPointerException`, checked exceptions, `interface` default methods,
records, sealed types, enums with bodies, autoboxing, `String` immutability
and `==` versus `.equals`, `synchronized` and virtual threads (M40, and the
open contradiction in §3 item 4), reflection and annotations (M41), and
`Object` methods (`equals`, `hashCode`, `toString`, which M38 and
DESIGN §12's deferred printer `to_string` bear on).
