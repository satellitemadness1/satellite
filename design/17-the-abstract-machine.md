*satellite design docs, §17 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§16](16-including-a-file.md), On: [§18](18-satellite-random.md).*

---

## 17. The abstract machine

Status: **specification, not implementation.** Nothing in the C++ implements any of this
yet. The tree walker of §10 is what runs today.

This section replaces §10's "no bytecode VM" for the *host*, and does not touch §15's
decision about what a compiler emits. Those were one entry in the DECIDED list wearing one
label, and they are two different questions:

- **What executes satellite today** — a tree walker, and §10 rejected bytecode for it. That
  rejection was made before anything was measured, which §14 says outright: *"a bytecode VM
  is what closes the rest, and §10's decision to stay a tree walker was made when nothing had
  been measured."* It is reversed here, on measurements recorded below.
- **What `satc` emits at the bootstrap** — C source, per §15, because bytecode needs a native
  VM and a native VM is the thing the bootstrap exists to delete. That reasoning is untouched
  and still holds. The machine specified here is an *abstract* machine: the C++ VM is one
  implementation of its transitions, and generated C is another. The specification outlives
  both, which is what lets §15 delete the C++ without deleting the semantics.

### What was measured, and what it rules out

Measured 2026-08-07 on a Xeon E5-2670 v3, against the tree walker, by differencing programs
that vary in exactly one dimension:

| measurement | result |
|---|---|
| one `+` | 112.5 ns, 1 heap allocation |
| of that, `make_shared` + atomic refcount | ~28 ns (25%) |
| of that, walk + operand loads + bignum | ~85 ns (75%) |
| one loop iteration | 600 ns, 5 allocations, 280 bytes |
| AST 488 B → 1 MB, operation count held constant | **5%** |

That last row is the one that decides the design. Growing the tree 2,000×, from trivially
L1-resident to spilling out of L2, changed per-operation cost by five percent. **Instruction
density is not the win**, because the same nodes are re-walked constantly and stay hot no
matter how large the program is. Anything justified by "better cache behaviour" is justified
by 5%.

What is left is the 75%: a recursive `eval()` call per node and a variant dispatch per node.
That is call overhead and branch misprediction, and it is what this machine exists to remove.

Two consequences follow immediately, and both cut against the obvious choices:

- **Not a stack machine.** A stack machine's cost is instruction *count* — `push`, `push`,
  `add`, `store` where a register machine dispatches once. Since dispatch is the expensive
  part and density is nearly free, more instructions is exactly the wrong direction.
- **A register machine is cheap to build here**, because §6's resolver already assigns every
  name a frame slot index at resolve time and already flattens spacesuit field layouts and
  method tables. Register allocation is largely done. The compiler walks a *resolved* tree.

And one that is not about speed at all: the 25% is a heap allocation per arithmetic result,
and no dispatch strategy touches it. A VM that keeps `shared_ptr<const Value>` per
intermediate mallocs to add two integers. The value representation is part of this work, not
a follow-up — see "The value model" below.

### The generating rule is what makes the encoding possible

§1 says a dotted path rooted at `satellite` names something the language owns, and a bare
identifier names something the user owns. That is a statement about *closure*: the set of
language-owned names is finite and known at compile time, so it can be enumerated and given
integer identities. Bare identifiers are never in that space at all — they are frame slots
already.

The invariant that made the parser backtrack-free (§4) is the same one that makes the opcode
space enumerable. This is not a coincidence to enjoy; it is the reason the encoding below can
be a fixed table rather than a hash lookup.

### The unit: four 64-bit words

Every language-owned name is **256 bits, as four 64-bit words** — one word per path segment,
zero for absent segments.

```
word 0   bits [63:60]  kind
         bits [59:0]   segment 1 id      (`satellite` is 1)
word 1                 segment 2 id      (0 if absent)
word 2                 segment 3 id      (0 if absent)
word 3                 segment 4 id      (0 if absent)
```

`satellite.console.display` is `{1, 6, 7, 0}`. `satellite.capsule` is `{1, 3, 0, 0}`.

256 bits is far more than the information requires, and that is a deliberate trade, taken on
the measurement above: density is worth 5%, so paying four words for a decode with no
shifting, no masking of packed sub-fields and no variable-length instructions is a good
trade at this exact ratio. It also leaves room to add segments without a format break, which
a packed encoding would not.

**Kind** occupies the top four bits of word 0. What a kind selects is **which id space the
rest of the unit is read against**. The assignment lives in `format.def` as the `SAT_KIND`
list and nowhere else; this table **reproduces** it, and if the two ever disagree the one that
compiles is right:

| kind | space the id is read against |
|---|---|
| 0 | the frozen name registry below — an instruction |
| 1 | none; the unit is an immediate operand |
| 2 | the constant pool — an index |
| 3 | the register file — an index |
| 4 | this file's type table — a spacesuit id (§17.2) |
| 5 | this file's capsule table — a capsule id (§17.2) |
| 6 | the machine-op table — `SAT_OP`, a separate frozen space (§17.5) |

Kinds 7–15 are unassigned, and 15 is the ceiling a four-bit field allows —
`format.hpp` asserts that no kind id exceeds it. **§17.2 and §17.5 must not restate this
table**, and the reason is that they once did: each extended a four-row table on its own, and
both landed on kind 4 — spacesuit id in one section, machine op in the other, colliding on the
same four bits. Nothing had been encoded, so it cost a renumber instead of a format break.
Machine ops moved to 6 because §17.2's assignment was written first, and the rule this
document applies to ids applies to kinds for the same reason: the earlier assignment stands.

A markdown table is why that was possible at all. Add a kind by adding a `SAT_KIND` row, and
copy it here afterwards if it helps a reader — never the other way round.

Kind is **not** what makes the stream decodable — see "Decoding" below. It is a check on the
decoder's own state, and it is what lets a disassembler walk a stream cold and a malformed
file fail at the first mismatch instead of executing garbage. At 253 spare bits it costs
nothing.

### Decoding is positional, and the arity table is the grammar

Nothing in the stream announces "I am an instruction." The reader starts at a known state —
an instruction boundary — and every name code's entry in the **arity table** says how many
operand units follow it. Consuming them returns the reader to an instruction boundary. This
is the same reason machine code needs no per-byte marker: position is the tag.

The arity table is therefore not an implementation detail, it is the grammar of the format,
and it must live in this document beside the registry. A name code whose arity is unrecorded
is a stream that cannot be decoded.

One path in the table has **more than one shape**, and the table says so rather than picking
one. `SAT_VARIADIC` — 255, the arity column's only non-count value — means the next unit is a
kind-1 immediate holding the operand count, and that many operand units follow it. Decoding
stays positional throughout: the reader knows the path, the path says a count is next, the
count says how many follow. What it stops needing is for that number to have been the same in
every program ever compiled. This is the same answer §17.5 gives for methods, where
`CALL_METHOD` already carries its argument count for exactly this reason.

The sentinel is 255 and not 0 or −1 because **0 is a real arity**. `satellite.help` was
recorded as 0 for three commits, meaning "the overview, and the other form cannot be written
at all" while reading as "takes no operands", and a sentinel that collides with a legitimate
value is indistinguishable from one. `format.hpp` asserts `kVariadic != 0` so that the
choice cannot quietly be undone.

### The registry, and why order of appearance is forbidden

Ids are **frozen in this table**, not assigned in order of first appearance. Assignment by
appearance means adding one construct renumbers every construct after it, and every
previously compiled file keeps its bits while changing its meaning — the worst failure a
binary format has. New names take the next free id, forever; nothing is ever renumbered, and
nothing is ever reused.

The space is **flat**: a name has one id wherever it appears. `console` is 6 in every
position. The alternative — numbering each segment position separately — packs tighter, and
at 64 bits per segment there is nothing to gain by packing. Flat means one table, and an id
that is unique in a hex dump.

| id | name | | id | name |
|---|---|---|---|---|
| 1 | `satellite` | | 21 | `bool` |
| 2 | `include` | | 22 | `number` |
| 3 | `capsule` | | 23 | `string` |
| 4 | `main` | | 24 | `time` |
| 5 | `return` | | 25 | `file` |
| 6 | `console` | | 26 | `container` |
| 7 | `display` | | 27 | `list` |
| 8 | `returns` | | 28 | `library` |
| 9 | `spacesuit` | | 29 | `help` |
| 10 | `protected` | | 30 | `now` |
| 11 | `public` | | 31 | `open` |
| 12 | `statement` | | 32 | `directory` |
| 13 | `if` | | 33 | `current` |
| 14 | `else` | | 34 | `change` |
| 15 | `while` | | 35 | `exists` |
| 16 | `for` | | 36 | `true` |
| 17 | `variable` | | 37 | `false` |
| 18–20 | *reserved* | | 38+ | methods, below |

Ids 18–20 are held open deliberately: `break` and `continue` are the two gaps §15 ranks
first, and reserving them now costs nothing and keeps them low.

