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
form.**

***This paragraph used `satellite.variable.binary` `1 6 5` as its example —
"no `(0)` because nothing calls it with no arguments, it is a type name, and
the number is the whole of it" — and M19.5 gave that node four children and a
`(0)`, on 2026-09-08.*** **The rule did not change and the example was wrong
about the reason.** A type name's `(0)` is not a call with no arguments; it is
the BARE SHAPE — `satellite.variable.binary bits` with no `= b1010` after it,
a declared name holding nothing — and every other type node in §2.2 already
carried one on exactly that ground: `string` `1 6 1 0`, `file` `1 6 2 0`,
`time` `1 6 3 0`, `number` `1 6 4 0`, `thread` `1 6 13 0`, `variant`
`1 6 14 0`. Binary had none because it had no children to hang them under, not
because it was a different kind of word.

**So the rule reads: a trailing `0` is written where a program can write the
node with nothing after it**, which for a verb is a call with no arguments and
for a type name is a declaration with no value. `1 6 13 (0)` and `1 6 14 (0)`
are annotated "the `(0)` is new with its children below" for this reason and
`1 6 5 (0)` now says the same thing.

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
and that is what a `.satc` writes. *(How the file spells it is SATC.md's and not
this document's: `#1.5.1`, the segments closed up with dots and a `#` in front,
because `1 5` closes up to `1.5` and so does the float one-and-a-half. SATC
§1.1.1.)*

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

#### The one hop is one hop, and `f().g()` is the cost of that

*(Named 2026-09-08, at M19. Not a new rule — the consequence of the rule above,
written down because three milestones have now paid it and none of them said
so.)*

**A selector reaches its number through the receiver's DECLARED TYPE, so the
receiver has to be a declared name.** `name_resolver/names.cpp` folds a selector
only through one — WORD_NUMBERS' one hop — so a method asked of a **call's
answer** has no declared type to fold through and the compiler refuses it:

    satellite.console.display(f.read_all().size())
                                          ^^^^
    error S0720: a method on this expression parses and does not run yet --
                 a selector folds only through a declared name -- WORD_NUMBERS.md
                 §1.5's one hop -- so name the receiver first

**The fix a program can make today is a name in between**, and it is one line:

```satellite
satellite.variable.string whole = f.read_all()
satellite.console.display(whole.size())
```

**NO MILESTONE OWNS LOOSENING IT**, and that is the reason this paragraph
exists rather than a note in one milestone's review. `evaluator/compile_expressions.cpp`
says the case *"waits for the type rules that would see through it"*, and PLAN §8
has no entry that promises them. Until one does, **every worked line in
`HELP.md` and every fixture in `tests/` has to be written with the hop in mind**
— M13 hit this from the other side (MILESTONES/M13.md §3.1), M16 built the
containers whose methods invite the chain, and M19 was the first milestone to
write enough of them to trip over it repeatedly: three help examples and five
test fixtures were written the natural way, refused, and had to be unwound.

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

***THE FOLD REACHES A `.satc` SINCE M19.6, AND WHAT WAS WRONG WITH THE SENTENCE
BELOW WAS THE WORD "never".*** The file now writes `my_list.sort(0#down)` — an
**option token**, which SATC.md §3 lists as a third kind of thing beside a number
and a literal. So "literals stay literal" is untouched, and this is no longer a
place where the numbering says more than the file does.

> **What stood here until 2026-09-09:** *"The fold is a resolve-time decision and
> must never reach a `.satc`, because SATC.md §3 says literals stay literal. The
> file keeps `"down"`; the runtime keeps the number. This is the one place where
> the numbering deliberately says more than the file does."*

**IT WAS RIGHT ABOUT WHY AND WRONG ABOUT WHETHER.** A fold IS a resolve-time
decision — that never stopped being true — and the reason the file could not
carry it was that the writer ran before resolve, not that §3 forbade it. M19.6
moved the writer. The token is what the move made writable, and the sentence to
keep is the narrower one: **a `.satc` records which option was WRITTEN and never
which row it lands on**, so `1 4 2 5` is still reached by asking the numbering
under the receiver's type. What the file saves is the DECISION — is `down` an
option at all, which options exist, does the folded shape exist, should it fall
back to the bare word — and not the lookup.

**BUILT AT M7, 2026-08-31, AND IT READS THIS TABLE RATHER THAN A LIST OF SPECIAL
CASES.** A word takes options when the rows beside it are spelled
`<word>_<something>` — `sort` has `sort_down` and `sort_up`, so a **literal**
first argument to it names one of them, and `contains` has no such sibling so
`contains("ok")` is an ordinary call with a string in it. PLAN's M7 entry says
*"M19 is waiting on this: whether `satellite.file.open`'s four mode words fold
decides whether its bad-mode message is M5's suggester or a runtime check"*, and
the answer is **the suggester, with no edit to any source file** on the day
`open_read_append` is written into `words.def`.

**AND THIS TABLE HAS NO `sort_up()`, WHICH BUILDING THE FOLD FOUND.** §2.2
assigns `sort()` `1 4 2 3` as *"ascending, no key"* and `sort_up(key)` `1 4 2 7`,
so `my_list.sort("up")` names an option that exists at a shape that does not, and
M7 answers it with **S0523** naming `sort_up(key)`. That is accurate and it is
not what somebody wanted. Two fixes exist and both are §1.2's freeze applied to a
decision only the author can take: an **alias** row spelling `sort_up()` onto
`1 4 2 3`, or a rule that a fold may land on the bare word it was spelled from.
MILESTONES/M7.md §6 item 1.

