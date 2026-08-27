# satellite — the goal

**This file is permanent, and it is why the language exists.**

satellite is a general-purpose language and everything in DESIGN.md is meant
generally. But the thing it is *for* — the program that decides whether the
language succeeded — is this one:

> **`/home/madness/code/cxx/quad_infinity/` must be expressible in satellite.**

QUAD ∞ is *"a running model of a mind. Thoughts arrive, most of them leave, a few
fly together, and what holds together gets written back as something the mind
knows."* It is **3029 lines of C++ across eight files**, it runs interactively in a
terminal, it reads corpora, and it persists what it works out.

Nothing here asks for a line-by-line port. The test is **expressiveness**: can a
program of that shape be *written* in satellite without the language fighting it.
A feature QUAD needs and satellite refuses is a hole in the language, and it is
better to find those now — while DESIGN.md is still being written — than at M10
with an interpreter already built around their absence.

---

## 1. What QUAD is made of

| file | lines | what it does |
|---|---:|---|
| `mind.hpp` | 1097 | the mechanism — thoughts, bonds, wants, crystallisation |
| `sky.hpp` | 642 | persistence: reading and writing a `.sky` memory file |
| `view.hpp` | 343 | the live terminal display |
| `flock.hpp` | 316 | thoughts that fly together |
| `quad_main.cpp` | 268 | the loop, the command line, the keys |
| `seed.hpp` | 171 | where thoughts come from |
| `quad_core.hpp` | 111 | the shared shapes |
| `rack.hpp` | 81 | |

Its own `DESIGN.md` has §5 *Invariants* — *"break one and this stops being this
system"* — and §11, a log of every design bug found by running it. Both are
load-bearing, and both are the kind of document this project already writes.

## 2. What it asks of a language

Counted from the source, 2026-08-27.

**Already decided in satellite, and fine:**

| QUAD uses | satellite has |
|---|---|
| 38 `struct`, 11 `class` | `satellite.spacesuit`, with `.protected` / `.public` (DESIGN §13) |
| `vector`, `map`, `unordered_map` | `satellite.container.list`, `satellite.container.map` |
| `string`, `sstream` | `satellite.variable.string` |
| `fstream` | `satellite.variable.file`, `satellite.file.*` |
| `thread`, `atomic` | `satellite.variable.thread` (DESIGN §10.4, PLAN M12) |
| `random` | `satellite.random.*`, three tiers |
| `chrono` | `satellite.time.*` |
| `csignal` | Ctrl-C, and DESIGN §10.2 already says it means two things |
| **no** templates, **no** `virtual`, **no** `shared_ptr`/`unique_ptr` | nothing to port — satellite defers generics (§12) and QUAD does not need them |

That last row matters more than it looks: **QUAD does not use the C++ features
satellite deliberately refuses.** No user-defined generics, no inheritance
hierarchies, no smart-pointer graphs. The refusals in DESIGN §12 cost this program
nothing.

## 3. The holes

Every one of these is something QUAD needs and satellite has **not** settled.
Ordered by how much they hurt.

### 3.1 `satellite.variable.float` — DESIGN §13 lists it as OPEN

QUAD includes `<cmath>` and is a model of a mind: weights, activations, decay,
temperature. **It cannot be written without real arithmetic on non-integers.**

DESIGN §8.1 gives the integer half — base-10⁹, exact, arbitrary precision — and
§13 leaves the fractional half undecided between *"lazy, or bounded by a precision
dial."* That decision is now on the critical path rather than a nicety, because
`satellite.variable.float` is `1 6 10` in WORD_NUMBERS.md and there is a program
that needs it to mean something.

**This is the largest gap in the language.**

### 3.2 There is no `set` and no `deque`

QUAD uses `std::set` and `std::deque`. WORD_NUMBERS.md numbers exactly two
containers — `satellite.container.map` at `1 4 1` and `.list` at `1 4 2`.

A set is a map with no values and a deque is a list with a cheap front, so neither
is deep work. But each is a **new child of `satellite.container`**, which under
DESIGN §4.3 means a number assigned now and frozen forever, and the next free slot
after `result` at `1 4 4` is `1 4 5`.

### 3.3 There is no `sort`, and `<algorithm>` is everywhere

A mind that ranks thoughts sorts them. satellite has no sort, no ranking, no
comparator, and the obvious spelling — a method on a list taking a capsule —
collides with DESIGN §12's deferral of *"a bare name can be a value"*, which is
exactly what passing a comparator requires.

**So sorting is blocked on the same decision that blocks first-class capsules**,
and QUAD needs it. This should move out of §12's deferred list.

### 3.4 The terminal display

`view.hpp` draws a live panel — cursor positioning, redraw, a header, panels that
update while the program runs. satellite has `satellite.console.display` and
`satellite.console.input` and nothing that moves a cursor.

`satl-term` exists and is a GTK4+VTE window (PLAN M11), which is a different thing:
it *hosts* a terminal rather than drawing in one. Either satellite grows a console
namespace that can address the screen, or QUAD's interface is expressed some other
way, and that is a language decision rather than a QUAD decision.

### 3.5 Map keys

QUAD keys maps by things that are not strings. DESIGN §8.4 restricts what may be a
map key, and §6.5 explains why it is not a preference — *"a map's hash and equality
run inside that lock, so they must be native and can never be satellite code."*
Whether QUAD's keys fall inside that restriction has not been checked.

### 3.6 `std::function`, and thoughts that hold behaviour

`<functional>` is included. If QUAD stores callables, DESIGN §12's deferral bites
again — the same one as §3.3.

---

## 4. What this changes about the plan

**Nothing before M8.** The trie, the lexer, the arena, the parser, resolve and the
closure tree are all needed whatever QUAD turns out to require, and none of the
holes above touches them.

**Everything from M9 on.** PLAN §8's M9 is scalars and control flow, M10 is
containers and the search power. QUAD says what has to be true by the end of M10,
and it adds work that is not currently in either:

- `satellite.variable.float`, decided in DESIGN and built (§3.1)
- `satellite.container.set` and a deque or a list with a cheap front (§3.2)
- sorting, and therefore capsules as values (§3.3 and §3.6 are one decision)
- a console that can address a screen (§3.4)

**And it suggests a milestone that does not exist yet:** a milestone whose
done-when condition is *a piece of QUAD, running*. Not the whole program — one
mechanism out of `mind.hpp`, chosen because it exercises floats, containers,
sorting and persistence at once. A language that can run that can probably run the
rest, and a language that cannot has learned it early and cheaply.

---

## 5. The honest risk

This document is written from the **shape** of QUAD — its includes, its structure,
its own README and DESIGN — and not from having read all 3029 lines. §3 is
therefore a floor on what is missing and not a ceiling.

The next real step is to take **one mechanism** out of `mind.hpp` and try to write
it in satellite by hand, on paper, against DESIGN.md as it stands. That is a day of
work and it will find things this list does not have, because the gap between *"the
language has floats"* and *"this expression is writable"* is where languages
actually fail.

*Companions: [DESIGN.md](DESIGN.md) — what the language is. [PLAN.md](PLAN.md) —
how it gets built, and the milestones §4 above amends. [WORD_NUMBERS.md](WORD_NUMBERS.md)
— every number, including the ones §3 says need assigning.*
