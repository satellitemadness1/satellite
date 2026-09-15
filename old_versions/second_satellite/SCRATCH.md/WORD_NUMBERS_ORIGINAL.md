# The numbering — from 36 hand-written lines to 254 nodes in code

**This file is mine now.** *(Handed over 2026-08-28, by the author, with M2
landed.)* It used to be one thing: a verbatim copy of what they typed, kept
because `WORD_NUMBERS.md` was never committed and rewriting it in prose overwrote
the only copy. That copy is still here in §1 and **has not been touched** — it is
the one part of this file nobody may edit, because it is a record of what somebody
actually wrote on a day, and rewriting it would falsify the record rather than
correct it.

What is new is everything after it. This is now the **account of the numbering**:
where it came from, what each of those 36 lines turned into, what the two `?`s in
it resolved to, what it looks like in code, and what is still open.

**It is not the authority and it never can be.** [WORD_NUMBERS.md](../WORD_NUMBERS.md)
§2.2 is, and `src/satellite_words/words.def` is the copy a compiler checks. When any
of the three disagree, §2.2 is right and the other two are a bug. This file explains;
it does not decide. That is the project's own rule — *prose may explain a number; it
may never be the only place the number lives* — applied to the file that is most
tempted to break it.

**Still scratch, and still deletable**, but no longer for the old reason. It was to
be deleted "once the rewrite has been read and accepted"; that happened. It survives
because the history in it is not written down anywhere else, and it should go when
somebody decides that history has stopped being useful.

---

## 1. The original, verbatim — 2026-08-27

**Do not edit this section.** Every number in it is preserved unchanged in
`WORD_NUMBERS.md` §2.2. Their two parenthesised comments and their two `//` notes
are preserved there as table annotations and as §3's blockquote.

```text
satellite.include()          1 1 (0)
satellite.include(satellite) 1 1 1
satellite.capsule            1 2 (0)
satellite.main               1 3 (0)
satellite.container          1 4 (0)
satellite.container.map      1 4 1
satellite.container.list     1 4 2
satellite.console            1 5 (0)
satellite.console.display    1 5 1
satellite.console.input      1 5 2
satellite.variable           1 6 (0)
satellite.variable.string    1 6 1
satellite.variable.file      1 6 2
satellite.variable.time      1 6 3
satellite.variable.number    1 6 4
satellite.random.fast        1 7 1
satellite.random.normal      1 7 2
satellite.random.ultra       1 7 3
satellite.file.new           1 8 1
satellite.file.open          1 8 2
satellite.time.now           1 9 1
satellite.time.new           1 9 2 (set the arguments for a point in time)
satellite.file.clear         1 8 3 (empty the contents of a file;)
satellite.spacesuit          1 10 (0)
satellite.protected          1 11 (0)
satellite.public             1 12 (0)
satellite.statement          1 13 (0) // this one kinda doesn't do anything
satellite.statement.if       1 13 1
satellite.statement.for      1 13 2
satellite.statement.while    1 13 3
satellite.statement.else     1 13 4
satellite.library            1 14 (0)
satellite.library.main       1 14 1
satellite.library.x          1 14 ? // this needs to be built-in to grab the next available number, as x is just a capsule name; so this needs to grab 1 14 x where x is the next available number, and a similar thing needs to happen with user defined classes (spacesuits)
satellite.return()           1 15 ?
satellite.return(satellite)  1 15 1


there was one thing that when we programmed it, it took 4 numbers; I can't remember
what it was though;

so all user defined capsules and all user defined spacesuits need to grab the next
available number; the language needs to keep and always have ready the next available number;
```

**36 numbered lines, two of them carrying a `?`, and two loose paragraphs.**
Everything below is what those became.

---

## 2. What the 36 lines are now

**All 36 survive with their numbers unchanged.** Not one was renumbered, which is
the promise DESIGN §4.3 makes — *never renumber, never reuse, always append* — and
the first place it was tested was the transcription itself.

Most gained precision rather than a different number, because §1.3 later settled
that **a number identifies a call shape and not a path**. So a line written as a
bare word is now written with the shape it was always describing:

| written 2026-08-27 | in §2.2 now | why it moved in spelling only |
|---|---|---|
| `satellite.console.input` `1 5 2` | `satellite.console.input()` `1 5 2` | `input` has three shapes; this is the one taking nothing. `(prompt)` and `(prompt, target)` are `1 5 3` and `1 5 4`, appended after |
| `satellite.random.fast` `1 7 1` | `satellite.random.fast()` `1 7 1` | the same, three tiers deep: `(digits)`, `(min, max)` and `(min, max, step)` are `1 7 4`–`1 7 6` |
| `satellite.file.new` `1 8 1` | `satellite.file.new(path)` `1 8 1` | and `new(path, mode)` went to `1 8 4`, **not** beside it — §1.2's "variants are not necessarily adjacent" is this row |
| `satellite.include()` `1 1 (0)` | `satellite.include()` `1 1 0` | the `(0)` and the trailing `()` are the same thing, which §1.3 defined a day later |