**DECIDED 2026-09-05, AT M16, BY THE AUTHOR: a fold may land on the bare word
it was spelled from.** When the option is legal — some sibling spells
`<word>_<option>` — and the folded shape does not exist at the written count,
the fold retries the BARE word at that count before refusing, so
`my_list.sort("up")` is `sort()` `1 4 2 3` and no alias row was minted. The
rule is general and the generality is the risk worth writing down: it is only
correct while every `<word>_<option>` row whose shape is missing MEANS the bare
word — true of `sort_up`, whose §2.2 note says `sort()` already is it — and a
future word where the option changes the meaning must get its shape row rather
than lean on this fallback. src/name_resolver/numbers.cpp is where the rule
runs; MILESTONES/M16.md §2 carries the decision.

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
| `satellite.container.map.search(pattern)` | `1 4 1 10` | assigned — appended 2026-09-05, §2.6 |
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
| `satellite.container.list.search(pattern)` | `1 4 2 26` | assigned — appended 2026-09-05, §2.6 |
| `satellite.container.arguments` | `1 4 3` | assigned — the type of the arguments object |
| `satellite.container.arguments.length` | `1 4 3 1` | assigned — how many words were on the command line, argv[0] included; the same number as `1 14 1 1 9`, from the same vector, reached through the receiver's type instead of off the object — the command line half and nothing else, so `for (i = 1; i < args.length(); i = i + 1)` walks what the user typed. M20 |
| `satellite.container.arguments.count` | `1 4 3 2` | assigned — how many entries the object holds, the command line and the machine's facts together. `1 14 1 1 9` is `length` and counts the command line; the author split the two words on 2026-09-11 rather than let one shadow the other. M20 |
| `satellite.container.arguments.keys` | `1 4 3 3` | assigned — every entry's name, in display order, as a `list<string>` — the map's word for the same idea, chosen by the author over `names` 2026-09-11. M20 |
| `satellite.container.arguments.to_string` | `1 4 3 4` | assigned — the whole object as the text `display` prints — names beside data, not the command line alone. M20 |
| `satellite.container.arguments.lines` | `1 4 3 5` | assigned — the same text as `1 4 3 4`; two words for one answer, v1's pair kept. M20 |
| `satellite.container.arguments.has(k)` | `1 4 3 6` | assigned — whether an entry of that name exists — the question `get` refuses. M20 |
| `satellite.container.arguments.get(k)` | `1 4 3 7` | assigned — the entry of that name, or a refusal naming it. M20 |
| `satellite.container.arguments.first` | `1 4 3 8` | assigned — the first word of the command line, which is the program. M20 |
| `satellite.container.arguments.last` | `1 4 3 9` | assigned — the last word of the command line. M20 |
| `satellite.container.arguments.contains(x)` | `1 4 3 10` | assigned — whether the command line holds that word. M20 |
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
| `satellite.variable.string.resolved` | `1 6 1 17` | assigned — the string with its live codes answered by the machine; the one method that crosses §5's render/store boundary, minted at M20 so the six live values can be read as well as printed |
| `satellite.variable.string(x)` | `1 6 1 18` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the bare spelling, written around a value |
| `satellite.variable.string.string` | `1 6 1 19` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — a string's own text, so the four names exist on every kind |
| `satellite.variable.string.number` | `1 6 1 20` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_number` `1 6 1 13` |
| `satellite.variable.string.binary` | `1 6 1 21` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the digits when the text is all `0` and `1`, otherwise the bytes |
| `satellite.variable.string.hex` | `1 6 1 22` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same bits as `.binary()`, four to a digit |
| `satellite.variable.file` | `1 6 2 (0)` |  |
| `satellite.variable.file.new` | `1 6 2 1` | assigned — the dispatch row §6.4 q2 describes; a program writes `satellite.file.new(path)` `1 8 1`. M19 |
| `satellite.variable.file.open` | `1 6 2 2` | assigned — reopens a handle that was closed or whose open failed; NOT a second `1 8 2`. M19 |
| `satellite.variable.file.read_line` | `1 6 2 3` | assigned — one line, or nothing at end; advances a per-handle cursor (DESIGN §8.7). M19 |
| `satellite.variable.file.write_line(s)` | `1 6 2 4` | assigned — writes `s` AND a newline; `write(x)` `1 6 2 11` is the byte-exact verb. M19 |
| `satellite.variable.file.read_all` | `1 6 2 5` | assigned — the whole file from the beginning; leaves the cursor at the end. M19 |
| `satellite.variable.file.close` | `1 6 2 6` | assigned |
| `satellite.variable.file.exists` | `1 6 2 7` | assigned — is THIS handle's own path still there; `satellite.file.exists(path)` `1 8 5` is the question asked without one. M19 |
| `satellite.variable.file.ok` | `1 6 2 8` | **assigned 2026-09-08** — did the open work. DESIGN §9's failed open is a VALUE, and this is the word that asks it. M19 |
| `satellite.variable.file.path` | `1 6 2 9` | **assigned 2026-09-08** — the path the handle was opened on. M19 |
| `satellite.variable.file.error` | `1 6 2 10` | **assigned 2026-09-08** — why the last call failed, in plain words, or `""`. M19 |
| `satellite.variable.file.write(x)` | `1 6 2 11` | **assigned 2026-09-08** — exactly the bytes of `x` and no newline. The verb M19.5's `binary` and `hex` values are written with. M19 |
| `satellite.variable.time` | `1 6 3 (0)` |  |
| `satellite.variable.number` | `1 6 4 (0)` |  |
| `satellite.variable.number.shift_left(n)` | `1 6 4 1` | assigned — relocated from the corrected §5.5; `(n)` written on 2026-08-31, matching §5.5 and its `shift_right` twin |
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
| `satellite.variable.number.digits` | `1 6 4 15` | appended 2026-08-31 — v1 exposes `.digits()` and this section had no row for it |
| `satellite.variable.number(x)` | `1 6 4 16` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the bare spelling |
| `satellite.variable.number.string` | `1 6 4 17` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_string` `1 6 4 6` |
| `satellite.variable.number.number` | `1 6 4 18` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — a number's own value |
| `satellite.variable.number.binary` | `1 6 4 19` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the bits, shortest first; a negative or fractional value is S0733 |
| `satellite.variable.number.hex` | `1 6 4 20` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same value, padded left to a whole digit |
| `satellite.variable.binary` | `1 6 5 (0)` | assigned; **built at M19.5** — the `(0)` is new with its children below |
| `satellite.variable.binary.to_number` | `1 6 5 1` | assigned 2026-09-08 — what the bits are WORTH; `b1010.to_number()` is 10 |
| `satellite.variable.binary.width` | `1 6 5 2` | assigned 2026-09-08 — how many bits were written; the one row that reads DESIGN §8.5's width |
| `satellite.variable.binary.to_string` | `1 6 5 3` | assigned 2026-09-08 — the characters `display` prints, the leading `b` included |
| `satellite.variable.binary.as_number` | `1 6 5 4` | assigned 2026-09-08 — the digits read as DECIMAL; `b1010.as_number()` is 1010 |
| `satellite.variable.binary.digits` | `1 6 5 5` | assigned 2026-09-09 — how many digits were written; always `width()` here, and not on hex |
| `satellite.variable.binary.to_hex` | `1 6 5 6` | assigned 2026-09-09 — the same bits as hex; REFUSES a width that is not a multiple of 4 |
| `satellite.variable.binary(x)` | `1 6 5 7` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the bare spelling |
| `satellite.variable.binary.string` | `1 6 5 8` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_string` `1 6 5 3` |
| `satellite.variable.binary.number` | `1 6 5 9` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_number` `1 6 5 1` |
| `satellite.variable.binary.binary` | `1 6 5 10` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the run itself, width and all |
| `satellite.variable.binary.hex` | `1 6 5 11` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_hex` `1 6 5 6` |
| `satellite.variable.binary.clear` | `1 6 5 12` | assigned 2026-09-12 — the author's, mid-M26, and the reason was that the words should line up: *"The words are supposed to all do the same thing."* It is `satellite.variable.string.clear` `1 6 1 15` on the other type, down to the write-back contract — **binary's first mutating row**, so the install in `bits_methods.cpp` grew a `mutates` column that its own note said it would never need. What it answers is a run of width 0, which is the empty value of the type the way `""` is the string's; §8.5 makes the width part of the value, so there is no other reading of empty available |
| `satellite.variable.bool` | `1 6 6` | assigned |
| `satellite.variable.date` | `1 6 7` | assigned |
| `satellite.variable.duration` | `1 6 8` | assigned |
| `satellite.variable.expression` | `1 6 9` | assigned |
| `satellite.variable.float` | `1 6 10 (0)` | assigned; **built at M15** — the `(0)` is new with its child below, `binary`'s and `hex`'s precedent |
| `satellite.variable.float.to_string` | `1 6 10 1` | assigned — **minted at M21**, and it is the type's FIRST method. `Float::to_string()` already existed in C++ and `satellite_value/render.cpp` was its only caller, so a float could be displayed and could not be turned into text: no `to_string`, `"x " + a` refused by S0711, `write_line(a)` by S0713. `Sky::save` writes 164 doubles into a text file and was unwritable for that reason while `Sky::load` worked, because `"0.4995225".to_number()` answers. Renders at the value's own precision, which is what `display` shows |
| `satellite.variable.float.string` | `1 6 10 2` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_string` `1 6 10 1` |
| `satellite.variable.float.number` | `1 6 10 3` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — EXACT, because a number is a decimal: `Float::to_number` is *"the one direction of conversion that needs no rounding rule"* |
| `satellite.variable.float.binary` | `1 6 10 4` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the bits when the value is whole; a fraction is S0733 |
| `satellite.variable.float.hex` | `1 6 10 5` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — as `.binary()`, four bits to a digit |
| `satellite.variable.hex` | `1 6 11 (0)` | assigned; also spelled `hexadecimal`; **built at M19.5** — the `(0)` is new with its children below. **No `as_number`** — binary's `1 6 5 4` reads its characters as a decimal, which `00FF` has no reading as; the row existed for part of 2026-09-09 and the author dropped it, so `digits` and `to_binary` took 4 and 5. Both were minted the same day and unseen outside this tree, which is the only condition §1.2's *never renumber* allows the shift under |
| `satellite.variable.hex.to_number` | `1 6 11 1` | assigned 2026-09-09 — what the digits are WORTH; `x00FF.to_number()` is 255 |
| `satellite.variable.hex.width` | `1 6 11 2` | assigned 2026-09-09 — how many BITS; `x00FF.width()` is 16, which keeps `write(x)`'s multiple-of-8 rule one rule |
| `satellite.variable.hex.to_string` | `1 6 11 3` | assigned 2026-09-09 — the characters `display` prints, the leading `x` included |
| `satellite.variable.hex.digits` | `1 6 11 4` | assigned 2026-09-09 — how many digits were written; `x00FF.digits()` is 4 |
| `satellite.variable.hex.to_binary` | `1 6 11 5` | assigned 2026-09-09 — the same bits as binary; never refuses, a digit being exactly 4 bits |
| `satellite.variable.hex(x)` | `1 6 11 6` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the bare spelling |
| `satellite.variable.hex.string` | `1 6 11 7` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_string` `1 6 11 3` |
| `satellite.variable.hex.number` | `1 6 11 8` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_number` `1 6 11 1` |
| `satellite.variable.hex.binary` | `1 6 11 9` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the same answer as `to_binary` `1 6 11 5` |
| `satellite.variable.hex.hex` | `1 6 11 10` | assigned 2026-09-12 — one of the conversion set the author asked for mid-M26: *"give them the `string(some_var)` `number(some_var)` `binary(some_var)` that will force that type"*, and *"give them the option of `some_var.to_string()` and `some_var.string()`"*. See §2.8 — the run itself, width and all |
| `satellite.variable.network` | `1 6 12` | assigned |
| `satellite.variable.thread` | `1 6 13 (0)` | assigned; **built at M23** — the `(0)` is new with its children below. A REFERENCE TYPE, which is `satellite.variable.file`'s row read across: two names for one thread are one thread, so `start()` through either reaches the same `pthread_t`. It gets NO `SAT_BUILT` row in `words.def` because its two children have handlers and `satellite_help/built.cpp` derives a parent from them |
| `satellite.variable.thread.start()` | `1 6 13 1` | assigned 2026-08-28; **built at M23** — makes one fresh OS thread and comes straight back. Answers NOTHING, because the capsule's answer does not exist yet; S1402 refuses a second start, since a thread runs once |
| `satellite.variable.thread.join()` | `1 6 13 2` | assigned 2026-08-28; **built at M23** — waits, and ANSWERS WHAT THE CAPSULE RETURNED. The author settled that on 2026-09-12: it is the one of the three verbs that waits until there is an answer, so it is the one that can have one. A capsule that refused on the thread re-raises ITS OWN diagnostic here, not a sentence about threads. S1403 refuses a join before a start, S1404 a second join |
| `satellite.variable.variant` | `1 6 14 (0)` | assigned — the `(0)` is new with its children below |
| `satellite.variable.variant.holding` | `1 6 14 1` | assigned 2026-09-03 |
| `satellite.variable.variant.holds(x)` | `1 6 14 2` | assigned 2026-09-03 |
| `satellite.variable.variant.held` | `1 6 14 3` | assigned 2026-09-03 |
| `satellite.variable.variant.clear` | `1 6 14 4` | assigned 2026-09-03 |
| `satellite.variable.window` | `1 6 15` | assigned |
| `satellite.variable.capsule` | `1 6 16` | assigned; **built at M23** — the type of a deferred call; `satellite.capsule` `1 2` is the keyword. What `satellite.thread.new(f(x))` builds: the capsule index and the ARGUMENT VALUES, worked out on the calling thread and frozen. **The one type in the language a program cannot yet declare and fill in** — it is reachable only as `new`'s argument, and M23.md §4 says what making it declarable would take. It is also the ONE row M23 added to `words.def`'s `SAT_BUILT` list, because it has no children for the built predicate to derive it from |
| `satellite.random` | `1 7 (0)` |  |
| `satellite.random.fast()` | `1 7 1` | the zero-argument shape — a refusal by design (DESIGN §11, 2026-09-04) |
| `satellite.random.normal()` | `1 7 2` | the same — a refusal by design (DESIGN §11) |
| `satellite.random.ultra()` | `1 7 3` | the same — a refusal by design (DESIGN §11) |
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
| `satellite.random.seeded(seed)` | `1 7 13` | assigned — **minted at M21**, the number PLAN §8 reserved for *"a fourth `satellite.random` shape that takes a seed"* and declined to assign at M13. Reseeds the tier's stream and answers nothing; **it does not spin**, which is the other half of why the three tiers cannot write `Rack::draw` |
| `satellite.random.seeded(min, max)` | `1 7 14` | assigned — **and it is the one draw in the language that takes FRACTIONAL bounds**, uniform over the grid of the value's own precision. The three spinning tiers refuse a fraction (S0905) because there is no uniform draw over the reals; a grid has one. `seeded(0, 1)` is QUAD's `rng.uniform()` |
| `satellite.random.seeded.range(min, max)` | `1 7 14` | ALIAS of the line above |
| `satellite.random.seeded(min, max, step)` | `1 7 15` | assigned — the step may be fractional too, and the same exact-division rule holds: the step must divide `max - min` exactly |
| `satellite.random.seeded()` | `1 7 16` | assigned — a refusal by design, the other three tiers' `1 7 1`–`1 7 3` |
| `satellite.file` | `1 8 (0)` |  |
| `satellite.file.new(path)` | `1 8 1` |  |
| `satellite.file.open(path, mode)` | `1 8 2` | shape written 2026-09-08 — v1's arity, `SAT_PATH(P_FILE_OPEN, ... 2)`. The mode is a WORD checked at run time and does not fold; §1.5's fold reaches selector calls at argument 0 only |
| `satellite.file.clear(path)` | `1 8 3` | shape written 2026-09-08 — the module face, by name. v1 had only the handle method; the handle keeps none, so this is the one spelling |
| `satellite.file.new(path, mode)` | `1 8 4` | assigned — not adjacent to shape one; §1.2 |
| `satellite.file.exists(path)` | `1 8 5` | **assigned 2026-09-08** — is there a file at this path, asked with no handle. Mirrors `satellite.directory.exists(d)` `1 18 3`. M19 |
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
| `satellite.library.main.arguments.machine.architecture` | `1 14 1 1 1 4` | assigned — M20. `uname.machine`, and NOT `cpu` `1 14 1 1 1 2` |
| `satellite.library.main.arguments.machine.byte_order` | `1 14 1 1 1 5` | assigned — M20 |
| `satellite.library.main.arguments.machine.page_size` | `1 14 1 1 1 6` | assigned — M20 |
| `satellite.library.main.arguments.machine.pointer_bits` | `1 14 1 1 1 7` | assigned — M20 |
| `satellite.library.main.arguments.memory` | `1 14 1 1 2 (0)` | assigned |
| `satellite.library.main.arguments.memory.total` | `1 14 1 1 2 1` | assigned |
| `satellite.library.main.arguments.username` | `1 14 1 1 3` | assigned |
| `satellite.library.main.arguments.system` | `1 14 1 1 4 (0)` | assigned — M20. The bare shape is the sub-object |
| `satellite.library.main.arguments.system.name` | `1 14 1 1 4 1` | assigned |
| `satellite.library.main.arguments.system.kernel` | `1 14 1 1 4 2` | assigned |
| `satellite.library.main.arguments.system.kernel_version` | `1 14 1 1 4 3` | assigned |
| `satellite.library.main.arguments.system.distribution` | `1 14 1 1 4 4` | assigned |
| `satellite.library.main.arguments.system.distribution_id` | `1 14 1 1 4 5` | assigned |
| `satellite.library.main.arguments.system.distribution_version` | `1 14 1 1 4 6` | assigned |
| `satellite.library.main.arguments.system.hostname` | `1 14 1 1 4 7` | assigned |
| `satellite.library.main.arguments.build` | `1 14 1 1 5 (0)` | assigned — M20. Baked in at compile time: these describe the binary that is RUNNING, not whatever compiler is installed now |
| `satellite.library.main.arguments.build.compiler` | `1 14 1 1 5 1` | assigned |
| `satellite.library.main.arguments.build.compiler_version` | `1 14 1 1 5 2` | assigned |
| `satellite.library.main.arguments.build.standard` | `1 14 1 1 5 3` | assigned |
| `satellite.library.main.arguments.build.flags` | `1 14 1 1 5 4` | assigned |
| `satellite.library.main.arguments.build.make` | `1 14 1 1 5 5` | assigned |
| `satellite.library.main.arguments.build.standard_library` | `1 14 1 1 5 6` | assigned |
| `satellite.library.main.arguments.build.c_library` | `1 14 1 1 5 7` | assigned |
| `satellite.library.main.arguments.build.built` | `1 14 1 1 5 8` | assigned |
| `satellite.library.main.arguments.interpreter` | `1 14 1 1 6 (0)` | assigned — M20. **The `(0)` IS THE PATH** and not a sub-object, the one asymmetry in this subtree: a grouping has no value of its own and a binary does |
| `satellite.library.main.arguments.interpreter.version` | `1 14 1 1 6 1` | assigned |
| `satellite.library.main.arguments.interpreter.library_path` | `1 14 1 1 6 2` | assigned |
| `satellite.library.main.arguments.interpreter.library_path_source` | `1 14 1 1 6 3` | assigned |
| `satellite.library.main.arguments.process` | `1 14 1 1 7 (0)` | assigned — M20. The bare shape is the sub-object |
| `satellite.library.main.arguments.process.id` | `1 14 1 1 7 1` | assigned |
| `satellite.library.main.arguments.process.parent` | `1 14 1 1 7 2` | assigned |
| `satellite.library.main.arguments.session` | `1 14 1 1 8 (0)` | assigned — M20. The bare shape is the sub-object |
| `satellite.library.main.arguments.session.shell` | `1 14 1 1 8 1` | assigned |
| `satellite.library.main.arguments.session.terminal` | `1 14 1 1 8 2` | assigned |
| `satellite.library.main.arguments.session.language` | `1 14 1 1 8 3` | assigned |
| `satellite.library.main.arguments.session.home` | `1 14 1 1 8 4` | assigned |
| `satellite.library.main.arguments.session.directory` | `1 14 1 1 8 5` | assigned |
| `satellite.library.main.arguments.length` | `1 14 1 1 9` | assigned — M20. v1's `argument_count`, respelled `length` the day the selectors landed: it DESCRIBES the command line and is not part of it, and `count` is now `1 4 3 2`'s word for how many entries the object holds |
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
| `satellite.directory.change(d)` | `1 18 1` | shape written 2026-09-08 — v1's arity, 1. Answers false rather than failing |
| `satellite.directory.current()` | `1 18 2` | shape written 2026-09-08 — v1's arity, 0 |
| `satellite.directory.exists(d)` | `1 18 3` | shape written 2026-09-08 — v1's arity, 1 |
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
| `satellite.system.delete(x)` | `1 22 1` | shape written 2026-09-08 — v1's arity, 1. `x` is a path OR an open `satellite.variable.file`, which is why the row is M19's. M19 |
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
| `satellite.system.memory.swap(unit)` | `1 22 4 4 6` | assigned — swap's own optional unit, which answers **how much swap is available** (the author, 2026-09-11) |
| `satellite.system.memory.swap.used(unit)` | `1 22 4 4 7` | assigned 2026-09-09 — M20. v1's unit block is reached for every swap form, so it ANSWERS this and §2.2 wrote `1 22 4 4 3` without the parens |
| `satellite.system.memory.this` | `1 22 4 5 (0)` | assigned |
| `satellite.system.memory.this.available()` | `1 22 4 5 1` | assigned |
| `satellite.system.memory.this.free()` | `1 22 4 5 2` | assigned |
| `satellite.system.memory.this.used` | `1 22 4 5 3` | assigned |
| `satellite.system.memory.this.available(unit)` | `1 22 4 5 4` | assigned |
| `satellite.system.memory.this.free(unit)` | `1 22 4 5 5` | assigned |
| `satellite.system.memory.this.used(unit)` | `1 22 4 5 6` | assigned 2026-09-09 — M20, and it was BLOCKING M20's done-when: `example/full_test.satl:613` is this call |
| `satellite.system.memory.free()` | `1 22 4 6` | assigned |
| `satellite.system.memory.total()` | `1 22 4 7` | assigned |
| `satellite.system.memory.used()` | `1 22 4 8` | assigned |
| `satellite.system.memory.main(unit)` | `1 22 4 9` | assigned |
| `satellite.system.memory.free(unit)` | `1 22 4 10` | assigned |
| `satellite.system.memory.total(unit)` | `1 22 4 11` | assigned |
| `satellite.system.memory.used(unit)` | `1 22 4 12` | assigned |
| `satellite.system.threshold()` | `1 22 5` | assigned — read |
| `satellite.system.threshold(n)` | `1 22 6` | assigned — set |
| `satellite.system.persist()` | `1 22 7` | assigned 2026-09-07 — read, §2.7 |
| `satellite.system.persist(x)` | `1 22 8` | assigned 2026-09-07 — set, §2.7 |
| `satellite.system.environment(name)` | `1 22 9` | assigned 2026-09-09 — M20. A SIBLING and not a child: §4's rule, a user-owned argument contributes no number of its own. Bare `environment` `1 22 2` answers the whole map |
| `satellite.thread` | `1 23 (0)` | assigned; **built at M23** — derived from `new` below it, so no `SAT_BUILT` row of its own |
| `satellite.thread.new` | `1 23 1` | assigned; **built at M23** — takes a CALL and packages it rather than performing it, which is DESIGN §13's settled form and the language's first DEFERRED argument. `words.def`'s seventh list (`SAT_DEFER`) is the declaration and `op_package` is the op; the argument's own arguments are evaluated where they are written, and the capsule is not entered until `start()`. Refuses anything that is not a capsule of the program's own with S1401 — **at compile time**, so `new(satellite.console.display("x"))` cannot print on its way to being refused |
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
| `satellite.class` | the number of `satellite.spacesuit` — added 2026-09-09 |
| `arg` `args` `argz` `argument` `arguments` `argumentz` `argv` | one node, **seven** spellings (DESIGN §7.7) — `argv` added 2026-09-09 |

