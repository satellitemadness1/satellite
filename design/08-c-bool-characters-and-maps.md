*satellite design docs, §8, part 3 of 4. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§8 part 2](08-b-time-file-and-window.md), On: [§8 part 4](08-d-size-length-and-arguments.md).*

---

## 8. Types — bool, characters and maps

*Continues [§8 part 2](08-b-time-file-and-window.md).*

### 8.4 Writing a `bool`

`satellite.variable.bool` was in the type table from the beginning with **no way to write
either of its two values**, which made the type unreachable from source: a bool could only
arrive as the result of a comparison. `satellite.bool.true` and `satellite.bool.false` are
those two values, and they are module constants — §5's dispatch table already warned against
requiring a `(` after a module path "or module constants like `satellite.math.pi` become
errors", and these are the first ones to exist.

The spelling costs no parser rule, no lexer keyword and no second reserved word: segment-1
dispatch sends `bool` down the module path like any other name.

It cannot be `satellite.variable.bool.true`, however much that reads like the type it
belongs to, because §4 reserves `satellite.variable.*` as a **type** namespace in which no
path is ever a value expression, and that reservation is exactly what keeps `<` unambiguous.
`bool` as a module segment and `bool` as a type name are different things, and keeping them
apart is the point. The obvious alternative — bare `true` and `false` — would be the
language's second and third reserved words, and §1 has exactly one.

**That last objection is about reserved words, not about the spelling, and §19.7
answers it without creating one.** Bare `TRUE` and `FALSE` are now accepted, but
only after every scope the user owns has been asked and said no — so a variable,
field, capsule or spacesuit called `TRUE` still wins the word, §1 holds exactly as
written, and nothing is reserved. `satellite.bool.true` and `satellite.bool.false`
remain canonical and remain the spelling this document recommends.

### 8.5 A character is 16 bits

`SatChar` is `char16_t` (satellite_string.hpp:31). It was `char32_t` until 2026-08-08, and the
width is worth a subsection because it is the one decision in the type table that is paid for
by every byte of data the language ever holds.

**Why not 32.** Nothing needs it. The code table assigns 101 codes — void, `a`–`z`, `A`–`Z`,
`0`–`9`, 32 punctuation, six live system characters — and everything unassigned round-trips
through a raw area of 256 codes, one per possible byte. 357 codes, in a space of four billion.
The other 99.99999% was paying rent as memory.

**Why not 8**, which is the tempting answer because it would put the whole C string library
within reach. 357 does not fit in 256. Shrinking the raw area is not available either: it
exists so that a satellite string can hold an arbitrary byte, and there are 256 of those. The
arithmetic decides this, not taste.

Two properties would also have been lost, and they are the ones that make this our string
rather than `char *`. A satellite string may contain a NUL, so `strlen` cannot measure it. And
`\home`, `\cwd` and their four siblings are **live** — single codes that expand at `decode()`
time, so a string reflects the current directory *now* rather than when it was built. Neither
survives being a C string, and §3.3 has already found the escape-timing bug three times.

**What 16 costs and buys.** It holds 357 codes with about 32,000 spare, which is where the
table grows when space and the rest of ASCII get real codes. Measured on a 33.7M-character
corpus: peak RSS 169 MB → 101 MB, the string itself 135 MB → 67 MB, an exact halving. On the
text-ingestion workload this language is being built for, that is the whole point.

Nothing above the representation moves. `length()` still counts characters, and `encode_raw`
still maps one input byte to one `SatChar` — which is what §3.3 requires so that a span stays
a byte offset and an error caret points at the right column. §17.3 carries the consequence for
the bytecode: 16 characters to a 256-bit unit instead of 8.

### 8.6 `satellite.container.map<K, V>`, and what may be a key

The second container. §15 ranked it fourth among the gaps stage 0 found and attached the only
deadline in that list: *"every symbol table is a linear scan — correct, but O(n) inside a tree
walker is what decides whether self-compilation takes seconds or minutes."*

```
satellite.container.map<satellite.variable.string, satellite.variable.number> m
m.set("bolt", 40)
m["bolt"]          40
m.get("bolt")      40          identical; the subscript is sugar for .get
m.has("nut")       false
m.keys()           [bolt]      insertion order
m.remove("bolt")
```

**It is insertion-ordered**, and that is load-bearing rather than decorative. §5's only loop is
the C-shaped `for`, so a map is walked by indexing `.keys()`; an unordered map would make every
program that walks one non-deterministic, and §15's stage 3 self-compiles to a **byte-identical
fixpoint**, which is a test that unstable iteration order makes unfalsifiable. It also keeps
the test suite honest, since `eval_test`'s `check_output` is exact string equality. `set` on an
existing key **updates in place and keeps its position** — a symbol table that reordered itself
whenever a binding was refined would make `.keys()` useless for reporting.

**A missing key is an error**, and `.has(k)` is how to ask without one. §7 already settled the
sibling case — an out-of-range *index* is an error, only a *slice* clamps — and the same answer
costs nothing here. Returning nil instead is not merely a different taste, it is wrong: nil is
a legitimate **stored** value, since a spacesuit-typed or file-typed slot defaults to one, so
nil-for-absent would make "absent" and "present but nil" the same answer.

**A key is a `satellite.variable.string` or a `satellite.variable.number`, and nothing else.**
Rejected at resolve time in the declaration, and again at insertion, because a bare `map` with
no type arguments skips the first check. The reason is §7's invariant that no satellite code
runs inside `Library::update()` — re-entering a held write lock deadlocks — so hashing and
equality must be native C++ and can never be a user hook. Three traps, each of which the
implementation comments name at the site:

- **A number has two representations of one exact value.** `Number`'s equality is
  `compare() == 0`, so `1` and `1.0` are the same number held as `sig 1 exp 0` and
  `sig 10 exp -1`. Canonicalising through `to_string()` is what makes the key agree with `==`,
  because `to_string` renders only from `normalized()`, which strips trailing zeros. `1` and
  `1.0` are one key, which is the only answer consistent with `1 == 1.0` being true.
- **A string is hashed over its `SatChar` codes and never over `decode()` output.** §8.5's
  `\home`, `\cwd` and `\user` are *live* and expand at decode time, so hashing decoded text
  would make a key's identity depend on the machine, the user and the current directory — and a
  key inserted before a `satellite.directory.change()` would stop being findable after it. Two
  strings that decode alike are different keys, and that is correct.
- **A spacesuit is rejected outright.** §14 makes an instance's equality *identity*, and §12
  defers user-defined operators, so two logically-equal objects would be different keys forever
  with no recourse available to the programmer.

A one-byte type tag precedes the canonical bytes, so the number `12` and the string `"12"` are
different keys rather than the same one by accident.

**Cost, stated plainly.** Lookup is O(1). **Insertion is O(n)**, because the body is copied on
write — which is not a new class of cost but parity with the language's only other insertion
primitive: `.append` copies the whole backing vector too, measured at 0.04 / 0.18 / 0.77 s to
build lists of 2000 / 4000 / 8000, which is quadratic. Copy-on-write is what §8.3's
immutable-`Value` contract requires: a reader holding a snapshot must keep seeing it. The
follow-on, deliberately deferred, is an in-place fast path when the slot's handle is unshared —
safe for a **frame** slot, which §6 makes reachable from one thread, and never for a field or a
global. It would fix `.append` in the same stroke.

**`m[k] = v` is not in the language**, exactly as `l[i] = v` is not: assignment resolves a
storage slot, and an index expression names none. `m[a:b]` is meaningless and is refused.

---