Methods (`plus`, `minus`, `times`, `divided_by`, `modulo`, `abs`, `floor`, `ceil`, `round`,
`to_string`, `length`, `concat`, `contains`, `starts_with`, `ends_with`, `first`, `last`,
`append`, `empty`, `and`, `or`, `negate`, `ok`, `read`, `write`, `close`, `error`, `path`,
`nanoseconds`) take ids from 38 upward in the order listed, and the table above is completed
in the commit that writes the compiler, not before — writing ids down is cheap, and writing
them down *wrong* is permanent.

Comparison took 67..72 (`equals`, `not_equals`, `less_than`, `less_or_equal`, `greater_than`,
`greater_or_equal`) — see §17.5 for why an operator needs a *selector* and not an opcode.

§8.6's map took 73..79: `map`, `get`, `set`, `remove`, `has`, `keys`, `values`. Next free is
**80**. Three things about that block are precedent rather than detail:

- `map` is a **type name** and belongs beside `list` (27), but it is 73 because ids are never
  renumbered. It is the first entry whose semantic group and numeric position disagree.
  Grouping is a comment; numbering is the format, and when they conflict the numbering wins.
- `length` (48) and `to_string` (47) are **reused, not duplicated**. The space is flat, so a
  word has one id wherever it appears, and a map answering to `length` is the same word.
- `has` is a **new** word rather than a reuse of `contains` (50), which is substring
  containment on a string and element containment on a list. On a map it would be ambiguous
  between keys and values, and a name that has to be disambiguated by its receiver is a name
  that will be read wrong.

The map needs no `SAT_PATH` row and no machine op. Its selectors are dispatched by
`CALL_METHOD`, which carries an explicit argument count, and `m[k]` compiles to the `get`
selector exactly as `+` compiles to `plus`. It needs nothing from the constant pool either,
because the language has no container literals — a map is only ever built by `.set()`, so
there is no map literal to encode. That is recorded here so nobody later assumes it was
overlooked.

This registry replaces **106** runtime string comparisons across `src/evaluator/` — 34 in
`methods.cpp`, 18 in `operators.cpp`, 13 in `helpers.cpp`, 11 in `modules.cpp` and the rest
scattered over seven more files — of which the module ones compare *whole dotted paths*
(`full == "satellite.directory.current"`, src/evaluator/modules.cpp:105). Removing them is not a
density argument; it is work deleted per call.

That figure was 88 when this paragraph was written, against an `eval.cpp` that no longer
exists — 059a9d8 split it into twelve files. The number is quoted here because it is worth
knowing; it is recounted by `grep -c '== "' src/evaluator/*.cpp`, which is the only reason it is right.

### format.def is the registry, and this document is not

Everything above is duplicated as data in `format.def`, and the duplication has a direction:
**prose may explain a number, but it may never be the only place the number lives.** The test
for which half a fact belongs in is whether anything breaks if it is wrong. If yes, it has to
be compilable.

This is not a preference. It is what four defects in this very section cost:

- §17.2 and §17.5 each extended a four-row kind table without reading the other, and both
  assigned **kind 4** — a spacesuit id in one section, a machine op in the other.
- Id **56** (`empty`) was frozen for a selector that no receiver implements and none ever did.
- §17.5 froze the builtin selectors as the literal range **38..72**, one commit before §8.6's
  map added 73..79 — after which the range silently classified every map selector as a user
  capsule with a frame.
- The comparison count above, and four file:line citations, named two files that no longer
  exist: `eval.cpp`, split away in 059a9d8, and `env.cpp`, in 4e8171e.

None of those is hard. All four are one defect: a number that lived only here. `format.def`
had no consumer at all — nothing included it, and it was not in the Makefile's `HDRS` — so its
own promise that everything "comes from ONE list and cannot drift apart" described a property
nothing checked.

`format.hpp` is that consumer, and `format_test` the proof. The lists expand into enums, into
switches where a duplicate id is `error: duplicate case value` naming both culprits, and into
`static_assert`s over the ids: distinct, ascending, dense but for the reserved 18..20, every
path segment a defined word, every path left-packed, every selector a real word. The asserts
live in the header rather than the test, so every future consumer — the compiler, the loader,
the disassembler, §15's generated C — inherits them. Eighteen deliberate mutations were tried
against it, the kind-4 collision among them: fourteen fail the build and four fail the test.