DESIGN §4.4 describes the opposite arrangement — two nodes sharing one piece of
text, as `list` under `container` and `list` under `directory` do. **Both
directions are real** and the spelling table has to hold each: deduplication is
many-nodes-one-string, aliasing is one-node-many-strings.

**`argv` is the seventh and it is the only row here the author added after the
fact.** DESIGN §7.7 closed this question the other way at M7, on 2026-08-31:
the seventh spelling was *refused* rather than silently plain, and S0531 told a
parameter named `argv` that it was one edit from `arg`. §7.7 also named the one
door a reversal could come through — *"adding `argv` to `words.def` would make
it work and is an edit to the numbering, which WORD_NUMBERS.md is the authority
over and which is the **author's** to make rather than a milestone's to take in
passing"* — and on **2026-09-09** the author walked through it, having written
`argv` in a program and been refused by their own interpreter. That is §7.7's
*"people type what they type"* with the language's author as the witness, and
it is the reason this table has seven spellings in its third row and not six.

**S0531 keeps its number and its job.** It fires for `argu`, `arrgs`, `argvs`
and every other near miss; one of its examples became a spelling, which is not
the same as the code becoming wrong. Because the suggester reads the alias rows,
adding one here grows the list it offers with no edit to the resolver.

**`satellite.class` is the fourth row and the first alias on a word that is
neither a type nor a call shape.** Added 2026-09-09, by the author, for the
reason DESIGN §7.7 gives about `arguments`: people type what they type, and
`class` is what every other language calls the construct `satellite.spacesuit`
is. **`spacesuit` stays the word.** It is the node at `1 10`, it is what
`satl --unparse` prints back, and it is what `satellite.help` heads the entry
with; `class` is a second spelling that takes no number, which is what §2.3 is
for. The alias shadows nothing — `satellite` has no child spelled `class` — and
`words_invariants.hpp` checks that rather than trusting it.

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

