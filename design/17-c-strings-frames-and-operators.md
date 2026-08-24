*satellite design docs, §17, part 3 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§17 part 2](17-b-the-container-and-the-value-model.md), On: [§18](18-satellite-random.md).*

---

## 17. The abstract machine — strings, frames and operators

*Continues [§17 part 2](17-b-the-container-and-the-value-model.md).*

### 17.3 Strings, lengths and the constant pool

`SatChar` is `char16_t` (satellite_string.hpp:31): **16 bits per character**, over satellite's
own code table rather than Unicode or ASCII. `"hello, world!"` is the codes
`8 5 12 12 15 87 32800 23 15 18 12 4 63` — `a` is 1 so `h` is 8, `,` is punctuation base
63 plus index 24, and the space is `0x8020` because §3.2 puts whitespace in the raw area.
The most common character in English prose takes the raw-area path; that is by design, and it
is why a character cannot be 8 bits: the table's 101 codes plus a 256-code raw area is 357,
and 357 does not fit in a byte. §8.5 carries that argument in full.

A string's length is unbounded, so a literal cannot be an inline operand at any fixed unit
size. It lives in the **constant pool** and the instruction carries a kind-2 operand holding a
pool index.

**Lengths carry a continuation bit.** A length is one 256-bit unit whose top bit says whether
another unit follows, leaving 255 bits of value; a set bit means the length continues.

255 bits will never overflow for a string — 2^255 characters is past the number of atoms
available to store them — so for a length the continuation bit is a formality that is always
zero. It is specified anyway, and the reason is `Number`: an exact arbitrary-precision
significand (§8.1) has no bound by definition, the continuation genuinely fires there, and no
fixed width could replace it. The varint reader has to exist regardless, so using it for every
length as well costs nothing and leaves the format with **one reader for every length and
every number in it**.

The cost is nothing for a reason specific to where it lives: the constant pool is decoded
**once at load**, not per instruction, so a loop there never touches the dispatch path. That
gives the rule the format is organised around:

> **Fixed-width units in the code section, variable-width in the constant pool.**

Instructions stay 4x64 so positional decoding (§17) stays a load rather than a loop.
Unbounded things — lengths, bignum significands — live in the pool where the loop is amortised
to zero.

There is a consistency argument underneath the engineering one, and it is the stronger of the
two. §8.1 banished fixed-width numbers from the *language*. A 64-bit cap in its *bytecode*
would be smuggling one back in through the basement.

**Type, continuation and length share one unit.** A whole 256-bit unit spent carrying the
number 13 is not thrift, it is theatre, and there is no reason the type tag cannot sit beside
it:

```
unit 0    word0 bits[63:60]   entry type   (string, number, ...)
          word0 bit  [59]     continuation — another length unit follows
          word0 bits[58:0]
          word1, word2, word3 length, 251 bits
unit 1..  data                16 characters per unit, last unit zero-padded
```

251 bits is the same unreachable number 255 was, and the entry is one unit shorter. The saving
is 32 bytes per pool entry and therefore does not matter; the reason to do it is that a field
sized 20 orders of magnitude past its largest possible value invites the reader to wonder what
they have misunderstood.

The continuation bit keeps its meaning: set, and another full 256-bit unit of length follows.
For a string it will never be set. For a `Number` significand it will.

`satellite.console.display("hello, world!")` in full — two units of code, two of pool. At 16
bits a 13-character literal fits in a single data unit where it needed two before, which is
§8.5 showing up in the instruction stream and not only in the heap:

```
CODE
  unit 0   name code
    word0  0x0000000000000001    kind 0 (name) | segment 1 = satellite
    word1  0x0000000000000006    segment 2 = console
    word2  0x0000000000000007    segment 3 = display
    word3  0x0000000000000000    segment 4 absent
  unit 1   operand
    word0  0x2000000000000000    kind 2 (pool ref) | index 0
    word1..3  zero

POOL entry 0
  unit 0   type = string, continuation clear, length = 13
    word0  0x100000000000000D    type 1 | cont 0 | length 13
    word1..3  zero
  unit 1   0x 0008 0005 000C 000C 000F 0057 8020 0017 000F 0012 000C 0004 003F 0000 0000 0000
                h    e    l    l    o    ,   (sp)  w    o    r    l    d    !    -    -    -
```

Two things that are correctness rather than encoding:

- Store `StringLit::value`, not `StringLit::source` (ast.hpp:113). `value` has escapes already
  expanded; `source` is what was typed.
