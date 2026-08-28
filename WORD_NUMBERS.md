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

`include` is met first, so it is 1. `capsule` is next, so it is 2. Then `main`, then
`container`. Nothing about that order claims `include` is more important than
`variable` — only that a reader meets it sooner.

**The last two of the six do not fall out of a strict walk of this program, and
saying so costs no number.** *(Corrected 2026-08-28.)* `console` is 5 and `variable`
is 6, but `satellite.variable.string` sits inside the parameter's type on line 3,
where a reader meets it **before** line 5 reaches `satellite.console.display`. Read
strictly, this program gives `variable` 5 and `console` 6.

**§2.1 already says what actually happened — *1 to 15 were written by hand*** — and
this same program carries the plainest proof of it: `satellite.return` is on its last
line and is **15**, not 7. The walk was never run to the end of it.

So the walk is **why the order is this one and not another**, and it is the rule that
decides every number from 16 on, where §2.1 applies it strictly and says so. It is
not a procedure that regenerates 1 through 15. Where the two disagree, §1.2 settles
which gives way: **the numbers are frozen, so a sentence about them is the only thing
here that can be wrong.**

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

**`(0)` in §2.2 is that same `0`, and nothing else.** *(Defined 2026-08-28; it was
used on forty-odd rows and defined nowhere.)* A row written `satellite.container`
`1 4 (0)` says the node is `1 4` and its **zero-argument call shape is `1 4 0`** —
exactly what `satellite.include()` `1 1 0` says without the parentheses. The
brackets are a reading aid marking a node that is reached both bare and as a
parent; they are not a third kind of thing, and there is no second rule to learn.

**A trailing `0` is written only where a program can actually write the bare
form.** `satellite.variable.binary` `1 6 5` has no `(0)` because nothing calls it
with no arguments — it is a type name, and the number is the whole of it.

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

Three 64-bit integers would be 24 bytes to say what 4 bytes says, and **depth is
not what the integer is holding**, so no path is a special case that needs a wider
one. The deepest in §2.2 is six segments —
`satellite.library.main.arguments.machine.cores` `1 14 1 1 1 1` — and it interns to
the same four bytes as `satellite.main` `1 3`.

*(Corrected 2026-08-28. This paragraph used to cite `satellite.random.fast.range`
as "four numbers and nothing else". It is not four numbers: §2.3 makes it an
**alias at `1 7 5`**, three numbers, sharing with the call shape above it. Both
statements were written in the same commit and disagreed from the start.)*

**So the path is NOT padded to a fixed number of segments, and that was decided
rather than left.** *(2026-08-28.)* Fixing every path at six — today's maximum —
was considered and declined for three reasons, in the order they bite:

- **It buys the runtime nothing.** The `uint32_t` above is an interned id, an index
  into a node table. It is not six segments packed into 32 bits, so a fixed segment
  count constrains nothing it does.
- **§3 makes fixed-width packing impossible anyway.** The widest child list in the
  language is `satellite.container.list` `1 4 2` with 25 children, which needs 5
  bits, and 6 × 5 = 30 fits in 32 with two to spare. Then a user's capsules take
  the next free number under their parent, **allocated at parse time and unbounded**
  — forty capsules need 6 bits and a thousand need 10. Fixed-width packing and §3
  cannot both be true.
- **Six is today's maximum, not a bound.** `satellite.system.memory.swap.free(unit)`
  `1 22 4 4 4` is already five, and a seventh level is one namespace away. Freezing
  a depth that would have to be broken is worse than not freezing one.

### 1.5 Paths are numbered in the file; selectors are numbered for dispatch

Two different things carry numbers, and confusing them is how a `.satc` ends up
naming a handler (SATC.md §7 forbids exactly that).

A **path** is rooted at `satellite` and resolves with no context. Wherever
`satellite.console.display` appears it is `1 5 1`, so a text substitution is sound
and that is what a `.satc` writes.

A **selector** is a bare word after a receiver — `sort` in `my_list.sort()`. It is
language-owned and it has a number, but the number is only reachable *through the
receiver's type*, and the receiver's type is not known until resolve runs. DESIGN
§6.3 keeps the parser resolution-free and PLAN M4.5 writes the `.satc` from the
parse tree, so at the moment the file is written the selector's identity is
**unknowable**. It stays a bare word in the file.