**§2.2 is now 229 rows and 226 distinct numbers.** Counted mechanically after each
edit; the only duplicates are still §2.3's three aliases below.

**It was 222 and 219 until 2026-08-31, when M8 appended `digits` `1 6 4 15`.**
That milestone found `Number::digit_count()` ported, tested and printed by
`satl --number`, with nothing in §2.2 for the language to call it by — the mirror
image of `shift_left` and `shift_right`, which had numbers and no meaning.
**Appending is what §1.2 permits**: nothing moved, one thing went on the end of
`1 6 4`, and no already-written program changed meaning.

**And it was 223 and 220 until 2026-09-03, when M12 appended the variant's four**
— `holding`, `holds(x)`, `held` and `clear`, `1 6 14 1` through `1 6 14 4`, and
§2.5 below says who assigned them and by what walk. The same sentence holds:
nothing moved, four things went on the end of `1 6 14`, and no already-written
program changed meaning.

**And it was 227 and 224 until 2026-09-05, when M16 appended `search(pattern)`
twice** — under the map at `1 4 1 10` and under the list at `1 4 2 26`. §2.6
below says who assigned them and why one word is two rows. The same sentence a
third time: nothing moved, one thing went on the end of each container, and no
already-written program changed meaning.

### 2.5 Four numbers assigned 2026-09-03, and by whom

