# The word surface — **fully numbered as of 2026-08-27**

> **This file has done its job.** Every path in it now has a number in
> [WORD_NUMBERS.md](../WORD_NUMBERS.md) §2.2, which is the authority. What is left
> below is the evidence trail — which source each path was found in — kept only
> until `words.def` is written and tested against it. **Delete it then.**


**This file is scratch.** It is raw material for [WORD_NUMBERS.md](../WORD_NUMBERS.md),
not a decision. Nothing is numbered until it is numbered *there*.

Produced 2026-08-27 by a ten-agent sweep of five independent sources — the v1
registry `format.def`, the v1 evaluator's dispatch arms, every `.satl` program in
the v1 tree, the v1 `design/` documents, and this tree's DESIGN.md and PLAN.md —
each swept once and then attacked by a second reader hunting for what the first
missed.

**112 real paths.** They are not 112 decisions. Most are *the next available
number*, which is a consequence rather than a choice, and the file is organised
by that distinction: §1 is already settled, §2 is the only place a real choice
gets made, and §3 and §4 fall out of §2 mechanically.

Two paths found by the sweep are deliberately excluded as not-real:
`satellite.consle.display` is DESIGN §4.6's misspelling example and
`satellite.control.return` is DESIGN §6.1's hypothetical parse collision.

---

## 1. Settled — 37 paths

**34 were written by hand** in WORD_NUMBERS.md §2.2 and are not repeated here.

**3 more are already fixed** without a line of their own, because a
parent's number is decided the moment any of its children has one:

| path | number | fixed by |
|---|---|---|
| `satellite.file` | `1 8 (0)` | `satellite.file.clear`, `satellite.file.new`, `satellite.file.open` |
| `satellite.random` | `1 7 (0)` | `satellite.random.fast`, `satellite.random.normal`, `satellite.random.ultra` |
| `satellite.time` | `1 9 (0)` | `satellite.time.new`, `satellite.time.now` |

These three want writing into WORD_NUMBERS.md §2.2 as bare lines, the way
`satellite.container` and `satellite.console` already have them. Nothing is being
decided by doing so — the numbers are already true.

## 2. The only real decision — 9 new children of `satellite`

Everything else in this file follows from this table. These are the namespaces
with no number and no numbered child, so each needs a slot after `return` at 15,
and **the order they go in is the one genuine choice left.** Under DESIGN §4.1
that order is the order they are first met, so the question is really: in what
order does a program meet them?

| path | what it is | seen in |
|---|---|---|
| `satellite.analyze` | v1's `satellite.analyze(path)` — one argument | registry, evaluator, programs |
| `satellite.bool` | the module constants `.true` / `.false`; **not** the type `satellite.variable.bool` | v1docs, v2docs |
| `satellite.directory` | `.current`, `.change`, `.exists`, `.list` | registry, evaluator, v1docs, v2docs |
| `satellite.help` | DESIGN §4.6 makes this a walk of the trie | registry, evaluator, v1docs, v2docs |
| `satellite.network` | v1 surface; nothing in this tree's documents promises it yet | v1docs |
| `satellite.returns` | DESIGN §6.1 lists it as a segment-1 form; §13 calls the syntax decided | evaluator, programs, v1docs, v2docs |
| `satellite.system` | the largest namespace here — `.home`, `.delete`, and all of `.memory.*` | evaluator, v2docs |
| `satellite.thread` | `satellite.thread.new(...)`, the constructor for `satellite.variable.thread` | v2docs |
| `satellite.window` | `satellite.window.new()`; DESIGN §10.3, PLAN M13 | v1docs, v2docs |

**A tenth candidate was removed.** `satellite.number` appeared exactly once, in
DESIGN §5.5's aside that bit shifts *"if ever needed"* would be
`satellite.number.shift_left(n)`, against every other number operation living under
`satellite.variable.number`. Confirmed a slip and corrected in §5.5 on 2026-08-27,
so there is no top-level `satellite.number` and there are nine here.

## 3. Appends under a parent that already has a number — 28 paths, 23 of them real

No decision beyond the order within each list, and each list continues from where
WORD_NUMBERS.md leaves it. The *next free* column says what the first of them
would take.


**`satellite.console`** — 1 to append, next free is `1 5 3`