- This is the one place `encode` applies. §3.3 fixes the rule: source text is lexed with
  `encode_raw` (one byte, one SatChar) so spans stay byte offsets, and only string literal
  *bodies* are `encode`d. src/evaluator/methods.cpp:248 records that getting this backwards has been found
  three times, most recently in `.read()`.

Exact-decimal numbers take the same route for the same reason, and are where the continuation
bit is load-bearing rather than ceremonial. Small integers that fit the inline value
representation are the exception and are emitted as kind-1 immediates, which is where §17's
value model pays off in the instruction stream as well as in the heap.

### 17.4 Calling convention: registers are slots, frames are windows

This section corrects an earlier recommendation in this document's own history. One operand
unit per argument was recommended on the grounds that it matches the arity table with no
special case. That reasoning weighed the *encoding* and never weighed the *frame setup*, which
is the part that runs. The conclusion was wrong and is reversed here.

**A register is not an object and is never created.** One array of `Value` is allocated once,
at VM startup, and a register is a slot in it. Register 5 of the current frame is `base[5]`,
where `base` is a pointer into that array. There is no allocation on the path, because there
is nothing to allocate — the slot was already there.

This is the whole of the mechanism, and stating it negatively is worth as much: a design where
each register is a separately allocated C++ object would put a malloc back on every value and
land exactly where the tree walker already is. The tree walker is slow *because* every value
is an individually allocated `shared_ptr<const Value>`. Registers as objects is that same
mistake under a new name.

**A frame is a window, not an allocation.**

```
value stack   [ main's 8 slots ][ fact's 5 slots ][ fact's 5 slots ][ unused
               base = 0          base = 8          base = 13
```

Calling advances `base`; returning restores it. Two pointer adjustments, no allocator. That
deletes the second of the three costs §14 names — "a `Frame` vector per activation" — outright.

**Arguments are contiguous, and that is what makes a call free.** The caller evaluates each
argument into the next slot at the top of its own frame. `CALL` names the first argument
register and the count. The callee's frame base is then set **to that first argument
register**, so the arguments are already sitting where the callee expects slots 0..N-1 to be.

Nothing is copied. Not one value.

With scattered per-argument operands the VM must copy N values into a fresh frame on every
call. With a contiguous range it copies zero. That is the entire argument, and it is why Lua
— the fastest register VM in wide use — encodes calls this way.

The price is paid in the compiler: it must evaluate call arguments into consecutive registers
rather than wherever is convenient. That is real work, it is well understood, and it is the
right direction — complexity in the thing that runs once, to make free the thing that runs
constantly.

**Arguments carry no names and no types.** This is worth saying because a calling convention
looks like it should need both. §6's resolve() already turned every parameter name into a slot
index, so the name exists only for error messages. And a `Value` carries its own tag, so an
argument describes its own type rather than being described by the call. An argument at run
time is a value in a slot, and nothing else.

Arity is checked statically, but **only for a call whose target is a bare name** — a top-level
capsule, an enclosing suit's method, or a spacesuit constructor (src/environment/names.cpp:63-113). A call
through a receiver, `obj.foo(1, 2)`, has a `Member` target, falls through src/environment/names.cpp:118
unexamined, and is caught at run time instead (src/evaluator/calls.cpp:77-83). This paragraph claimed
the check was universal and cited `env.cpp:251`, a file deleted in 4e8171e. The correction
matters to the machine and not only to the prose: `CALL` carries an argument count that the
compiler cannot always have verified, so the VM keeps the run-time check rather than trusting
the stream.

**What C++ owns, and what it does not.** `Value` is an ordinary C++ struct with a copy
constructor, a move constructor and a destructor, and those do the fiddly work: releasing a
string's buffer or decrementing an object's refcount when a register is overwritten.
`base[5] = base[6]` is a 16-byte move for an integer and one refcount bump for a string, and
the correctness is the compiler's problem rather than the VM author's.

What C++ must **not** own is register lifetime. `Value` is the element type of an array, never
a thing that is individually allocated.

**The stack is fixed at startup, and the guard already exists.** A growing `std::vector` would
reallocate and invalidate every frame base being held — a use-after-free that surfaces as
inexplicable garbage. Fix the size up front and report exhaustion. That is not a new limit:
src/evaluator/helpers.cpp already carries a measured depth guard, `DEFAULT_MAX_DEPTH = 2000` with one
activation costing exactly 3 units. The bounded register stack is the same guarantee the
language already makes, expressed in the new machine.