**The table below repeats numbers that already appear in §2.2 and is NOT a
source** — §2.4's warning, standing for the same reason.

**The author delegated M12 whole on 2026-09-03** — the blocker PLAN §8 reserves
for them (*is "nothing" a state every type has, or a value only a `variant` can
hold?*) and, with its answer, the variant's vocabulary — in the words "I don't
have a vote on this one." MILESTONES/M12.md §2 carries the answer and the
argument; DESIGN §8.7 is where the language states it. The numbers follow §1.1
exactly as §2.4's did: `example/variant.satl` is the walk, and the order its
lines first ask is the order below.

| path | number | what decided it |
|---|---|---|
| `satellite.variable.variant.holding` | `1 6 14 1` | §1.1 — the acceptance walk asks *what are you holding* before it asks anything else, so the word-answer is first |
| `satellite.variable.variant.holds(x)` | `1 6 14 2` | the same walk, three lines later — the yes/no form, one written argument, `box.holds("string")` |
| `satellite.variable.variant.held` | `1 6 14 3` | the walk takes the value out third — the checked extraction, S0714 by name when there is nothing to hand over |
| `satellite.variable.variant.clear` | `1 6 14 4` | met last — the one mutating row, the same word and the same write-back contract as `1 6 1 15` |

**`1 6 14` gained a `(0)` and did not change number** — the notation edit §2.4
records for `1 6 13`, made again for the same reason: every node in §2.2 with
children carries the marker. Nothing moved; five things were appended, four of
them numbered.

