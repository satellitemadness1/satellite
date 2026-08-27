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

Nothing here asks for a line-by-line port.

## 0. Both sides bend, and QUAD bends more

This is the rule the rest of the document is written under, and getting it backwards
produces a language shaped by one program's C++ conveniences.

**Neither side is fixed.** QUAD is bent to fit satellite and satellite is bent to fit
QUAD, at the same time. But the two are not equal partners: **satellite is the thing
being built, so the burden of proof sits on satellite changing.** A feature only
enters the language when QUAD's *meaning* cannot survive without it.

The test, applied to every item in §3: **is the C++ feature carrying QUAD's meaning,
or only its convenience?** Convenience bends. Meaning is a language requirement.

This is not a softening of the acceptance test. A hole is still a hole. But the
first draft of §3 was written from QUAD's includes and derived requirements
one-directionally, and four of its six entries dissolved the moment the test above
was applied to the actual source.

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
| `rack.hpp` | 81 | the impulses that do the thinking |

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
| `string`, `sstream` | `satellite.variable.string`, and its sixteen methods at `1 6 1 1`+ |
| `fstream` | `satellite.variable.file`, and `.read_line` / `.write_line` at `1 6 2 3`+ |
| `random` | `satellite.random.*`, three tiers |
| `chrono` | `satellite.time.*` |
| `csignal` | Ctrl-C, and DESIGN §10.2 already says it means two things |
| **no** templates, **no** `virtual`, **no** `shared_ptr`/`unique_ptr` | nothing to port — satellite defers generics (§12) and QUAD does not need them |

That last row matters more than it looks: **QUAD does not use the C++ features
satellite deliberately refuses.** No user-defined generics, no inheritance
hierarchies, no smart-pointer graphs. The refusals in DESIGN §12 cost this program
nothing.

**One correction to an earlier version of this table.** It credited QUAD with
`thread` and `atomic` against `satellite.variable.thread`. **QUAD spawns zero
threads.** The only use of `<thread>` in all 3029 lines is
`std::this_thread::sleep_for` at `quad_main.cpp:260`, and there is no `std::atomic`
and no `std::mutex` anywhere. What QUAD needs is a **sleep** —
`satellite.time.sleep(n)` at `1 9 3`. Threads are not a QUAD requirement at all, and
the design work under DESIGN §13 stands on the language's own account.

---

## 3. The holes

Every one of these was something QUAD needs and satellite had not settled. Four are
now closed. Ordered by what is left.

### 3.1 `satellite.variable.float` — still open, and it is the whole gap

QUAD is **164 `double`s** across eight files: activation, altitude, salience,
strength, weight, urgency, temperature. It cannot be written without arithmetic on
non-integers, and this is the one entry where satellite bends rather than QUAD.

DESIGN §13 framed the fractional half as a choice between *lazy* and *bounded by a
precision dial*. **Reading the source shows that framing is answering the wrong
question**, in two ways:

**A dial is not a convenience — it is the only way some answers exist.**
`rack.hpp:59` computes `pow(urgency, exp)` where `exp = 0.15 + 4.5 * (1 - t)`,
always fractional, ranging 0.15 to 4.65. `x^y` at fractional `y` has no exact
decimal value **at any length**. An exact arbitrary-precision type cannot represent
it and "lazy" has nothing to be lazy about. The answer must be rounded to exist, so
a rounding rule is part of the type rather than a setting on it.

**And exactness fails on the hot path.** `sky.hpp:355` runs `activation *= keep`
every tick on every node, with `keep = 0.15 + 0.85·alt²`. Exact decimal
multiplication adds the operands' digit counts — a thousand ticks is thousands of
digits per node, for a number the program prints at `%.2f`.

**How much precision QUAD actually needs is already on record, by its own hand.**
`sky.hpp:461` writes the entire mind to its `.sky` file with bare `operator<<` —
**six significant digits** — and reads it back. Every save. The program round-trips
its whole state through six digits and keeps working. Its display is two.

**The representation was decided on 2026-08-27: a float is a `satellite.variable.bool`
and two `satellite_number`s** — a sign, and one number for each side of the decimal
point (DESIGN §8.6). The sign is held once, in the bool, and the two halves are
magnitudes that never carry one. The left half stays exact and
unbounded; the right half is bounded, which is where QUAD's per-tick decay grows and
where `pow`'s irrational answer has to be rounded to exist. The right half's length is
the precision, so `satellite.library.system.float_digits` (`1 14 2 4`) becomes the
**default** length rather than a global dial — which is what QUAD needs, holding
activations near six digits and printing them at two.

**What is still open is the rounding rule** — truncate, half-up, or half-even. QUAD's
invariant 8 is determinism, so its behaviour depends on the answer. PLAN §8 puts the
float in **M9**, which cannot land until the rule is chosen.

**This puts DESIGN §8.1 under pressure and §8.1 now says so.** Its argument — no
`double`, exact always, guarded at the C++ type level — is right for
`satellite.variable.number` and is not available one type over.