**The `(0)` in seven of those lines turned out to be the most load-bearing notation
in the file**, and it was used before it was defined. §1.3 now says what it means —
the node is `1 4` and its zero-argument call shape is `1 4 0`, a real number and not
a piece of punctuation — and `words.def` encodes exactly that: a `SAT_BARE` row at
position 0 that takes no position from its siblings.

### 2.1 The two `?`s, both resolved

**`satellite.return()` `1 15 ?` → `1 15 0`.** By §1.3, not by choice: `0` means
nothing in that position, so the zero-argument shape of `return` is `1 15 0`, the
same shape `include()` has. `return(satellite)` stayed `1 15 1` as written, and
`return(value)` was appended at `1 15 2` because a user value has no number to
contribute.

**`satellite.library.x` `1 14 ?` → whatever is free when `x` is first met, which
today is `1 14 3`.** This one was not a gap to fill in; it was **a feature being
specified**, and the note beside it is the whole of PLAN §8.1:

> *this needs to be built-in to grab the next available number, as x is just a
> capsule name*

It is built. `words::Words` keeps a live child counter on every node, seeded from
the frozen table, and hands out the next free number when a name is first met.
`satellite.library` has `main` at `1 14 1` and `system` at `1 14 2` frozen, so a
user's `x` takes 3 — verified by running it, not by reading it.

**And the consequence the note did not mention, which is the sharp one:** a user's
number is **not stable between runs**, because it is not the same name in two
programs. So `is_language_word(id)` is the predicate that separates the two halves,
and anything that writes a `PathId` down — a `.satc`, Satellite Orbit, a wire format
— must record the **name** instead. SATC.md §3 is that rule and it exists because of
this line.

### 2.2 The four-number memory, identified

> *there was one thing that when we programmed it, it took 4 numbers; I can't
> remember what it was though*

Two families in the first satellite took four segments, both in
`old_versions/first_satellite/src/bytecode_format/format.def`:

    satellite.random.fast.range(low, high)     format.def:598
    satellite.random.normal.range(low, high)   format.def:599
    satellite.random.ultra.range(low, high)    format.def:600

    satellite.system.memory.used([unit])       format.def:552
    satellite.system.memory.free               format.def:553
    satellite.system.memory.total              format.def:554
    satellite.system.memory.swap               format.def:555
    satellite.system.memory.main               format.def:556
    satellite.system.memory.frequency          format.def:557
    satellite.system.memory.bit                format.def:558

**`.range` was the likelier memory and it turned out to be the interesting one.**
It hangs off the three tiers already numbered at `1 7 1`, `1 7 2` and `1 7 3` — and
under this numbering it is **not four numbers at all.** §2.3 makes it an **alias**:
`satellite.random.fast.range(min, max)` is a four-segment *spelling* of a node that
is three numbers, `1 7 5`. One node, two spellings.

Those three rows are the only duplicate numbers in the language, and they are the
entire difference between §2.2's 222 rows and its 219 numbers.

**Why four felt like a limit rather than a length**: v1's macro was
`SAT_PATH(ident, s1, s2, s3, s4, arity)` — literally four segment slots and no
fifth. DESIGN §4.5 names that as a limit this language refuses, and §1.4 refuses to
fix a depth at all. The deepest path today is six,
`satellite.library.main.arguments.machine.cores` `1 14 1 1 1 1`, and it interns to
the same four bytes as `satellite.main`.

---

## 3. What the numbering is now

| | |
|---|---:|
| rows in WORD_NUMBERS.md §2.2 | **222** |
| distinct numbers in §2.2 | **219** |
| duplicate numbers, all three declared aliases | **3** |
| `(0)` markers in §2.2 | **35** |
| — | |
| nodes in `words.def` | **254** |
| of which numbered children | 216 |
| of which bare call shapes | 38 |
| aliases in `words.def` | **9**, over 5 nodes |
| deepest path | 6 segments |
| `sizeof(PathId)` | 4 bytes |

**254 is a count no document had**, and it is the one figure here that is not in
§2.2. That section's 219 does not include the bare shape a `(0)` marker names,
because the marker rides on the row of the node it belongs to — 216 numbered rows
plus 38 bare shapes is 254. Both counts are right about different questions.

**Nine aliases over five nodes**: the three `.range` rows, `hexadecimal` for `hex`,
and five of DESIGN §7.7's six spellings of `arguments` (the sixth is the node's own
text). That is the whole of the language's aliasing.