### 2.6 Two numbers assigned 2026-09-05, and by whom

**The author, directly, at M16.** v1 gives both containers a rich `.search(p)`
— one arm for both in `methods_containers.cpp`, a map per hit carrying the
value, the key, the path and the score — and the 2026-08-28 transcription
carried no row for it, so the search power's rich spelling was unreachable in a
language whose subscripts already searched. Asked whether M16 ships the
subscript form alone or mints the number, the author answered that the first
satellite is the base and had already answered this: *"Let's add another number
to our list of numbers."*

**One word, two rows, and that is §1.5 and not a duplication.** A selector's
number is reachable only through the receiver's type, so a word both containers
answer appears once under each — exactly as `size`, `empty` and `clear` already
do. `1 4 1 10` is the map's next free child and `1 4 2 26` the list's;
MILESTONES/M16.md carries what `.search(pattern)` answers and why the subscript
form is the short spelling of the same walk.

### 2.7 Two numbers assigned 2026-09-07, and by whom

**The author, directly, during M22.** The prompt keeps a finished program's
variables so they can be used afterwards, and the author asked for that to be
something a program can turn off and something the terminal can display:
*"let's make a `satellite.system.persist(true)` `satellite.system.persist(false)`
and we can even display what it is set to in the terminal... leave it on by
default."*