**`pow` itself bends QUAD's way.** The exponent's entire job is to flatten or sharpen
a roulette wheel — `rack.hpp` calls it simulated annealing over meaning. The dial is
load-bearing; `pow` being `pow` is not.

### 3.2 A set and a deque — closed, declined

*(2026-08-27.)* QUAD's five `std::set` are three `set<Id>` used for membership and
canonical ordering, and two `static const set<string>` stopword tables. All five are
membership tests, which a map with no values answers — and the map now has its own
methods at `1 4 1 1` through `1 4 1 9`, where before it had none at all.

Its two `std::deque` are `journal_` and `speech_`, both bounded ring buffers: push
back, pop front at a cap. `list.remove_first()` at `1 4 2 16` and `truncate(n)` at
`1 4 2 14` answer them.

`satellite.container.set` is deliberately absent and **`1 4 5` is free** if something
later earns a real one.

### 3.3 Sorting — closed, and it does not need capsules as values

*(2026-08-27.)* This entry used to say sorting was blocked on DESIGN §12's deferral
of *a bare name can be a value*, because passing a comparator requires it. **That is
not true of this program.**

Ten `std::sort`, three with no comparator. **All seven comparators are the identical
shape** — mind.hpp:624, mind.hpp:879, flock.hpp:231, flock.hpp:271, view.hpp:186,
sky.hpp:403, sky.hpp:597:

```cpp
if (kx != ky) return kx > ky;   // one numeric key, descending
return x < y;                    // ties by identity, ascending
```

Seven for seven. And `flock.hpp:232`'s own comment says why the tie-break is there —
`// deterministic ties` — because `std::sort` is not stable.

So QUAD needs **one primitive**, not a comparator: sort descending by a key, ties by
identity. `satellite.container.list.sort_down(key)` is `1 4 2 6`, with `sort_up(key)`
at `1 4 2 7` and the bare forms at `1 4 2 3`–`1 4 2 5`. Where a key would be awkward,
QUAD bends: `mind.hpp:879` already builds `vector<pair<double,Id>>` by hand, so it can
always hand over pairs.

**DESIGN §12's deferral survives**, and §2 stays shut.

### 3.4 The terminal display — closed, and it was the wrong gap

*(2026-08-27.)* This entry used to say satellite needs "a console namespace that can
address the screen." **`view.hpp` never moves a cursor.** Its entire escape
vocabulary is four things — `\x1b[H` (home), `\x1b[J` (erase to end), `\x1b[2J`
(clear), and SGR colour — plus `ioctl(TIOCGWINSZ)` for width and height.

And it builds **the whole frame as one string** (`out.reserve(8192)`) and writes it
with one `fwrite` + `fflush`. That is exactly the shape DESIGN §10.1 already
specifies: the unit queued is a whole string, and a line stays atomic.

The real gap is elsewhere. `quad_main.cpp:34` puts the terminal in `~(ICANON|ECHO)`
with `VMIN=0, VTIME=0`, and it is **not for single keypresses** — every QUAD command
dispatches on `\n` (`/q`, `/p`, `/s`, `/f`, `/w WORD`, `/nw` are all enter-terminated).
It is there for two other reasons:

1. **`VMIN=0`** — `read()` never blocks, so the mind keeps ticking while you type.
2. **`~ECHO`** — the tty does not print your characters, because the frame draws them
   itself at `> typing_`.

`ISIG` is left on, so Ctrl-C still raises SIGINT and DESIGN §10.2 is untouched.

**satellite answers (1) and QUAD bends on (2).** DESIGN §10.1 now specifies a reader
thread — §10.3's polarity flip run once more — that blocks on stdin and pushes whole
lines into a queue; `satellite.console.typed()` at `1 5 5` asks it and gets a line or
nothing, immediately. That also retires QUAD's `VMIN=0` poll loop, which is strictly
worse than a thread that waits properly.

Echo-off exists solely so QUAD can draw the half-typed line inside its own frame. Let
the tty echo and drop the `> _` row. **satellite then needs no terminal control at
all** beyond width, height, home and clear — `1 5 6` through `1 5 9`. The cost is
cosmetic: your typing appears below the frame rather than inside it.

### 3.5 Map keys — closed, and one finding underneath

*(2026-08-27.)* DESIGN §6.5 restricts what may be a map key, and §8.4 explains it is
the deadlock one level down rather than a preference. **Whether QUAD's keys fall
inside the restriction is now checked, and they do.**

Every map in QUAD: `map<Id,Thought>`, `map<Id,Bond>`, `map<Id,vector<Id>>`,
`map<Id,double>`, `map<uint64_t,int>`, `map<string,int>`, `map<string,string>`,
`unordered_map<string,Id>`, `unordered_map<uint64_t,bool>`, `unordered_map<Id,double>`.
**Every key is a number or a string. None is a spacesuit.** The restriction costs
QUAD nothing.

Two of them pack a pair of ids into one key with `(a << 32) | b`. With
arbitrary-precision numbers that is `a * 4294967296 + b` and needs no bitwise
operators, so DESIGN §5.5's silence about `|` never has to be broken.