| | written in source | in a `.satc` | numbered for |
|---|---|---|---|
| **path** — `satellite.console.display`, `satellite.thread.new` | rooted at `satellite` | becomes a number | the file *and* dispatch |
| **selector** — `sort`, `append`, `get`, `substring` | bare, after a receiver | stays bare | dispatch only |

Most of §2.2 is the second kind. That costs the file nothing and buys the runtime
everything: DESIGN §6.4's method sugar resolves once to a `PathId`, and every
execution after that is `handlers[path_id]` — one array index. The number is what
`my_list.sort()` *becomes*, never another way to spell it.

#### A literal option folds into the number

DESIGN §1.1 requires an option to be a word at the call site rather than a bitmask:
`satellite.file.open("filename", "read_append")`. That word is a string, and testing
it at runtime is the cost of the readability.

It need not be. When the option is a **literal**, the parser already knows it, so
`my_list.sort("down")` can intern to a different `PathId` than `my_list.sort("up")`
— same readable surface, no runtime test, one more array index. `sort_down` at
`1 4 2 5` is that fold; `sort(direction)` at `1 4 2 4` is the general form kept for
when the option is a variable, where §2.4's inline cache takes over on the second
execution.

**The fold is a resolve-time decision and must never reach a `.satc`**, because
SATC.md §3 says literals stay literal. The file keeps `"down"`; the runtime keeps
the number. This is the one place where the numbering deliberately says more than
the file does.