**Asked which node should own it, the answer was "just set it to the next
available number under `satellite.system`, whatever that number is."** That node's
children run 1 through 6, so the next is **7**.

**Two rows and not one, and §1.3 is why.** *The arity is the identity*: a call
that reads and a call that sets are different shapes and therefore different
numbers. That is not a reading imposed on the author's request — **the same
parent already carries the same pair**, `threshold()` `1 22 5` and `threshold(n)`
`1 22 6`, assigned in the 2026-08-28 transcription. `persist()` `1 22 7` answers
what the setting is; `persist(x)` `1 22 8` sets it. A single row would have had
to mean both, which is the one thing §1.3 says a number cannot do.

**It is the first child of `satellite.system` that anything implements.** §2.2 has
carried that node's thirty paths since the transcription and PLAN §8 reached none
of them; `delete` `1 22 1` waits for M19 and the whole `memory` subtree for M20.
So this is a namespace whose first working row is not its first numbered one,
which is ordinary — `handlers[path_id]` is a table with holes by construction —
and worth saying once, because a reader who sees `persist` answer may reasonably
expect `home` to.

**Where the setting lives is not a numbering question and §2.2 takes no view.**
MILESTONES/M22.md records what it is stored in and what happens when it is off.

### 2.8 Twenty-four numbers assigned 2026-09-12, and by whom