**One caveat this paragraph acquired later.** The guard's *ceiling* was a fixed 3000 when the
sizing below was chosen, and it is now derived from `RLIMIT_STACK`; the calibration and the
measurement behind it are in src/evaluator/helpers.cpp, above `max_max_depth()`. It is still 3000 on the ordinary 8 MB stack, so nothing here is wrong today, but
on a raised `ulimit -s` the walker accepts far deeper recursion than 3000 frames of registers
would hold: ~24,000 units on 64 MB, ~24,577,781 on 64 GB. A VM built on this stack as sized
would refuse programs the tree walker runs, which inverts the rule the sizing exists to serve.
Sizing from `max_max_depth()` at startup is the obvious answer and costs nothing, since the
allocation is an mmap whose untouched pages never become resident — but the choice belongs to
whoever builds the VM, and nothing includes `reg.hpp` yet.

**Built: `reg.hpp` and `reg_test`.** The register type and the stack exist; the VM does not,
and nothing in the interpreter includes the header. Four things it settled that were open here.

*The register type is new, and `Value` is untouched.* A `Reg` is a five-state tagged slot —
empty, nil, bool, small decimal, heap — at **32 bytes**, holding an ordinary `ValuePtr` for the
heap case. That is the hybrid a design pass measured at 15.38 against 15.91 ns/iter for a full
tagged union, a wash, which buys the speed without reopening the lock-free publish protocol
ThreadSanitizer verifies. §17's value-model paragraph reads as though `Value` itself goes
inline; it is the *register slot* that does.

*Five states, not four.* A slot distinguishes "nothing written here yet" from "holds nil",
because `read_slot` reports "is read before its declaration runs" by testing exactly that, and
can only do so today because `ValuePtr` has a null state no `Value` occupies. `EMPTY` is tag 0
so the rule is written once.

*The inline path allocates nothing, counted rather than assumed.* 100,000 inline additions:
**0 allocations**. The same additions through `Number` and a `ValuePtr`, which is what the tree
walker does: **100,000**. `add_inline` declines rather than being wrong — including when
*aligning* two exponents overflows before a digit is added, which `1 + 1e-30` does and a scheme
checking only the addition gets wrong. Fuzzed at 200,000 random pairs: 132,727 stayed inline,
every one exact against `Number::add`.

