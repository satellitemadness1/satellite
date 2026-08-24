*satellite design docs, §8 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§7](07-method-calls-indexing-slicing.md), On: [§9](09-runtime-architecture.md).*

---

## 8. Types

Each type is a C++ struct with named fields, exposed through accessor methods.

| satellite type | C++ representation | notes |
|---|---|---|
| `satellite.variable.bool` | `bool` | exists; written `satellite.bool.true` / `.false`, see §8.4 |
| `satellite.variable.number` | `Number` (arbitrary precision) | **replaces `double`** — see §8.1 |
| `satellite.variable.string` | `SatString` | exists; 16-bit code table, see §8.5 |
| `satellite.container.list<T>` | `std::vector<ValuePtr>` | exists; children shared by pointer |
| `satellite.container.map<K, V>` | `MapBody` behind a handle | exists; insertion-ordered; keys restricted, see §8.6 |
| `satellite.variable.time` | `struct Time { int64_t ns; }` | absolute instant, UTC |
| `satellite.variable.file` | `shared_ptr<FileHandle>` | reference type |
| `satellite.variable.window` | `shared_ptr<WindowHandle>` | reference type |
| `satellite` | the runtime singleton | `satellite.return(satellite)` |

### 8.1 Numbers — one type, arbitrary precision

"Technically infinite" should mean **exact arbitrary-precision decimal** (bignum significand
× 10^exp), named `satellite.variable.number`. Not rationals (denominators grow without bound
and collide with the memory watchdog), not integer-only (`1/3` → 0 is unacceptable), not double.

**`double` must leave the variant.** **Verified by doing the migration on a copy of the
project**: replacing `double` with a 32-byte
`Number { long long sig; int exp; shared_ptr<const BigInt> big; }` leaves
`sizeof(ValueBase)` unchanged at 40 bytes, and the migrated `library_test` passes 160,000
concurrent increments TSan-clean. Only four source files plus the Makefile change.

Three traps, each found only by compiling:

1. After removing `double`, `Value v = 3.14` (`main.cpp:41`, and again at `main.cpp:94`)
   does **not** fail to compile — it silently truncates to `3`, with zero warnings under
   the project's actual `-Wall -Wextra -O2`.
2. The obvious guard `Number(double) = delete;` then breaks `Number(42)`, because deleted
   overloads still participate in resolution. What works is a template constructor
   constrained to integral types **plus** deleted float overloads.
3. "Keep both `double` and `Number`" is worse than it looks: `Value v = 4096.0` silently
   selects `double` while `Value v = 42` is a hard compile error. That would split
   `satellite.library` into two numeric representations and make the watchdog quietly ignore
   any user retuning of `min_free_mb`.

Implement with **base-10^9 limbs**, not base 2^32 — decimal I/O and 10^k scaling become
trivial, which is most of a decimal type's work. Roughly 400 lines for integer-only
`+ - * / %` and decimal I/O; ~800 for full decimal.

**Staging:** ship v1 with the `Number` *type name and semantics* even if the first
implementation is int64-backed, so adding full precision later is invisible to programs
already written.

### 8.1.2 What landed, and the three traps in the flesh

**Done**, in `bignum.hpp` / `bignum.cpp`. `sizeof(ValueBase)` is unchanged at 40 bytes and
the whole suite passes, including `library_test` under TSan.