An adversarial review of that first attempt found four invariants it had claimed and not
checked, of which one is the lesson. The four-bit kind ceiling was asserted as
`sizeof(kKindIds) / sizeof(uint64_t) <= 16` — which counts *rows*, not values, so
`SAT_KIND(99, ...)` compiled clean and passed. A guard written as arithmetic on a `sizeof`
reads exactly like a guard on the thing it is named after, which is the failure mode a
mutation test exists to catch and prose review does not.

ast.hpp:50-54 got there first and said it in one line: *a printed number that nobody compares
is not a budget.*

### The container

```
unit 0   [256 bits]   magic 0x547311173, zero-padded
unit 1   [256 bits]   counts:
           word 0     format version — bump when the registry changes
           word 1     flags; carries a known byte pattern that reveals byte order
           word 2     code_units   — 256-bit units in the code section
           word 3     const_count  — entries in the constant pool
unit 2..              code section
                      constant pool
                      span table
```

The endianness marker is not ceremony: both Debian binary packages are `Architecture: any`
(§9, debian/README.packaging), and a 64-bit word written on x86-64 reads byte-reversed on a
big-endian host. Catch it in the header or debug it in the interpreter.

### The value model

The 25% that dispatch cannot touch. Today `ValuePtr` is `shared_ptr<const Value>`
(value.hpp:27) and every intermediate result is a `make_shared` — a malloc plus an *atomic*
refcount, for adding two integers.

A value must be **inline for the common case and heap only when it has to be**: small exact
integers, `bool`, and nil never allocate; a `Number` promotes to the base-10⁹ bignum
(bignum.hpp) only when an operation actually leaves the inline range, and demotes when it
fits again. §8.1's guarantee is untouched by this — the type is still one exact
arbitrary-precision decimal, and the promotion is a representation detail below the
specification line, which is precisely the property "build abstractly" is for. There is still
no float, and this is not the machine-double fast path §8.1 forbids: it is one number type
with two representations of the same exact value, not two answers.

This is worth stating plainly because the abandoned v001 had exactly this value model — a
16-byte tagged union where nil, bool and integer never allocated, promoting on overflow — and
it was the sound half of a design that failed for unrelated reasons (§17.1). Taking it back
is not a reversal; the reason 001 died was linking a C++ compiler into the process, not its
value representation.

Order matters here. A VM built on the current `ValuePtr` would remove the 75% and keep the
25%, land at roughly 1.6×, and read as evidence that bytecode was oversold. The value model
is part of milestone 1, not a follow-up.

### What is not decided

Honestly, and per §16's example of marking placeholders as placeholders. Two entries that
stood here have been decided and moved to §17.4; a third — how a path with two arities is
encoded — is decided and implemented, and is kept below with its answer rather than deleted,
because the reasoning is the reason the sentinel is 255. Two remain genuinely open.

- **The span table shape.** Per-instruction is the obvious form and costs one entry for every
  instruction, most of them repeating the line before. A change-only table — one entry
  wherever the line changes, found by binary search — is typically 5 to 10 times smaller, and
  the search costs nothing because this table is read **only when reporting an error** and
  never during execution. Change-only is the recommendation and is not yet written down as a
  decision.

  Whichever it is, it must carry a line number **and** the file id §16 adds to `Span`. An
  error that names the wrong file is the failure §16 calls fatal.

- **The method-id assignments.** `format.def` carries selector ids from four sources: 38..66
  from the method dispatch in src/evaluator/methods.cpp and src/evaluator/mutators.cpp, 67..72 minted by §17.5
  as lowering targets for the comparison operators and implemented nowhere yet, 73..79
  added with §8.6's map — 73 being the type name rather than a selector — and 85..86 added with
  §8.7's `size` and `digits`, which sit above the random block (80..84) and are the reason the
  selectors stopped being one contiguous range. They are frozen the
  moment a file is compiled with them and not before, so this is the last cheap moment to
  renumber. One is already known to be wrong and
  is being kept anyway: 56 (`empty`) names a selector no receiver implements, and the id stays
  spent rather than reused, because reusing a number that has appeared in a published table is
  how a format learns to lie. It is simply absent from the `SAT_SELECTOR` list, so nothing can
  emit a call to it.

- **How a language path with two arities is encoded.** Decided, and implemented — see
  "Decoding is positional" above. `satellite.help` takes zero arguments (the overview) or one
  (help for a thing), and `format.def` recorded arity 0, which encoded the overview and
  silently lost the other form. The answer is a variable-arity marker, `SAT_VARIADIC`, whose
  next unit is a kind-1 immediate holding the count; the reader still always knows what comes
  next, so decoding stays positional. It is general, which was the requirement — it was the
  first path with two arities and it will not be the last.