| path | arity | seen in |
|---|---|---|
| `satellite.console.input(prompt, target)` | 2 (second is a place, not a value) | evaluator |

**`satellite.container`** — 2 to append, next free is `1 4 3`

| path | arity | seen in |
|---|---|---|
| `satellite.container.arguments` | n/a (name only; NOT declarable) | evaluator, v1docs |
| `satellite.container.result` | 0 type arguments (not generic) | evaluator |

**`satellite.container.list`** — 2 to append, next free is `1 4 2 1`

| path | arity | seen in |
|---|---|---|
| `satellite.container.list.append` | 2 (receiver, value) | v1docs, v2docs |
| `satellite.container.list.size` | 1 (the receiver) | v2docs |

**`satellite.library.main`** — 5 found by the sweep, none of them a word to number,
**and one enormous thing the sweep missed entirely**

The five below are globals that *example programs* declared, not language words:
`calls`, `greeting`, `ready` and `total` are names a user chose, and `x` is
DESIGN's placeholder for exactly that. Under WORD_NUMBERS.md §3 they take the next free
number under `satellite.library.main` **when the program that declares them is
read**, and they are gone when it ends. They are listed here only so that a later
reader does not find them in the sweep and assume they were missed.

They are also the clearest illustration of why §3 exists: five different programs
would give `calls` five different numbers, and every one of them would be right.

**What the sweep missed: `satellite.library.main.arguments`.** Ten agents over five
sources did not surface it, because in real programs it appears as a bare parameter
name — `arguments`, or `args`, or `argz` — and never as a dotted path. It is a
language-owned library global with a nested tree of properties under it
(`username`, `memory`, `memory.total`, `machine`, `machine.cpu`, `machine.cores`,
`machine.threads`), all of which need frozen numbers, and `machine.threads` is six
numbers deep. DESIGN §7.7 and WORD_NUMBERS.md §4 have it.

It is worth recording *why* it was missed rather than just that it was: **a sweep
for dotted paths cannot find a thing whose whole point is that it is written bare.**
Anything else in the language with that property is invisible to this file too.

| path | seen in |
|---|---|
| `satellite.library.main.calls` | programs |
| `satellite.library.main.greeting` | programs |
| `satellite.library.main.ready` | programs |
| `satellite.library.main.total` | programs |
| `satellite.library.main.x` | v1docs |

**`satellite.random.fast`** — 1 to append, next free is `1 7 1 1`

| path | arity | seen in |
|---|---|---|
| `satellite.random.fast.range` | 2 | registry, evaluator, programs, v1docs, v2docs |

**`satellite.random.normal`** — 1 to append, next free is `1 7 2 1`

| path | arity | seen in |
|---|---|---|
| `satellite.random.normal.range` | 2 | registry, evaluator, v1docs, v2docs |

**`satellite.random.ultra`** — 1 to append, next free is `1 7 3 1`

| path | arity | seen in |
|---|---|---|
| `satellite.random.ultra.range` | 2 | registry, evaluator, programs, v1docs, v2docs |

**`satellite.variable`** — 13 to append, next free is `1 6 5`

| path | arity | seen in |
|---|---|---|
| `satellite.variable.binary` | n/a | evaluator, programs, v1docs |
| `satellite.variable.bool` | n/a | evaluator, programs, v1docs, v2docs |
| `satellite.variable.date` | n/a — type | v2docs |
| `satellite.variable.duration` | n/a | v1docs |
| `satellite.variable.expression` | n/a | v1docs |
| `satellite.variable.float` | n/a — type | v2docs |
| `satellite.variable.hex` | n/a | evaluator, programs, v1docs |
| `satellite.variable.hexadecimal` | n/a (alias of .hex) | evaluator, programs, v1docs |
| `satellite.variable.network` | n/a | v1docs |
| `satellite.variable.thread` | unknown (type, no parameters) | programs, v1docs, v2docs |
| `satellite.variable.timemy_time` | — | v2docs |
| `satellite.variable.variant` | n/a — type | v2docs |
| `satellite.variable.window` | n/a | v1docs |

**`satellite.variable.file`** — 2 to append, next free is `1 6 2 1`

| path | arity | seen in |
|---|---|---|
| `satellite.variable.file.new` | unknown (receiver-binding tag required) | v2docs |
| `satellite.variable.file.open` | 1 | registry, v1docs |