### 3.1 How it is written down

`words.def` is `SAT_NODE(parent, ident, text, kind)` and **the number is not a
column** — it is the row's position among its parent's `SAT_NUMBERED` siblings,
computed at compile time. The `// 1 5 1` at the end of each row is a comment nothing
reads; it is there so the file can be checked against §2.2 by eye.

Three consequences worth keeping:

- **Order is meaning, everywhere in that file.** A row inserted in the middle
  silently renumbers every sibling after it.
- **Three of PLAN M2's four checks needed no `static_assert`.** A number is a
  position, so holes and duplicates cannot be expressed; an alias names an
  identifier, so a bad one is a compiler error. Only *no orphans, no cycles* was
  left to assert.
- **The transcription is the one thing no assert can reach**, so `tests/words_test`
  opens `WORD_NUMBERS.md` and walks all 222 of its paths — in both directions, since
  2026-08-28. Deleting a row from `words.def` compiles clean and every assert
  passes; only the authority check sees it.

### 3.2 The two depths, which is the subtlety in the original

The original writes `satellite.include()` as a *child* of `include` and
`satellite.console.input` as a *sibling* of `display`. Both are correct and they are
one rule, which §1.3's definition of `(0)` settled:

> **A word reached bare holds its shapes as children. A word only ever called does
> not exist apart from them, so its shapes are siblings.**

`include` has a number of its own — it is `1 1` — so `include()` is `1 1 0` beneath
it. `input` has no number anywhere in §2.2, so its three shapes take slots beside
`display`. `match_shape()` looks in both places, in that order, and nothing else in
the code has to know which kind a word is.

---

## 4. Corrections this history produced

Kept because each was found by counting rather than by reading, and each had
survived several readings before that.

- **`satellite.number.shift_left` was a slip** for
  `satellite.variable.number.shift_left`. DESIGN §5.5 implied a top-level `number`
  namespace that nothing else in either document has. *(2026-08-27.)*
- **§1.1's "walk the program" order does not regenerate 1 to 15.** Read strictly,
  DESIGN §3 meets `variable` inside the parameter's type before line 5 reaches
  `console`, so a literal walk gives them the other way round. **The numbers are
  frozen and right; the sentence was the imprecise part** — and the file already
  held its own proof, since `satellite.return` is met on §3's last line and is 15,
  not 7. *(2026-08-28.)*
- **§1.4 called `satellite.random.fast.range` "four numbers and nothing else"** in
  the same commit that made it a three-number alias. *(2026-08-28.)*
- **The container methods are 34, not 29.** `SCRATCH.md/MILESTONE.md` §0.3 counted
  the list at twenty when §2.2 gives it twenty-five, and PLAN M10 inherited the 29
  into a sentence that names nine and twenty-five two lines above. Its "35 implied"
  becomes 40. *(Found 2026-08-28, at M2 — the first time anything counted those rows
  instead of repeating the count.)*
- **`satellite.variable.thread.start()` and `.join()` were numbered on 2026-08-28**
  and LAYOUT.md still said they were not. *(Fixed at M2.)*

---

## 5. What is still open about the numbering

Not scheduling — that is `MILESTONE.md`'s job. These are questions about the numbers
themselves, and they are WORD_NUMBERS.md §4's list as it stands:

- **The variadic split is mechanical but not small.** Every variadic path from the
  first satellite has to become one number per call shape, and the accepted shapes
  have to be read out of the v1 evaluator rather than guessed — `SAT_VARIADIC`
  recorded that a count varied without recording which counts were legal.
- **`satellite.file` `1 8` and `satellite.variable.file` `1 6 2` both carry a
  `new`.** Nothing says which a program should write, and two spellings for one
  construction is the kind of thing that gets decided by accident at M10.
- **`satellite.thread.new` has one number for two shapes.** `1 23 2` is free if that
  is decided the other way.
- **`satellite.container.set` is deliberately absent** and `1 4 5` is free.
- **What happens to a seventh spelling of `arguments`.** DESIGN §7.7: a parameter
  named `argv` gets a plain list with no properties, silently, and §9 says that
  silence is wrong.

---

*Companions: [WORD_NUMBERS.md](../WORD_NUMBERS.md) — the authority, and the only
place a number is decided. [DESIGN.md](../DESIGN.md) §4 — why the numbering has this
shape. [PLAN.md](../PLAN.md) §8 — M2, which turned it into code, and §8.1, which is
the `satellite.library.x` line above. [SATC.md](../SATC.md) §3 — why a user's number
may never be written down. [MILESTONE.md](MILESTONE.md) — which of these paths any
milestone actually builds, which is a different and worse question.*
