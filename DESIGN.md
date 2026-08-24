# satellite — language design and implementation plan

Status: runtime substrate, lexer, tree, parser, resolver and evaluator all landed,
including §6's frames and capsule calls. §14 adds spacesuits (classes) with
inheritance, access control and constructors, and `satellite.variable.time` with
`satellite.time.now()`. §8.1's `Number` migration is **done**: `double` is gone
from the variant and `satellite.variable.number` is an exact arbitrary-precision
decimal. §8.3's `file` is done, so a satellite program can now read and write
one, which is what §16 was waiting on. The interpreter and the GUI terminal are
two binaries, `satl` and `satl-term` (§9). **§16 is done**: a unit of includable
code is a **spaceship** (named 2026-08-18), `Span` carries a file id so an error
names the spaceship it is in, and `loader.cpp` merges every included spaceship
into one Program before resolve() runs — so `satellite.include(my_parser)`
works, include-once and cycles included. Still outstanding: the window (M7),
which is also the native-`.so` half of a language-owned spaceship. **§19 is
done**: nine surface-syntax additions taken from running a real 13,273-line
program against the interpreter, including quoted `satellite.include` paths, the
list literal, and a redeclaration that rebinds. **§21 is done**:
`satellite.variable.binary` and `satellite.variable.hex` (with `hexadecimal`
as the language's one alias) are real types with `x00FF` and `b1010` literals,
and the width is part of the value — `x0009` is not `x9`, which is the whole
reason they are not number literals in another base. §20 is still plan only.
Last updated: 2026-08-24

This file is the index. The design itself is twenty-one numbered sections under
[`design/`](design/), one section per file, and it fixes the syntax, names the
decisions still open, and lays out the build order. Everything marked
**verified** was checked by compiling and running code against the real source
in this repo, not reasoned about on paper.

---

## The twenty-one sections

**A file is numbered by its section**, so `§14` is `design/14-*.md` and there is
nothing to look up. §1–§8 and §14 are the language; §9–§11 and §17 are the
implementation; the rest is what is deferred, what is decided, and what a real
program asked for. §20 is the first section written entirely ahead of the code
it describes, and says so at the top. Read them in section order — that is the order they were
written to be read in.

| section | file | what it settles |
| --- | --- | --- |
| §1 | [The generating rule](design/01-the-generating-rule.md) | the one invariant every naming rule in the language follows from |
| §2 | [Hello world](design/02-hello-world.md) | the smallest program, and what each of its five lines is for |
| §3 | [Lexical structure](design/03-lexical-structure.md) | tokens, strings, the escapes, and duration literals |
| §4 | [The reservation rule (what makes generics parse)](design/04-the-reservation-rule.md) | what makes `<` and `>` parse as generics rather than comparison |
| §5 | [Grammar (v1)](design/05-grammar.md) | the whole surface, as productions |
| §6 | [Scope model — the most important decision](design/06-scope-model.md) | slot-indexed frames, and why a slot is never reused across scopes |
| §7 | [Method calls, indexing, slicing](design/07-method-calls-indexing-slicing.md) | the selector rule, and what indexing and slicing do at the edges |
| §8 | [Types](design/08-types.md) | every type: the exact-decimal `number`, containers, `file`, writing a bool |
| §9 | [Runtime architecture](design/09-runtime-architecture.md) | two binaries, the console thread, and what was measured rather than assumed |
| §10 | [Evaluator](design/10-evaluator.md) | the walk, and what it does at each kind of node |
| §11 | [Build order](design/11-build-order.md) | the milestones in order, and which of them earned a test binary |
| §12 | [Deliberately deferred](design/12-deliberately-deferred.md) | what is not in the language on purpose, and what came off the list |
| §13 | [Decisions, and the one still open](design/13-decisions.md) | what is settled, and the one thing that is not |
| §14 | [Spacesuits (classes)](design/14-spacesuits.md) | classes: inheritance, access control, constructors, reference semantics |
| §15 | [Bootstrapping](design/15-bootstrapping.md) | the self-hosting stages, and what each one would have to compile |
| §16 | [Including a file](design/16-including-a-file.md) | spaceships, include-once, cycles, and where a path is resolved from |
| §17 | [The abstract machine](design/17-the-abstract-machine.md) | the bytecode format, and worked dumps of real programs down to the bit |
| §18 | [`satellite.random`](design/18-satellite-random.md) | three tiers, what each costs, and what none of them claims |
| §19 | [Nine additions, and the program that asked for them](design/19-nine-additions.md) | the nine changes a real 13,273-line program asked for, and the one it did not |
| §20 | [Networking](design/20-networking.md) | a client and a server, and how a satellite value crosses to another machine |
| §21 | [Binary and hexadecimal](design/21-binary-and-hexadecimal.md) | `x00FF` and `b1010`, and why the width is part of the value |

---

## Citing this document

**By section number, never by file and never by line.** `§8.3.1` is the whole
address. Twenty comments across thirteen files under `src/`, the man page,
`install.sh`, `debian/` and everything in `plans/` already cite that way, and
every one of them still resolves after this split, because the split renumbered
nothing.

The section bodies moved **byte for byte**. The only text written for the split
is this index and two navigation lines at the top of each section.
**Verified** by extracting all nineteen sections from the single file this was
before, and from the nineteen it is now, and comparing them.

One thing the split does break, and it is worth saying here rather than leaving
to be discovered: notes that cite a **line** of the old file — thirty-five of
them under `plans/`, nearly all in `pcg_k16384_spec.md`, `DESIGN.md:3051` and
the like — now point at nothing. They were already fragile against any edit
above them, which is exactly why `plans/todo.txt` says references are by section
number and never by line number. The prose those notes quote is intact and
findable by its words.