**The finding underneath, which the first draft missed: `std::map<Id, Thought>` is
key-ordered and `satellite.container.map` is insertion-ordered** (DESIGN §8.4).
QUAD's invariant 8 is determinism, and insertion order is deterministic too, so the
invariant survives. But `for (auto& [id,t] : thoughts_)` currently walks ids
ascending, and that feeds five of §3.3's seven sort inputs. **QUAD bends** — it takes
insertion order and keeps the identity tie-break that `sort_down(key)` provides
anyway.

### 3.6 Behaviour stored in a field — closed by a feature satellite already needed

*(2026-08-27.)* This entry used to read *"if QUAD stores callables"*. It does, and it
is not incidental: `rack.hpp:22` is `std::function<void(Mind&)> act` as a **field of
`Impulse`**, and `Impulse` is the unit of all of QUAD's work — *"nothing in this mind
is thought by a central thinker."* Eleven `rack_.post({name, urgency, lambda})` sites
in `mind.hpp`. It is stored, moved into a vector, held across ticks, drawn by a
weighted lottery, and invoked later.

But look at what is captured. **Eight of the eleven capture nothing.** The other
three capture **by value, scalars and ids only** — `[thought]`, `[thought]`,
`[a, b, via, kind, strength]`. Nothing captures by reference; nothing captures a
mutable environment. So the requirement is *a capsule plus a copied argument bundle*,
which is precisely the version that does not fight DESIGN §7's frames, because
nothing outlives a frame.

And satellite already had to express exactly that, for its own reasons:

```satellite
satellite.variable.thread my_thread = satellite.thread.new(capsule_name(args))
```

That parses today with **no change to DESIGN §6**. `capsule_name(args)` at that
position is not performed — it is *packaged*: the handler evaluates the arguments and
stores `(capsule number, argument values)`. Its type is
`satellite.variable.capsule` at `1 6 16`.

**It is a call form, not a bare name used as a value**, so DESIGN §12's deferral
holds and §2 stays shut. QUAD's rack gets built out of a feature threads already
required — and `Impulse` already carries a `name` for its census and journal, so
storing a capsule identity rather than a closure is arguably the shape it wanted.

---

## 4. What this changes about the plan

**Nothing before M8.** The trie, the lexer, the arena, the parser, resolve and the
closure tree are all needed whatever QUAD turns out to require, and none of the holes
above touches them.

**M2 got larger, and it is the numbering rather than the code.** *(2026-08-27.)*
Settling §3 added **71 numbers** to WORD_NUMBERS.md §2.2, taking it from 144 entries
to 215 — the map's nine methods and the string's sixteen, where both had none at all;
twenty-three more on the list; ten on the number; five on the file; five on the
console; `satellite.variable.capsule`; `satellite.time.sleep`; and
`satellite.library.system.float_digits`. All of it validates the way M2's
`static_assert`s will: no duplicates, no holes, no orphans, no alias pointing at
nothing.

**M9 and M10 carry the rest.** PLAN §8's M9 is scalars and control flow, M10 is
containers and the search power. What QUAD adds that is not currently in either:

- `satellite.variable.float`, decided in DESIGN and built (§3.1) — **the only one
  still open**
- a stable sort, descending by key (§3.3)
- a console reader thread and `typed()` (§3.4)

**And it still suggests a milestone that does not exist yet:** one whose done-when
condition is *a piece of QUAD, running*. Not the whole program — one mechanism out of
`mind.hpp`, chosen because it exercises floats, containers, sorting and persistence
at once.

---

## 5. The honest risk

**This document is no longer written from QUAD's shape.** The first version was — from
its includes, its structure, its own README and DESIGN — and §5 said so, and said the
next real step was to read it. That has now happened for the load-bearing parts:
`rack.hpp` and `quad_core.hpp` in full, and every sort, every map, every container
declaration, every terminal escape and every use of `<thread>` and `<cmath>` across
all eight files, counted rather than inferred.

**Four of the six holes closed and one grew teeth.** That ratio is the argument for
reading source before deriving requirements from it, and it is also the warning: the
first draft would have spent DESIGN §12's deferral of *a bare name can be a value* on
a requirement that was never there.

What has **not** happened is writing a mechanism out of `mind.hpp` in satellite by
hand, line by line, against DESIGN.md as it stands. §3 is a floor on what is missing
and not a ceiling, because the gap between *"the language has floats"* and *"this
expression is writable"* is where languages actually fail. That is still a day of
work and it will find things this list does not have.

The candidate mechanism is `Sky::decay` plus `Rack::draw` — between them they touch
floats, the map, a weighted pick, and the one `pow` that §3.1 says has no exact
answer.

*Companions: [DESIGN.md](DESIGN.md) — what the language is. [PLAN.md](PLAN.md) —
how it gets built, and the milestones §4 above amends. [WORD_NUMBERS.md](WORD_NUMBERS.md)
— every number, including the 71 §3 added. [SATC.md](SATC.md) — the cached form, and
why most of those numbers never appear in it.*