**The author, directly, during M26**, and the request arrived in three pieces
over one conversation. First the forced conversions: *"then also give them the
`string(some_var)` `number(some_var)` `binary(some_var)` that will force that
type"*. Then the selector spelling: *"give them the option of
`some_var.to_string()` and `some_var.string()` gives a string of some_var, and
give `.number()` and `.binary()` and `.hex()` and `hex(some_var)`"*. And then,
asked whether the bare form should be written out as
`satellite.variable.string(x)`: *"just `string(some_var)` is good enough I
guess, as we are not using this for anything else"*, and **"single word"**.

**Four names, two spellings each, on five types — and the numbers hang under the
TYPES rather than anywhere new.** §1.3 says a one-argument call shape is a child
of the node it is called on, which `satellite.help(x)` `1 19 1` already
demonstrates, so `satellite.variable.string(x)` is `1 6 1 18` — the next free
child of `string`. **The bare spelling a program writes is a RECOGNITION, not a
number of its own**: `string(n)` answers `1 6 1 18` because the resolver knows
the word, which is how §7.7's `arguments` reaches
`satellite.container.arguments` and what PLAN §8 asks of every bare identifier —
*the language recognises a name rather than introducing one*. This section
therefore mints twenty-four numbers and **no new parents**.

**Nothing was renumbered and no `to_` row was retired**, which §1.2 requires and
which is worth saying because the set overlaps them heavily: `to_string`
`1 6 4 6` and `.string()` `1 6 4 17` are two rows answering identically on
purpose. The `to_` names were minted one at a time as each type needed one, so
they are a set with holes — hex never got `as_number`, binary never got
`to_binary`, float had only `to_string` — and a program that just wants a value
in another radix should not have to know which milestone minted what.

**A float got all four, and the first cut of this section gave it one.** The row
was written saying a float may only be rendered, because reading one as a whole
number "loses the fraction" — and that is false in this language.
`satellite.variable.number` is a **decimal** bignum: `n = 1.5` holds 1.5 and
`n + 0.25` is 1.75, measured before this paragraph was rewritten.
`satellite_float.hpp` says the same from its own side, calling `to_number`
*"exact, because L + R is a finite decimal by construction … the one direction
of conversion that needs no rounding rule."* What a fraction genuinely has no
form for is **bits**, so `f.binary()` refuses with S0733 and `f.number()`
answers — the obstacle is the bit run's shape, which is true, and not the
number's precision, which was not.

**`1 6 10 3`, `1 6 10 4` and `1 6 10 5` exist even though two of them usually
refuse, and that is the point of having them.** Leaving them out made
`f.number()` answer S0723 — *"`number` is not a question a float answers"* —
which is the numbering saying *no such word* about a word every other scalar
has. The mistake is not that the name is unknown; it is what the conversion
would have to do. A row that refuses in its own sentence is worth more than a
hole that refuses in the numbering's.

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
kind of thing that gets decided by accident at M16.

> **Settled 2026-09-08, at M19, and it was not decided by accident.** A program
> writes `satellite.file.new(path)` `1 8 1`. `1 6 2 1` is the dispatch row and
> has no surface spelling — DESIGN §13's settled `satellite.thread.new` entry
> already reads that way, calling the two-part shape the one *"`satellite.file.new`
> and `satellite.time.new` already do"*, and v1 has no `new` handle method at
> all. **`open` is NOT a second collision of the same kind**: `1 8 2` opens a
> path and `1 6 2 2` reopens a handle that was closed or whose open failed, so
> they are two operations that share a word rather than two spellings of one.
> M19 builds `1 6 2 2` and leaves `1 6 2 1` a numbered row with nothing behind
> it, which `satellite.help` says out loud.

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
other number operation living under `satellite.variable.number`, which holds fourteen
more of them at `1 6 4 2` through `1 6 4 15`. **It was a slip and §5.5 is
corrected**, and §5.5 says what a shift MEANS as of 2026-08-31 rather than only
where its path hangs. *(2026-08-27.)*

---

*Companions: [DESIGN.md](DESIGN.md) — what the language is, and §4 for why the
numbering has this shape. [PLAN.md](PLAN.md) — how it gets built, and §8 for the
milestone that turns this file into code. [LAYOUT.md](LAYOUT.md) — every file in
the tree.*