---

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
| `satellite.container.map.set(k, v)` | `1 4 1 1` | assigned |
| `satellite.container.map.get(k)` | `1 4 1 2` | assigned |
| `satellite.container.map.has(k)` | `1 4 1 3` | assigned |
| `satellite.container.map.size` | `1 4 1 4` | assigned |
| `satellite.container.map.empty` | `1 4 1 5` | assigned |
| `satellite.container.map.clear` | `1 4 1 6` | assigned |
| `satellite.container.map.remove(k)` | `1 4 1 7` | assigned |
| `satellite.container.map.keys` | `1 4 1 8` | assigned |
| `satellite.container.map.values` | `1 4 1 9` | assigned |
| `satellite.container.list` | `1 4 2 (0)` |  |
| `satellite.container.list.append` | `1 4 2 1` | assigned |
| `satellite.container.list.size` | `1 4 2 2` | assigned |
| `satellite.container.list.sort()` | `1 4 2 3` | assigned — ascending, no key |
| `satellite.container.list.sort(direction)` | `1 4 2 4` | assigned — direction not a literal |
| `satellite.container.list.sort_down()` | `1 4 2 5` | assigned — folded from `sort("down")`, §1.5 |
| `satellite.container.list.sort_down(key)` | `1 4 2 6` | assigned |
| `satellite.container.list.sort_up(key)` | `1 4 2 7` | assigned |
| `satellite.container.list.contains(x)` | `1 4 2 8` | assigned |
| `satellite.container.list.index_of(x)` | `1 4 2 9` | assigned |
| `satellite.container.list.empty` | `1 4 2 10` | assigned |
| `satellite.container.list.clear` | `1 4 2 11` | assigned |
| `satellite.container.list.first` | `1 4 2 12` | assigned |
| `satellite.container.list.last` | `1 4 2 13` | assigned |
| `satellite.container.list.truncate(n)` | `1 4 2 14` | assigned |
| `satellite.container.list.reserve(n)` | `1 4 2 15` | assigned |
| `satellite.container.list.remove_first()` | `1 4 2 16` | assigned — the cheap front |
| `satellite.container.list.remove_last()` | `1 4 2 17` | assigned |
| `satellite.container.list.remove_at(n)` | `1 4 2 18` | assigned |
| `satellite.container.list.remove(x)` | `1 4 2 19` | assigned — by value |
| `satellite.container.list.insert(n, x)` | `1 4 2 20` | assigned |
| `satellite.container.list.join(separator)` | `1 4 2 21` | assigned |
| `satellite.container.list.reverse` | `1 4 2 22` | assigned |
| `satellite.container.list.sum` | `1 4 2 23` | assigned |
| `satellite.container.list.max` | `1 4 2 24` | assigned |
| `satellite.container.list.min` | `1 4 2 25` | assigned |
| `satellite.container.arguments` | `1 4 3` | assigned — the type of the arguments object |
| `satellite.container.result` | `1 4 4` | assigned — Satellite Orbit's answer |
| `satellite.console` | `1 5 (0)` |  |
| `satellite.console.display` | `1 5 1` |  |
| `satellite.console.input()` | `1 5 2` |  |
| `satellite.console.input(prompt)` | `1 5 3` | assigned |
| `satellite.console.input(prompt, target)` | `1 5 4` | assigned — a place, not a value |
| `satellite.console.typed()` | `1 5 5` | assigned — non-blocking; a line or nothing |
| `satellite.console.width` | `1 5 6` | assigned |
| `satellite.console.height` | `1 5 7` | assigned |
| `satellite.console.clear()` | `1 5 8` | assigned |
| `satellite.console.home()` | `1 5 9` | assigned |
| `satellite.variable` | `1 6 (0)` |  |
| `satellite.variable.string` | `1 6 1 (0)` |  |
| `satellite.variable.string.size` | `1 6 1 1` | assigned |
| `satellite.variable.string.empty` | `1 6 1 2` | assigned |
| `satellite.variable.string.find(x)` | `1 6 1 3` | assigned |
| `satellite.variable.string.contains(x)` | `1 6 1 4` | assigned |
| `satellite.variable.string.substring(start, end)` | `1 6 1 5` | assigned |
| `satellite.variable.string.starts_with(x)` | `1 6 1 6` | assigned |
| `satellite.variable.string.ends_with(x)` | `1 6 1 7` | assigned |
| `satellite.variable.string.lower` | `1 6 1 8` | assigned |
| `satellite.variable.string.upper` | `1 6 1 9` | assigned |
| `satellite.variable.string.split(separator)` | `1 6 1 10` | assigned |
| `satellite.variable.string.trim` | `1 6 1 11` | assigned |
| `satellite.variable.string.replace(a, b)` | `1 6 1 12` | assigned |
| `satellite.variable.string.to_number` | `1 6 1 13` | assigned |
| `satellite.variable.string.append(x)` | `1 6 1 14` | assigned |
| `satellite.variable.string.clear` | `1 6 1 15` | assigned |
| `satellite.variable.string.at(n)` | `1 6 1 16` | assigned |
| `satellite.variable.file` | `1 6 2 (0)` |  |
| `satellite.variable.file.new` | `1 6 2 1` | assigned |
| `satellite.variable.file.open` | `1 6 2 2` | assigned |
| `satellite.variable.file.read_line` | `1 6 2 3` | assigned — one line, or nothing at end |
| `satellite.variable.file.write_line(s)` | `1 6 2 4` | assigned |
| `satellite.variable.file.read_all` | `1 6 2 5` | assigned |
| `satellite.variable.file.close` | `1 6 2 6` | assigned |
| `satellite.variable.file.exists` | `1 6 2 7` | assigned |
| `satellite.variable.time` | `1 6 3 (0)` |  |
| `satellite.variable.number` | `1 6 4 (0)` |  |
| `satellite.variable.number.shift_left` | `1 6 4 1` | assigned — relocated from the corrected §5.5 |
| `satellite.variable.number.max(a, b)` | `1 6 4 2` | assigned |
| `satellite.variable.number.min(a, b)` | `1 6 4 3` | assigned |
| `satellite.variable.number.abs(a)` | `1 6 4 4` | assigned |
| `satellite.variable.number.clamp(a, low, high)` | `1 6 4 5` | assigned |
| `satellite.variable.number.to_string` | `1 6 4 6` | assigned |
| `satellite.variable.number.floor` | `1 6 4 7` | assigned |
| `satellite.variable.number.ceil` | `1 6 4 8` | assigned |
| `satellite.variable.number.round` | `1 6 4 9` | assigned |
| `satellite.variable.number.power(a, b)` | `1 6 4 10` | assigned |
| `satellite.variable.number.shift_right(n)` | `1 6 4 11` | assigned |
| `satellite.variable.number.modulus(a, b)` | `1 6 4 12` | assigned — exact; DESIGN §8.6 |
| `satellite.variable.number.truncate(a)` | `1 6 4 13` | assigned — on a float this is just its left half |
| `satellite.variable.number.sqrt(a)` | `1 6 4 14` | assigned — irrational in general, so it rounds |
| `satellite.variable.binary` | `1 6 5` | assigned |
| `satellite.variable.bool` | `1 6 6` | assigned |
| `satellite.variable.date` | `1 6 7` | assigned |
| `satellite.variable.duration` | `1 6 8` | assigned |
| `satellite.variable.expression` | `1 6 9` | assigned |
| `satellite.variable.float` | `1 6 10` | assigned |
| `satellite.variable.hex` | `1 6 11` | assigned; also spelled `hexadecimal` |
| `satellite.variable.network` | `1 6 12` | assigned |
| `satellite.variable.thread` | `1 6 13 (0)` | assigned — the `(0)` is new with its children below |
| `satellite.variable.thread.start()` | `1 6 13 1` | assigned 2026-08-28 |
| `satellite.variable.thread.join()` | `1 6 13 2` | assigned 2026-08-28 |
| `satellite.variable.variant` | `1 6 14` | assigned |
| `satellite.variable.window` | `1 6 15` | assigned |
| `satellite.variable.capsule` | `1 6 16` | assigned — the type of a deferred call; `satellite.capsule` `1 2` is the keyword |
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
| `satellite.time.sleep(n)` | `1 9 3` | assigned |
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
| `satellite.library.system.float_digits` | `1 14 2 4` | assigned — the precision dial, DESIGN §13 |
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
| `satellite.window.console` | `1 24 2 (0)` | assigned 2026-08-28 |
| `satellite.window.console.new(title, width, height)` | `1 24 2 1` | assigned 2026-08-28 |

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