*The stack is 3000 activations × 64 slots — and it is built lazily.* Sizing it from the depth
guarantee rather than from a cache figure follows §17's own 5% measurement. But `new
Reg[192000]` runs a constructor per slot, because a `Reg` holds a `ValuePtr` and is therefore
not trivially constructible, and that writes all 5.86 MB: **measured at 3.2 ms, against a satl
startup of about 2.5 ms.** It would more than double the cost of hello world to prepare 3000
frames for a program that uses four. The allocation itself is nearly free — a request that size
is an mmap and untouched pages never become resident — so the cost was entirely in constructing
slots nobody asked for. Slots are now constructed as the stack grows into them: **0.011 ms to
build, 32 slots constructed to run four frames deep**, a 290× improvement, with the array still
never moving. Verified under both AddressSanitizer and UndefinedBehaviorSanitizer, since raw
storage plus placement new is not something to take on trust.

**Expected result — an estimate, not a measurement.** Both allocation sources in the measured
600 ns per iteration are removed by this section and by §17's value model: no malloc per
arithmetic result, no allocation per activation. What remains is dispatch, which the register
encoding cuts from four instructions per operation to one. The projection is roughly 2.5x to
3x overall, which would put satellite at or slightly past CPython on execution while keeping
the 6.4x startup advantage that is already measured.

That number is a projection and is labelled as one deliberately. Everything it rests on was
measured; how much comes back was not.

### 17.5 Operators are selectors, and the machine ops are not names

Two questions that look like one. "How does `+` become bytecode?" and "where do `MOVE` and
`JUMP` live?" have opposite answers, and the difference is §1.

**An operator is surface syntax for a selector.** `a + b` and `a.plus(b)` are the same
operation spelled two ways, so both compile to the same instruction and `+` needs no opcode:

```
x + 1      ->   CALL_METHOD  recv=r1, selector=38 (plus), args=r2, count=1, dst=r3
x.plus(1)  ->   CALL_METHOD  recv=r1, selector=38 (plus), args=r2, count=1, dst=r3
```

This is §7's collapse one step further out. §7 verified that a method is sugar for a module
function over one table; an operator is sugar for a method over that same table. The result is
one dispatch mechanism where a machine of this shape usually has three, and it is why the
arithmetic opcodes every other bytecode has are absent here: `plus` (38), `minus` (39),
`times` (40), `divided_by` (41) and `modulo` (42) were already in the registry, and the six
comparisons were added at 67..72 to finish the set. Dispatch is on the receiver, which the
table already does — `minus` is both a number and a time method, `contains` both a string and
a list one.

It also deletes work rather than adding it. `+` currently takes a path entirely separate from
`.plus()`: ast.hpp:164 stores the operator as a `std::string` and src/evaluator/operators.cpp compares
it against `"+"`, `"-"`, `"*"` on every arithmetic operation. Those are 18 of the 106 runtime
string comparisons §17 opened by promising to remove.

When the selector is a builtin `CALL_METHOD` builds **no frame**. It is a table jump
to native code taking register indices, so `+` stays one dispatch and one write. §17.4's frame
machinery is for user capsules, which are the only things that have frames.

**Which selectors are builtin is a list, not a range.** This section said "38..72" until the
map landed at 73..79 one commit later, at which point the range began classifying every map
selector as a user capsule with a frame — a wrong answer at run time, produced by prose that
had gone stale. The set was never contiguous in the first place: 56 (`empty`) answers to no
receiver, and 73 (`map`) is a type name. So the set lives in `format.def` as `SAT_SELECTOR`
rows and is read through `format::is_selector()`, which is derived from the list and therefore
cannot go stale. There are **40** of them today.

Each row names a word by its *identifier* rather than its number, and that is the technique
rather than the detail: `SAT_SELECTOR(PLUS)` does not compile if `PLUS` is not a `SAT_WORD`.
A cross-reference the compiler checks is the only kind that survives a year.

Six of the forty are declared by the format and implemented by nothing: `equals` (67) through
`greater_or_equal` (72) have no named method anywhere in `src/evaluator/` — only the `==` `!=` `<` `<=`
`>` `>=` operators — because they were minted in 906a4a8 purely as lowering targets. That gap
is real work and not a naming detail, for a reason recorded in `format.def`: `==` is **total**,
defined over every pair of values including nil and mismatched types (src/evaluator/operators.cpp:213),
while receiver dispatch refuses a nil receiver outright (src/evaluator/methods.cpp:34). Lowering `==` to
a receiver-dispatched selector without giving that selector total semantics turns `x == nil`
from an answer into an error.

**A machine op is not a name**, and that is the whole reason it lives elsewhere. §1 says a
dotted path rooted at `satellite` names something the language owns, and the registry is
enumerable precisely because that set is closed. No satellite program can write `MOVE`. Adding
machine operations to the registry would spend the closure property that made a frozen table
possible, to save a tag value the format has nine spare copies of. So they take **kind 6** and
a separate id space, restarting at 1, and format.def carries them as `SAT_OP` beside
`SAT_KIND`, `SAT_WORD`, `SAT_SELECTOR` and `SAT_PATH`.

Kind 6 and not kind 4, which is what this section said until §17's kind table was made the
single authority: 4 was already a spacesuit id in §17.2, and two sections had each extended a
four-row table without looking at what the other had done. The id spaces stay separate, which
was always the point; only the tag moved.

Three consequences worth stating, because each answers a question that has been asked twice:

- **Methods need no arity entries, anywhere.** `CALL_METHOD` carries an explicit argument
  count, so a method's arity is read from the stream rather than looked up. The 42 selectors
  with ids and no `SAT_PATH` row are not an omission; a row would be unreachable, and
  `format_test` asserts that none of them has one.
- **Jump targets are absolute unit indices.** Not byte offsets, because units are fixed width
  and an index is then a subscript with no multiply; not relative, because an absolute target
  survives code moving around it while the compiler is still emitting.
- **`JUMP_IF_TRUE` exists for `||`.** §15 ranks `&&` and `||` third among the gaps stage 0
  found. When they land they cannot compile to the `and` (57) / `or` (58) selectors, because a
  call evaluates its arguments before it runs and short-circuiting must not. They compile to
  branches, which is why the format carries a branch of each polarity from the start.

**What has no operator, and will not get one.** `>>` and `<<` are settled by §3.5: satellite
has no shift operator *ever*, because `<` and `>` are always single-character tokens and that
is exactly what lets `list<list<string>>` parse with no maximal-munch special case. §3.5
already names the alternative — `satellite.number.shift_left(n)`, a path like everything else.
Bitwise `|` and `&` are a different refusal: §8.1 makes a number an exact arbitrary-precision
decimal, and a bit pattern is not defined for one. Both would need an integer domain the
language deliberately does not have.
