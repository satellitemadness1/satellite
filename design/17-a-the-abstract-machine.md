*satellite design docs, §17, part 1 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§16](16-including-a-file.md), On: [§17 part 2](17-b-the-container-and-the-value-model.md).*

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