Settled, and recorded here because it keeps being asked:

- **SIMD.** Rejected for the dispatch loop. Decode is serial and data-dependent — each step
  waits on the one before — which is the opposite shape to what SIMD accelerates, and
  `Architecture: any` means i386 and armhf have no intrinsics, so it could never be
  load-bearing. The real opportunity is bignum's base-10⁹ limb arrays under add and multiply,
  which is genuinely SIMD-shaped and has nothing to do with this machine.

- **The register ceiling.** A non-issue, and the 1024 that stood here was a leftover from a
  packed encoding this format did not adopt. A register index has a whole 64-bit word, so the
  ceiling is 2^64. The real bound is the exact slot count §6 already computes per capsule.
  What survives from that entry is the warning in §17.4: a fixed per-frame allocation would
  reintroduce the per-activation `vector` that is part of the measured 600 ns.

### Cost model

Every instruction must state roughly what it compiles to, and any instruction that cannot be
implemented in bounded work is pitched at the wrong level.

This rule is here because of §17.1. Abstraction without a cost model is what killed v001:
`satellite.cxx { }` was a clean abstraction whose implementation was "link clang into the
process," and the bill was a 107 MB binary and ~900 ms per run. An abstract machine is a
specification of *transitions*, not a licence to specify transitions nobody can afford.

### 17.1 What v001 was, and what it cost

Preserved at the tag `v001-llvm-abandoned`. It had no parser and no virtual machine: a `.satl`
file was lexed and validated, and only its `satellite.cxx { }` blocks executed. Both engines
were clang — one linked into the process (~900 ms per run, no cache), one shelling out to g++
and `dlopen`ing the result. The binary was ~107 MB by design.

It is kept because two of the decisions in this document are reactions to it: §15's rule that
a compiler emits C source so that no compiler of ours survives in the shipped artifact, and
the cost model above. Its value representation was right, and this section takes it back.

### 17.2 User-defined names, and encoding a spacesuit

The registry of §17 is language-owned and therefore closed, which is the property that let it
be a frozen table. User-defined names are the other half of §1's rule: open, unbounded, and
different in every program. They cannot share that space and do not need to.

**User names live in per-file tables**, and the kind tag on word 0 says which space an id is
read against: **kind 4** is a spacesuit id and **kind 5** a capsule id, both indices into this
file's tables rather than into the registry, which stays kind 0 and stays the same in every
file. §17's kind table is where those are assigned and is the only place they are; this
section names them and does not restate the table, because restating it is exactly how kind 4
came to mean two things at once.

The type table is counted in the header the same way the constant pool is. The id field is 60
bits, so the format's ceiling on distinct spacesuits is 2^60; the real limit is the per-file
count, and neither is a constraint any program will meet.

**A spacesuit compiles to one table entry plus integers at every use site.** §14 already
flattens at resolve time, so the compiler is serialising an answer rather than computing one:

```
type table entry
    name              metadata only — error messages, never dispatch
    superclass id      or none
    field_count        an instance allocates exactly this many slots
    method_slots[]     flattened, one code offset per slot
    ctor_offsets[]     superclass first, most derived last (§14)
```

Nothing at a use site names a class. A field read is an immediate index, because `SLOT_FIELD`
(ast.hpp:135) already *is* that index and `find_field` already returns it — "the index is what
an Object is addressed by" is env.hpp's own phrasing, and it is a description of the bytecode
before there was any. A method call is a slot index. Construction is a type index. The class
name survives only so an error can say which class.

**The one thing resolve() does not do yet: assign method slots.** Fields already have the
property that makes integer addressing work —

> Superclass fields FIRST, then this suit's own ... prefixing means those indices still name
> the same fields in a descendant's instance.

— and methods do not, because `SpacesuitInfo::methods` is an `unordered_map` keyed by name. A
hash has no ordering, so there is no index to emit. Methods need the same discipline the
fields already have, which is the ordinary vtable rule:

- a superclass's slots keep their indices in every descendant
- an override writes into the slot it overrides
- a method the subclass introduces appends

That is a new pass in resolve(), and it is the only part of the object model this machine has
to invent rather than serialise. It is also what turns dispatch from a hash lookup on the
object's suit into one array index — the win §14 was already reaching for when it flattened
the tables in the first place.

Constructor ordering is unchanged and must stay unchanged: every field initialiser runs before
any constructor body, then constructors run superclass-first, and the construction site's
arguments go to the most derived one. §14 records why that is deliberately not C++'s
interleaving, and the bytecode has no opinion — it emits the order resolve() already fixed.

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