The representation is §8.1's, exactly: `long long sig; int exp; shared_ptr<const BigInt>
big`, 32 bytes, with a **null `big` as the fast path**. That split is what keeps the
migration cheap at run time — a loop counter, an index and every small literal live entirely
in the `long long`, so `i = i + 1` is an add and an overflow check with no allocation and no
atomic. Measured on the §14 benchmark: **2.79 s against the double version's 2.79 s**, which
is to say the exactness is free at this workload.

`BigInt` is base 10^9 little-endian limbs, per §8.1's reasoning: this backs a decimal type,
so decimal I/O and scaling by 10^k are most of the work, and both are limb shifts in base
10^9 against full base conversions in base 2^32. Division is long division one **decimal**
digit at a time rather than Knuth's algorithm D in base 10^9 — the inner loop runs at most
nine times, and getting nine comparisons right is a different proposition from getting
base-10^9 quotient estimation and its correction step right.

**All three traps fired.** They were predicted on paper and every one of them turned up as a
real compile error during the migration:

1. `Value v = 4096.0` in `main.cpp` — the memory watchdog's `min_free_mb` default. Under the
   double version this compiled clean and silently truncated. It is now a compile error.
2. The constrained template plus deleted float overloads is what makes that error appear
   while `Number(42)` still works. `bool` is excluded from the template so `Value v = true`
   still selects the variant's `bool`.
3. `system.cpp`'s watchdog read `get_if<double>(min_free_mb)` — precisely the site §8.1
   warned would "quietly ignore any user retuning" if both representations coexisted.

**What is and is not exact.** Addition, subtraction and multiplication are exact with no
bound on digits, and so is division that terminates — which includes every division by a
power of ten, so a count of nanoseconds becomes an exact number of seconds. Division that
does not terminate is rounded to `satellite.library.system.division_digits` significant
figures, default 34 (decimal128's precision). That is the only rounding anywhere in the
language, and it is unavoidable rather than chosen.

### 8.1.1 How a number renders

**The rule is "print the value", never "print N significant digits."** A whole number
prints whole; a fraction prints the shortest digit string that reads back as the same
number.

This is phrased as a rule about *values* on purpose, and that is what let §8.1's migration
happen underneath it: the `double` became an arbitrary-precision decimal, "print the value"
went on meaning the same thing, and programs written before it kept printing what they
printed. A rule phrased in significant digits would not have survived. The *implementation*
this section used to name did not survive it, and where it went belongs here rather than
only in a code comment. It was `std::to_chars` in `std::chars_format::fixed`, in
`value.cpp`; there is no `to_chars` anywhere in the tree today. Rendering is
`Number::to_string` in `bignum.cpp`, because how a number renders is a property of the type
rather than of the printer — which is the reason `value.cpp` no longer answers the question
at all.

**The notation boundary was restated, and it moved.** It was `[1e-6, 1e21)`, on the grounds
that outside it digits stop being informative and `1e308` in fixed notation is 309
characters. That was true of a `double`, where a 33-digit integer had no 33 informative
digits to give. An exact decimal has them, so `bignum.cpp` rewrote the boundary in the terms
the reasoning had always really been about:

> switch to scientific when the padding zeros would outnumber the information, never when
> the information itself is long.

The thresholds are 20 trailing and 5 leading zeros, which are the old bounds restated — 20
trailing zeros is `1e20`, 5 leading is `1e-6` — so every value that printed fixed before
still does. What changed is what happens above them. `1e308` is one significant digit and
308 zeros and stays scientific; `30!` is 26 significant digits and 7 zeros and prints whole:

```
265252859812191058636308480000000
```

That value is 2.65e32, comfortably outside `[1e-6, 1e21)`, so the old rule would have
rendered it `2.6525285981219105863630848e+32` — hiding the exact answer the type went to
the trouble of computing, which is the one thing an exact type must not do.

**Verified defect this fixes.** The original `snprintf(buf, sizeof buf, "%g", d)` printed
six significant digits, so `123456789` displayed as `1.23457e+08` and `2000000 + 1` as
`2e+06` — wrong values, through the only output path the language has, since
`satellite.console.display`, the REPL echo, `.to_string()` and every error message that
quotes a number all arrive at the same function. Note that neither obvious alternative
works: `%.17g` round-trips but renders `0.1` as `0.10000000000000001`, and `to_chars`'
own `general` mode picks whichever notation is *shorter*, so it returns `1.23456789e+08`
for the same number. `fixed` was the one that was exact and readable at once, which is where
the `double` era settled; `Number::to_string` inherits the requirement rather than the
function.

Exactness is shown rather than hidden: `0.1 + 0.2` renders `0.3`, because §8.1's decimal
migration made `0.3` the value that is actually there. Before it the same expression
rendered `0.30000000000000004`, which was equally the value that was actually there, in a
type that could not hold the other one. Neither answer was ever a rounding rule bolted onto
the printer, which is the point: the printer never decided anything.

### 8.2 Time

**Verified**: time cannot be a `double`. Epoch nanoseconds now is 1785988800000000000,
needing 61 bits; a double's 53-bit mantissa gives 198 ns resolution at the current epoch, so
`satellite.time.now()` called twice in quick succession can return the identical value.

`struct Time { int64_t ns; }` — 8 bytes, does not grow the variant (SatString's 32 bytes
already dominate). **Absolute instant only** in v1: nanoseconds since the Unix epoch, UTC,
no timezone stored, no instant/duration mode flag. `a.minus(b)` returns a `number` of
nanoseconds. Adding `satellite.variable.duration` later only adds overloads.

### 8.3 File and window are reference types

The immutable-Value contract **survives**, but only because it was never a claim about the
resources a Value names. The `shared_ptr` member never changes after construction, so the
Value node's bytes are immutable and the lock-free snapshot protocol is completely
unaffected. What is mutable is the OS file description behind it.

State the split explicitly rather than hedging:

> satellite has **value semantics for values** and **reference semantics for the external
> resources that `file` and `window` name**. Copying a file value copies the handle, not the file.

True value semantics is not achievable: `dup()` shares the file offset, so independent
offsets require re-`open()` by path, which fails for pipes, sockets and unlinked files.

Ship an explicit `my_file.close()` returning a status — a destructor cannot report that
`close()` failed with ENOSPC/EIO, and buffered writes commit at close. After close, other
snapshots see a closed handle and get a clean language-level error; never silently reopen.
Guard fd state with `std::atomic<int>`. RAII in the destructor stays as the backstop.

**Require accessor methods (`my_window.height()`); do not add bare field access.** R4
specified method calls, and consistency matters more than the saved parentheses here.

### 8.3.1 What landed for `file`

**Done.** `satellite.file.open(path, mode)` returns a `satellite.variable.file`, and the
value answers `.ok()`, `.path()`, `.error()`, `.read()`, `.write(s)` and `.close()`. The
handle is the `shared_ptr<FileHandle>` §8.3 specified, with `std::atomic<int>` guarding the
descriptor, an explicit `close()` reporting its own status, and RAII in the destructor as
the backstop. The window is not built, so M7 is what remains of this section.

Two decisions §8.3 did not have to make, both forced by the first line of code that opens
a file:

- **`open` is the only constructor**, and a declaration cannot make one. Opening needs a
  path and somewhere to report failure, and `satellite.variable.file f` offers neither, so
  it is `nil` — **verified** — for the same reason §14 makes a spacesuit-typed field `nil`:
  a reference may name nothing.
- **A failed open is a value, not an error.** The handle comes back holding `errno` and the
  caller asks `.ok()`. Failing at the call site instead would make "does this file exist"
  unanswerable without killing the program that asked, which is the one question a script
  most often has.

Modes are the strings `"read"`, `"write"` and `"append"` rather than flags, because the
language has no enum and wants none: the three words say at the call site what a bitmask
never does.

**§10's two Library fixes are no longer hypothetical.** They were filed as harmless "before
resources land", and a file is a resource: a top-level `satellite.variable.file` declaration
writes the handle into `satellite.library`, and reassigning that variable now destroys the
old handle — `close(fd)`, which can block for seconds on NFS or FUSE — while the write lock
is held. Neither fix has been made.

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

### 8.7 `.size()` is bytes, `.length()` is items

Two questions had been sharing one word. "How many characters is this string" and "how much
memory does this string cost" are different questions with different answers, and until now the
language could only ask the first. `.length()` keeps that meaning everywhere it already had it,
and `.size()` is the second question, on every receiver that has methods at all.

    "abc".length()     3      characters
    "abc".size()       46     bytes

The gap between 3 and 46 is the whole point of having both: a value is a node plus its content,
and a program deciding whether to hold a million of something needs the second number.

**A number answers neither `length` the way a container does.** `.length()` on a
satellite.variable.number is an **error**, and the error names `.digits()` and `.size()`:

    satellite.variable.number has no method length — length() counts the items in a
    container and a number holds none; use .digits() for its decimal digits, or
    .size() for its bytes

Giving `length` a second meaning on numbers was the cheaper change and the wrong one. §17's
registry had already settled the principle when it minted `has` as a new word rather than
reusing `contains`: a word means one thing, and dispatch on the receiver is for types that
answer the *same* question differently, not for types that answer different questions. So
`digits` is its own word (86), and it counts the digits the value is **written** with, from its
most significant down to its least: `100` has three, `1230` has four, `12.5` has three, and `0`
has one because "0" is a digit. It is deliberately *not* the significand's count. Storage
normalizes `100` to `1e2` because trailing zeros are representation and not value — that is what
keeps `2.50` and `2.5` printing the same — but the number a program asks about still has three
digits, and a `.digits()` that answered `1` was reporting the representation instead of the
number. Leading zeros are padding rather than digits, so `0.001` has one.

#### The model: three numbers, and what is deliberately left out

A value costs a **node** of 40 bytes, plus a 16-byte **handle** for every slot that points at
another value, plus its **content**:

| receiver | content beyond the node |
| --- | --- |
| `bool` | none — it lives in the node |
| `number` | none in the small form; 4 bytes per limb once the magnitude is boxed |
| `string` | 2 bytes per character (§8.5) |
| `time` | none — eight bytes of nanoseconds, in the node |
| `file` | the path's bytes; the file description is the kernel's, not the program's (§8.3) |
| `list` | a handle per element, plus each element |
| `map` | two handles per entry, plus each key and value, plus the canonical key bytes and one slot each in the side index (§8.6) |
| spacesuit instance | a handle per field, plus each field |

What the model does **not** count is the allocator: `make_shared` control blocks, `std::vector`
capacity beyond its size, hash-table bucket arrays, `malloc`'s rounding. Those are real bytes,
and leaving them out is the deliberate half of the design. They are properties of the C++
runtime a build happened to use rather than of the program's data, and the packages are
`Architecture: any` — a `.size()` that moved with the pointer width would make any program that
prints one unportable and any test that asserts one unfalsifiable on half the build matrix. So
the three constants are the 64-bit layout **frozen as the definition**, the same move
`sizeof(Value) == 40` already makes, and a `static_assert` in value.hpp keeps them from drifting
from the layout they were derived from.

The consequence worth stating: **`.size()` is the language's model of memory, not the
allocator's.** It is exact about what the value model owns and silent about what sits underneath
it. A process that wants its real RSS should ask the operating system.

#### Shared storage counts once, and a cycle terminates

The walk carries a set of the storage it has already counted, and that one mechanism is load
bearing twice.

**Shared storage counts once.** A list holding the same sublist twice holds one sublist, and
`b = a` shares one buffer. The useful number is the bytes that would be freed if this value went
away, and that is not the sum of the parts:

    inner.size()       152
    outer.size()       224     two handles onto one 152-byte inner, not two inners

**A cycle terminates.** §12 records the price of making objects a reference type in one
sentence — an object *can* close a shared_ptr cycle, which no previous value could. `to_string()`
sidesteps that by refusing to print an object's fields at all; `.size()` has to walk them. An
honest and slightly startling consequence, from the run that verified it: a fresh `node` whose
`next` is nil costs **96**, and after `a.link(a)` it costs **56** — the nil was a real 40-byte
node, and the self-reference is storage already counted. The number is right; "smaller after you
add a link" is just what counting each distinct piece once looks like from the outside.

Depth is **not** capped, and that matches the precedent rather than overlooking it: `ValuePrinter`
recurses over nested lists with no cap either, so a structure deep enough to break the size walk
already breaks every print of the same value. The cycle — the one shape that would never
terminate at any depth — is handled.

#### Where it sits in the dispatch, and the one place it yields

`.size()` is tested **above** the per-type tables in `call_method`, not copied into each of them.
That is what keeps the answer consistent as the language grows: the walk in value.cpp is an
exhaustive `std::visit` with no `auto` fallback, so a tenth alternative in `ValueBase` fails to
compile until `.size()` has an answer for it, and no one has to remember to add a row.

The exception is a spacesuit instance, and it falls out of §14's existing rule rather than being
a new one. An instance answers for its own members **before** any built-in table is consulted, so
a spacesuit that already defines `size()` keeps its own and `.size()` never reaches the model
above. Adding this to the language therefore cannot change what any program that compiles today
does — the same guarantee that already let a spacesuit name a method `length` or `to_string`.

`nil` is unchanged: it has no methods, so it has no `.size()` either.
