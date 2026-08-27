# satellite — the numbering

**This file is permanent, and it is the authority.** Every number in the language
lives here first. [DESIGN.md §4](DESIGN.md) explains the scheme and
`src/satellite_words/words.def` transcribes it into something a compiler checks —
but when any of the three disagree, **this file is right and the other two are a
bug.**

That ordering is the project's own rule, stated in the first satellite and broken
there: *prose may explain a number; it may never be the only place the number
lives.*

---

## 1. The rule

**Every node numbers its own children, starting at 1. A path is that sequence of
numbers, read left to right.** `satellite` is 1 because it is the root.

```
satellite . console . display
    1     .    5    .    1
```

`console` is 5 because it is `satellite`'s fifth child. `display` is 1 because it
is `console`'s first child.

**The lists are per-parent, not per-level.** `display` and `input` are numbered
under `console`; `string` and `number` are numbered under `variable`. Those are two
separate lists and both start at 1, so `1 5 2` and `1 6 2` both end in 2 and the
two 2's have nothing to do with each other. That is the scheme working. A number
means nothing on its own; it means something *at a position, under a parent*.

### 1.1 The order is the order words first appear

This is what decides the numbers, and it is not a ranking. **Walk a real program
from the top and change the number only when you must.** Hello world is the program
that fixed the first six:

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("Hello, World!")

    satellite.return(satellite)
}
```

`include` is met first, so it is 1. `capsule` is next, so it is 2. Then `main`,
then `container`, then `console`, then `variable`. Nothing about that order claims
`include` is more important than `variable` — only that a reader meets it sooner.

The alternative was to group the namespaces by what they do and number the groups,
and it was rejected: it makes the numbering an argument about taste, and there is no
version of that argument that a second person would settle the same way.

### 1.2 The order is frozen

Because a child index *is* a position in its parent's list, **the order words are
registered in is the numbering**:

> **Never renumber. Never reuse. Always append.**

A new child of `console` goes on the end. Removing one leaves a hole rather than
shifting its neighbours down, because shifting them would silently change what
every already-written program means.

This is stricter than the first satellite's flat global word id, and it has to be:
there, a word's number survived being moved between namespaces. Here the number
*is* the position, so the position is the thing that must not move.

### 1.3 Arguments are part of the number

A number does not identify a path. It identifies a **call shape** — the path
together with what is being handed to it — and there are two ways an argument can
show up in the sequence, decided by who owns the argument.

**A language-owned argument extends the path.** `satellite` is a word and its
number is 1, so it simply becomes the next number along:

```
satellite.include()              1 1 0
satellite.include(satellite)     1 1 1
```

**`0` means nothing in that position.** It is a real number in the sequence and not
a piece of notation, which is why `include()` and `include(satellite)` are two
different sequences rather than one path called two ways.

**A user-owned argument cannot extend anything**, because a string literal or a
variable the user named has no number to contribute. So what distinguishes those
calls is *how many* arguments there are, and each count takes its own slot under
the parent:

```
satellite.console.display        1 5 1
satellite.console.input()        1 5 2
satellite.console.input(prompt)  1 5 3
satellite.console.input(prompt, target)   1 5 4
```

`display`, and then `input` three times — because from `console`'s point of view
those are three different things a program can ask for, and telling them apart is
the whole job the number does.

Two consequences worth stating before they surprise someone:

- **Variants are not necessarily adjacent.** A second shape of a word registered
  later takes the next free slot, not the one beside its sibling. `satellite.file`
  already has `new` at 1, `open` at 2 and `clear` at 3, so a second shape of `new`
  goes to 4. That is §1.2 doing exactly what it promises.
- **Every variadic path in the first satellite is more than one number here.**
  `satellite.file.new(path[, mode])` was one entry with a `SAT_VARIADIC` arity
  column; here it is two numbers, and the arity column stops being needed because
  the number already carries it.

### 1.4 One `uint32_t` carries the whole path

The numbers are how a path is **written down**. They are not how it travels. The
trie walk happens once, when the source is read; the node it lands on has an
interned id; that single `uint32_t` is what everything downstream holds, and
dispatch is one array index and one indirect call.

Three 64-bit integers would be 24 bytes to say what 4 bytes says, and a path of
four segments is not a special case that needs a wider one — see
`satellite.random.fast.range`, which is four numbers and nothing else.

---

## 2. The numbers

`satellite` is **1**, the root, and is also a value: the singleton runtime object,
which is why `satellite.include(satellite)` and `satellite.return(satellite)` both
make sense (DESIGN §3).

### 2.1 The children of `satellite`

`satellite` is **1**, the root, and is also a value: the singleton runtime object,
which is why `satellite.include(satellite)` and `satellite.return(satellite)` both
make sense (DESIGN §3).

| | | | | | |
|---:|---|---:|---|---:|---|
| 1 | `include` | 9 | `time` | 17 | `bool` |
| 2 | `capsule` | 10 | `spacesuit` | 18 | `directory` |
| 3 | `main` | 11 | `protected` | 19 | `help` |
| 4 | `container` | 12 | `public` | 20 | `network` |
| 5 | `console` | 13 | `statement` | 21 | `returns` |
| 6 | `variable` | 14 | `library` | 22 | `system` |
| 7 | `random` | 15 | `return` | 23 | `thread` |
| 8 | `file` | 16 | `analyze` | 24 | `window` |

1 to 15 were written by hand. **16 to 24 were assigned** on 2026-08-27 by §1's
next-lowest-free rule, with alphabetical order as the tie-break — a stated,
reproducible way to order words no program has met yet, so that nothing in the
sequence encodes an opinion about which namespace matters more.

### 2.2 Every number

Rows marked *assigned* were derived by §1's rules rather than written by hand.

| path | number | |
|---|---|---|
| `satellite` | `1` | the root, and the runtime singleton |
| `satellite.include` | `1 1` |  |
| `satellite.include()` | `1 1 0` | zero arguments — 0 means nothing there |
| `satellite.include(satellite)` | `1 1 1` | `satellite` is word 1, so it extends the path |
| `satellite.include(spaceship)` | `1 1 2` | assigned — a user-named spaceship |
| `satellite.capsule` | `1 2 (0)` |  |
| `satellite.main` | `1 3 (0)` |  |
| `satellite.container` | `1 4 (0)` |  |
| `satellite.container.map` | `1 4 1 (0)` |  |
| `satellite.container.list` | `1 4 2 (0)` |  |
| `satellite.container.list.append` | `1 4 2 1` | assigned |
| `satellite.container.list.size` | `1 4 2 2` | assigned |
| `satellite.container.arguments` | `1 4 3` | assigned — the type of the arguments object |
| `satellite.container.result` | `1 4 4` | assigned — Satellite Orbit's answer |
| `satellite.console` | `1 5 (0)` |  |
| `satellite.console.display` | `1 5 1` |  |
| `satellite.console.input()` | `1 5 2` |  |
| `satellite.console.input(prompt)` | `1 5 3` | assigned |
| `satellite.console.input(prompt, target)` | `1 5 4` | assigned — a place, not a value |
| `satellite.variable` | `1 6 (0)` |  |
| `satellite.variable.string` | `1 6 1 (0)` |  |
| `satellite.variable.file` | `1 6 2 (0)` |  |
| `satellite.variable.file.new` | `1 6 2 1` | assigned |
| `satellite.variable.file.open` | `1 6 2 2` | assigned |
| `satellite.variable.time` | `1 6 3 (0)` |  |
| `satellite.variable.number` | `1 6 4 (0)` |  |
| `satellite.variable.number.shift_left` | `1 6 4 1` | assigned — relocated from the corrected §5.5 |
| `satellite.variable.binary` | `1 6 5` | assigned |
| `satellite.variable.bool` | `1 6 6` | assigned |
| `satellite.variable.date` | `1 6 7` | assigned |
| `satellite.variable.duration` | `1 6 8` | assigned |
| `satellite.variable.expression` | `1 6 9` | assigned |
| `satellite.variable.float` | `1 6 10` | assigned |
| `satellite.variable.hex` | `1 6 11` | assigned; also spelled `hexadecimal` |
| `satellite.variable.network` | `1 6 12` | assigned |
| `satellite.variable.thread` | `1 6 13` | assigned |
| `satellite.variable.variant` | `1 6 14` | assigned |
| `satellite.variable.window` | `1 6 15` | assigned |
| `satellite.random` | `1 7 (0)` |  |
| `satellite.random.fast()` | `1 7 1` |  |
| `satellite.random.normal()` | `1 7 2` |  |
| `satellite.random.ultra()` | `1 7 3` |  |
| `satellite.random.fast(digits)` | `1 7 4` | assigned |
| `satellite.random.fast(min, max)` | `1 7 5` | assigned |
| `satellite.random.fast.range(min, max)` | `1 7 5` | ALIAS of the line above |
| `satellite.random.fast(min, max, step)` | `1 7 6` | assigned |
| `satellite.random.normal(digits)` | `1 7 7` | assigned |
| `satellite.random.normal(min, max)` | `1 7 8` | assigned |
| `satellite.random.normal.range(min, max)` | `1 7 8` | ALIAS of the line above |
| `satellite.random.normal(min, max, step)` | `1 7 9` | assigned |
| `satellite.random.ultra(digits)` | `1 7 10` | assigned |
| `satellite.random.ultra(min, max)` | `1 7 11` | assigned |
| `satellite.random.ultra.range(min, max)` | `1 7 11` | ALIAS of the line above |
| `satellite.random.ultra(min, max, step)` | `1 7 12` | assigned |
| `satellite.file` | `1 8 (0)` |  |
| `satellite.file.new(path)` | `1 8 1` |  |
| `satellite.file.open` | `1 8 2` |  |
| `satellite.file.clear` | `1 8 3` |  |
| `satellite.file.new(path, mode)` | `1 8 4` | assigned — not adjacent to shape one; §1.2 |
| `satellite.time` | `1 9 (0)` |  |
| `satellite.time.now` | `1 9 1` |  |
| `satellite.time.new` | `1 9 2` |  |
| `satellite.spacesuit` | `1 10 (0)` |  |
| `satellite.protected` | `1 11 (0)` |  |
| `satellite.public` | `1 12 (0)` |  |
| `satellite.statement` | `1 13 (0)` |  |
| `satellite.statement.if` | `1 13 1` |  |
| `satellite.statement.for` | `1 13 2` |  |
| `satellite.statement.while` | `1 13 3` |  |
| `satellite.statement.else` | `1 13 4` |  |
| `satellite.library` | `1 14 (0)` |  |
| `satellite.library.main` | `1 14 1 (0)` |  |
| `satellite.library.main.arguments` | `1 14 1 1 (0)` | assigned — the special variable, §7.7 |
| `satellite.library.main.arguments.machine` | `1 14 1 1 1 (0)` | assigned |
| `satellite.library.main.arguments.machine.cores` | `1 14 1 1 1 1` | assigned |
| `satellite.library.main.arguments.machine.cpu` | `1 14 1 1 1 2` | assigned |
| `satellite.library.main.arguments.machine.threads` | `1 14 1 1 1 3` | assigned |
| `satellite.library.main.arguments.memory` | `1 14 1 1 2 (0)` | assigned |
| `satellite.library.main.arguments.memory.total` | `1 14 1 1 2 1` | assigned |
| `satellite.library.main.arguments.username` | `1 14 1 1 3` | assigned |
| `satellite.library.system` | `1 14 2 (0)` | assigned |
| `satellite.library.system.division_digits` | `1 14 2 1` | assigned |
| `satellite.library.system.max_depth` | `1 14 2 2` | assigned |
| `satellite.library.system.min_free_mb` | `1 14 2 3` | assigned — the watchdog threshold |
| `satellite.return` | `1 15` |  |
| `satellite.return()` | `1 15 0` | RESOLVED from `?` by §1.3 |
| `satellite.return(satellite)` | `1 15 1` |  |
| `satellite.return(value)` | `1 15 2` | assigned — a user value cannot extend the path |
| `satellite.analyze` | `1 16` | assigned |
| `satellite.bool` | `1 17 (0)` | assigned |
| `satellite.bool.false` | `1 17 1` | assigned |
| `satellite.bool.true` | `1 17 2` | assigned |
| `satellite.directory` | `1 18 (0)` | assigned |
| `satellite.directory.change` | `1 18 1` | assigned |
| `satellite.directory.current` | `1 18 2` | assigned |
| `satellite.directory.exists` | `1 18 3` | assigned |
| `satellite.directory.list()` | `1 18 4` | assigned |
| `satellite.directory.list(d)` | `1 18 5` | assigned |
| `satellite.help` | `1 19` | assigned |
| `satellite.help()` | `1 19 0` | assigned |
| `satellite.help(x)` | `1 19 1` | assigned |
| `satellite.network` | `1 20 (0)` | assigned |
| `satellite.network.http(port)` | `1 20 1` | assigned — server |
| `satellite.network.https(host, port)` | `1 20 2` | assigned — client |
| `satellite.network.new` | `1 20 3` | assigned |
| `satellite.network.open` | `1 20 4` | assigned |
| `satellite.network.receive` | `1 20 5` | assigned |
| `satellite.network.http(host, port)` | `1 20 6` | assigned — client |
| `satellite.network.https(port, cert, key)` | `1 20 7` | assigned — server |
| `satellite.returns` | `1 21` | assigned |
| `satellite.system` | `1 22 (0)` | assigned |
| `satellite.system.delete` | `1 22 1` | assigned |
| `satellite.system.environment` | `1 22 2` | assigned |
| `satellite.system.home` | `1 22 3` | assigned |
| `satellite.system.memory` | `1 22 4 (0)` | assigned |
| `satellite.system.memory.bit` | `1 22 4 1` | assigned |
| `satellite.system.memory.frequency` | `1 22 4 2` | assigned |
| `satellite.system.memory.main()` | `1 22 4 3` | assigned |
| `satellite.system.memory.swap` | `1 22 4 4 (0)` | assigned |
| `satellite.system.memory.swap.free()` | `1 22 4 4 1` | assigned |
| `satellite.system.memory.swap.total()` | `1 22 4 4 2` | assigned |
| `satellite.system.memory.swap.used` | `1 22 4 4 3` | assigned |
| `satellite.system.memory.swap.free(unit)` | `1 22 4 4 4` | assigned |
| `satellite.system.memory.swap.total(unit)` | `1 22 4 4 5` | assigned |
| `satellite.system.memory.swap(unit)` | `1 22 4 4 6` | assigned — swap's own optional unit |
| `satellite.system.memory.this` | `1 22 4 5 (0)` | assigned |
| `satellite.system.memory.this.available()` | `1 22 4 5 1` | assigned |
| `satellite.system.memory.this.free()` | `1 22 4 5 2` | assigned |
| `satellite.system.memory.this.used` | `1 22 4 5 3` | assigned |
| `satellite.system.memory.this.available(unit)` | `1 22 4 5 4` | assigned |
| `satellite.system.memory.this.free(unit)` | `1 22 4 5 5` | assigned |
| `satellite.system.memory.free()` | `1 22 4 6` | assigned |
| `satellite.system.memory.total()` | `1 22 4 7` | assigned |
| `satellite.system.memory.used()` | `1 22 4 8` | assigned |
| `satellite.system.memory.main(unit)` | `1 22 4 9` | assigned |
| `satellite.system.memory.free(unit)` | `1 22 4 10` | assigned |
| `satellite.system.memory.total(unit)` | `1 22 4 11` | assigned |
| `satellite.system.memory.used(unit)` | `1 22 4 12` | assigned |
| `satellite.system.threshold()` | `1 22 5` | assigned — read |
| `satellite.system.threshold(n)` | `1 22 6` | assigned — set |
| `satellite.thread` | `1 23 (0)` | assigned |
| `satellite.thread.new` | `1 23 1` | assigned |
| `satellite.window` | `1 24 (0)` | assigned |
| `satellite.window.new` | `1 24 1` | assigned |

### 2.3 Two spellings, one number

An alias is not a second word and does not take a second number:

| | |
|---|---|
| `satellite.random.fast.range(min, max)` | the number of `satellite.random.fast(min, max)` |
| `satellite.variable.hexadecimal` | the number of `satellite.variable.hex` |
| `arg` `args` `argz` `argument` `arguments` `argumentz` | one node, six spellings (DESIGN §7.7) |

DESIGN §4.4 describes the opposite arrangement — two nodes sharing one piece of
text, as `list` under `container` and `list` under `directory` do. **Both
directions are real** and the spelling table has to hold each: deduplication is
many-nodes-one-string, aliasing is one-node-many-strings.

## 3. User-defined names take the next free number

A capsule and a spacesuit are the user's, not the language's, but they still need
identity — and the identity they get is a number under the node that owns them,
allocated **when the name is first met** rather than frozen in advance:

> all user defined capsules and all user defined spacesuits need to grab the next
> available number; the language needs to keep and always have ready the next
> available number

So `satellite.library.main` is `1 14 1` because the language put it there, and a
user's `x` is `1 14 ?` — whatever number is free under `library` at the moment `x`
is read.

Three consequences, and they matter enough to say once, here:

- **Every node keeps a live count of its children**, so "the next available number"
  is a read rather than a search. The language's own children are numbered first
  and frozen; the user's are allocated after them.
- **§1.2's freeze applies to the language's words, not to the user's.** A user name
  cannot be frozen across programs, because it is not the same name in two of them.
  *Never renumber, never reuse, always append* is a promise about `words.def`.
- **A user name's number is not stable between runs**, and anything that writes one
  down — a saved program, a wire format — must record the name and not the number.
  For dispatch inside one run it is exactly as good as a frozen one.

---

## 4. Still to be decided

**Settled 2026-08-27: `0` is a real number meaning nothing in that position**, and
a path id identifies a call shape rather than a path. §1.3 is the rule. What
remains is mechanical but not small: **every variadic path from the first satellite
has to be split into one number per call shape**, and the accepted shapes have to
be read out of the v1 evaluator rather than guessed — `SAT_VARIADIC` recorded that
a count varied without recording which counts were legal.

`?` is the other mark in the table and means something different: a number not
fixed in advance because it is allocated when a name is first met (§3).

**`satellite.returns` has no number.** DESIGN §6.1 lists it as one of the eleven
segment-1 words with their own parse rule, and DESIGN §13 records the return-type
syntax as decided.

**The `arguments` object has no numbers, and it is the deepest thing here.**
`satellite.library.main.arguments` is a language-owned library global, not a user
name — the one thing under `satellite.library.main` that is *not* covered by §3's
dynamic rule — so it and every property it carries need frozen numbers. DESIGN §7.7
is the design. The shape, with `satellite.library.main` already at `1 14 1`:

| path | needs |
|---|---|
| `satellite.library.main.arguments` | a child number under `main` |
| `arguments.username` | a child of `arguments` |
| `arguments.memory` | a child of `arguments`, and it has children of its own |
| `arguments.memory.total` | a child of `memory` |
| `arguments.machine` | a child of `arguments`, and it has children of its own |
| `arguments.machine.cpu` | a child of `machine` |
| `arguments.machine.cores` | a child of `machine` |
| `arguments.machine.threads` | a child of `machine` |

`arguments.machine.threads` is **six numbers deep** —
`satellite`, `library`, `main`, `arguments`, `machine`, `threads` — which is the
clearest argument in the language for §1.3 refusing a segment limit. The first
satellite's macro stopped at four and could not have expressed this path at all.

`memory` and `machine` both answer bare *and* have children, so both take a `(0)`
form alongside their numbered children, the way `satellite.container` does.

**The six spellings are one node, not six.** `arg`, `args`, `argz`, `argument`,
`arguments` and `argumentz` all name the same thing and share one number; only the
spelling table knows there are six (DESIGN §4.4, §7.7).

**`satellite.thread` has no number.** The author's first note constructs a thread
with `satellite.thread.new(...)`, under a top-level `thread` namespace, while the
*type* is `satellite.variable.thread`. That is exactly the shape already numbered
for files and time — `satellite.variable.file` at `1 6 2` with `satellite.file.new`
at `1 8 1`, `satellite.variable.time` at `1 6 3` with `satellite.time.new` at
`1 9 2` — so the pattern is settled and only the word is missing.

**69 paths are still unnumbered, but they are not 69 decisions.** Almost all of
them are *the next available number*, which is a consequence and not a choice.
`SCRATCH.md/WORD_SURFACE.md` sorts them by how much judgement each actually needs:

- **9 new children of `satellite`** — `analyze`, `bool`, `directory`, `help`,
  `network`, `returns`, `system`, `thread`, `window`. **This is the only real
  decision left**, because it is an ordering, and under §1.1 the order is the order
  a program first meets them.
- **23 append under a parent that already has a number**, so each continues a list
  that is already running: `satellite.variable` has 13 waiting and its next free is
  `1 6 5`, and the three `.range` paths sit at `1 7 1 1`, `1 7 2 1` and `1 7 3 1`.
- **36 cannot be numbered until their namespace is**, and then they number from 1
  under it with nothing further to settle. `satellite.system` alone is 19 of them.

There is deliberately no `satellite.number`. A tenth candidate came from DESIGN
§5.5's aside that bit shifts *"if ever needed"* would be
`satellite.number.shift_left(n)` — the only place in either document that implied a
top-level `number`, against every other number operation living under
`satellite.variable.number`. **It was a slip and §5.5 is corrected.** *(2026-08-27.)*

Two paths that appear in the documents must **never** be numbered:
`satellite.control.return` (DESIGN §6.1) is a hypothetical showing a parse
collision, and `satellite.consle.display` (DESIGN §4.6) is a deliberate
misspelling in an error-message example.

---

*Companions: [DESIGN.md](DESIGN.md) — what the language is, and §4 for why the
numbering has this shape. [PLAN.md](PLAN.md) — how it gets built, and §8 for the
milestone that turns this file into code. [LAYOUT.md](LAYOUT.md) — every file in
the tree.*