### 2.4 Four numbers assigned 2026-08-28, and by whom


**The table below repeats numbers that already appear in §2.2 and is NOT a source.**
§2.2 is the only place a number is declared; anything counting or transcribing the
numbering must read that section and stop at its end. This note sat *inside* §2.2
for one commit on 2026-08-28 and a mechanical count read its four paths twice —
226 rows instead of 222 — which is exactly the failure §2.2 exists to prevent.

**The author delegated these four and only these four.** Everything else in §2.2 is
theirs. They are recorded here because a number assigned by somebody else, once, is
exactly the kind of fact that becomes invisible a month later.

| path | number | what decided it |
|---|---|---|
| `satellite.variable.thread.start()` | `1 6 13 1` | §1.1 — `example/thread_test.satl` writes `my_thread.start()` before `my_thread.join()`, and `1 6 13` had no children, so start is first |
| `satellite.variable.thread.join()` | `1 6 13 2` | the same walk, one line later |
| `satellite.window.console` | `1 24 2 (0)` | §4.2 — numbering is per-parent, and `satellite.window` `1 24` already had `new` at 1, so the next free child is 2 |
| `satellite.window.console.new(title, width, height)` | `1 24 2 1` | §1.3 — a string and two numbers are all user-owned, so none extends the path and the three of them are one shape, the first child of `console` |

**`console` is spelled twice in the language and is two different nodes.**
`satellite.console` is `1 5`; this one is `1 24 2`. DESIGN §4.4 is the case exactly —
*"`list` under `container` and `list` under `directory` are different nodes that
happen to be spelled alike"* — and the spelling table holds the dedup, not the trie.

**`1 6 13` gained a `(0)` and did not change number.** Every other node in §2.2 with
children carries the marker — `1 4 1 (0)`, `1 6 1 (0)`, `1 22 4 (0)`, `1 24 (0)` — and
`satellite.variable.thread` had none because it had no children. **This is a notation
edit, not a renumbering**, and §1.2's freeze is untouched: nothing moved, two things
were appended.

**What §1.3 does not define.** Bare `0` is defined above — *"`0` means nothing in that
position, a real number in the sequence."* Parenthesised `(0)` is used on forty-odd
rows and is defined nowhere. The two were kept consistent here by copying what the
table already does; **the sentence that says what `(0)` means is still unwritten**,
and it is the author's.