## 4. Waiting on §2 — 36 paths

These cannot be numbered until their namespace has a number, and then they number
from 1 under it with no further decision. Counted by namespace so the size of each
is visible before its slot is chosen.

| namespace | children waiting |
|---|---|
| `satellite.bool` | 2 |
| `satellite.directory` | 4 |
| `satellite.library` | 3 |
| `satellite.network` | 5 |
| `satellite.number` | 1 |
| `satellite.system` | 19 |
| `satellite.thread` | 1 |
| `satellite.window` | 1 |

**`satellite.bool`** — 2

| path | arity | seen in |
|---|---|---|
| `satellite.bool.false` | 0 | registry, evaluator, programs, v1docs, v2docs |
| `satellite.bool.true` | 0 | registry, evaluator, programs, v1docs, v2docs |

**`satellite.directory`** — 4

| path | arity | seen in |
|---|---|---|
| `satellite.directory.change` | 1 | registry, evaluator, v1docs |
| `satellite.directory.current` | 0 | registry, evaluator, programs, v1docs |
| `satellite.directory.exists` | 1 | registry, evaluator, programs, v1docs |
| `satellite.directory.list` | VARIADIC | registry, evaluator, programs, v2docs |

**`satellite.library`** — 3

| path | arity | seen in |
|---|---|---|
| `satellite.library.system.division_digits` | n/a (a knob the evaluator reads) | evaluator, v1docs |
| `satellite.library.system.max_depth` | n/a (a knob the evaluator reads) | evaluator, v1docs |
| `satellite.library.system.min_free_mb` | n/a (a variable slot) | evaluator, v1docs |

**`satellite.network`** — 5

| path | arity | seen in |
|---|---|---|
| `satellite.network.http` | 1 (server: port) or 2 (client: host, port) | v1docs |
| `satellite.network.https` | 2 (client: host, port) or 3 (server: port, cert, key) | v1docs |
| `satellite.network.new` | 1 (port) | v1docs |
| `satellite.network.open` | 1 (port) | v1docs |
| `satellite.network.receive` | 1 (port) | v1docs |

**`satellite.number`** — 1

| path | arity | seen in |
|---|---|---|
| `satellite.number.shift_left` | 1 | v1docs, v2docs |

**`satellite.system`** — 19

| path | arity | seen in |
|---|---|---|
| `satellite.system.delete` | 1 | registry, evaluator, programs |
| `satellite.system.environment` | 1 | v1docs |
| `satellite.system.home` | 0 | registry, evaluator, programs |
| `satellite.system.memory` | unknown | registry |
| `satellite.system.memory.bit` | 0 | registry, evaluator |
| `satellite.system.memory.free` | VARIADIC | registry, evaluator, programs |
| `satellite.system.memory.frequency` | 0 | registry, evaluator |
| `satellite.system.memory.main` | VARIADIC | registry, evaluator, programs |
| `satellite.system.memory.swap` | VARIADIC | registry, evaluator |
| `satellite.system.memory.swap.free` | 0 or 1 (unit string) | evaluator |
| `satellite.system.memory.swap.total` | 0 or 1 (unit string) | evaluator |
| `satellite.system.memory.swap.used` | 0 | registry, evaluator |
| `satellite.system.memory.this` | unknown | registry |
| `satellite.system.memory.this.available` | 0 or 1 (unit string) | evaluator |
| `satellite.system.memory.this.free` | 0 or 1 (unit string) | evaluator |
| `satellite.system.memory.this.used` | unknown | registry, evaluator, programs |
| `satellite.system.memory.total` | VARIADIC | registry, evaluator, programs |
| `satellite.system.memory.used` | VARIADIC | registry, evaluator |
| `satellite.system.threshold` | 0 (read) or 1 (set) | evaluator, programs, v2docs |

**`satellite.thread`** — 1

| path | arity | seen in |
|---|---|---|
| `satellite.thread.new` | 1 (an unevaluated call expression as the thread body) | programs, v1docs, v2docs |

**`satellite.window`** — 1

| path | arity | seen in |
|---|---|---|
| `satellite.window.new` | unknown — written `satellite.window.new(...)` with elided arguments | v2docs |