**§2.2 is now 222 rows and 219 distinct numbers.** Counted mechanically after the
edit; the only duplicates are still §2.3's three aliases below.

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

*Last reconciled against §2.2 on 2026-08-27, when the table went from 144 entries
to 215.*

**Four things this section used to hold open are now in §2.2 and are struck from
the list**: `satellite.returns` (`1 21`), `satellite.thread` (`1 23`), the whole
`arguments` object down to `arguments.machine.threads` at `1 14 1 1 1 3` — six
numbers deep, and still the clearest argument in the language for §1.3 refusing a
segment limit — and the 69-path backlog, which is empty. Nothing found by the
sweep is unnumbered.

### What is actually still open

**The variadic split is mechanical but not small.** Every variadic path from the
first satellite has to become one number per call shape (§1.3), and the accepted
shapes have to be read out of the v1 evaluator rather than guessed —
`SAT_VARIADIC` recorded that a count varied without recording which counts were
legal.

**Call shapes sit at two different depths, and `words.def` can only encode one.**
`include()` and `include(satellite)` are *children of* `include` at `1 1 0` and
`1 1 1`; `input()` and `input(prompt)` are *siblings of* `display` under `console`
at `1 5 2` and `1 5 3`. Both were written by hand and both were confirmed, so the
table is faithful rather than uniform.

There is a rule that fits both and it is worth checking before either is changed:
a **language-owned** argument extends the path downward, because it has a number of
its own to contribute (`satellite` is word 1, so `include(satellite)` is `1 1 1`);
a **user-owned** argument cannot extend anything, so its call shape takes a slot
beside its siblings. `include` has a bare number *and* a `0` child because it is
the one word with both kinds of shape. If that reading holds, the inconsistency is
apparent rather than real and the two depths are two different rules doing their
jobs. **Confirm before `words.def` encodes it.**

**`satellite.file` `1 8` and `satellite.variable.file` `1 6 2` both carry a `new`.**
The type node's children are where DESIGN §6.4 dispatches a method — which is why
the five file methods added on 2026-08-27 went to `1 6 2 3` through `1 6 2 7` —
while `satellite.file.new(path)` at `1 8 1` is the module form. §6.4's own second
qualification says constructors live in the same table with a flag for whether the
first parameter binds the receiver, so the two can coexist. But nothing yet says
which one a program should write, and two spellings for one construction is the
kind of thing that gets decided by accident at M10.

**`satellite.thread.new` has one number for two shapes.** `thread.new(f())` and
`thread.new(f(x))` both hand `new` exactly one thing — a deferred call — so its own
arity is always 1, and the capsule's arity rides on the capsule's own number under
§3. Splitting on the capsule's arity does not stop at two: `f(a,b)` and `f(a,b,c)`
are equally distinct and the split becomes unbounded. `1 23 2` is free if this is
decided the other way.

**`satellite.container.set` is deliberately absent** and `1 4 5` is free. A set is a
map with no values, and the map now has its own children at `1 4 1 1` onward. If a
set earns its own type it takes `1 4 5`; until something needs one it is a word the
language does not have.

### Two things that must never be numbered

`satellite.control.return` (DESIGN §6.1) is a hypothetical showing a parse
collision, and `satellite.consle.display` (DESIGN §4.6) is a deliberate misspelling
in an error-message example.

And there is deliberately no `satellite.number`. A candidate came from DESIGN §5.5's
aside that bit shifts *"if ever needed"* would be `satellite.number.shift_left(n)` —
the only place in either document that implied a top-level `number`, against every
other number operation living under `satellite.variable.number`, which now holds ten
more of them at `1 6 4 2` through `1 6 4 11`. **It was a slip and §5.5 is
corrected.** *(2026-08-27.)*

---

*Companions: [DESIGN.md](DESIGN.md) — what the language is, and §4 for why the
numbering has this shape. [PLAN.md](PLAN.md) — how it gets built, and §8 for the
milestone that turns this file into code. [LAYOUT.md](LAYOUT.md) — every file in
the tree.*
