*satellite design docs, §17, part 2 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§17 part 1](17-a-the-abstract-machine.md), On: [§17 part 3](17-c-strings-frames-and-operators.md).*

---

## 17. The abstract machine — the container and the value model

*Continues [§17 part 1](17-a-the-abstract-machine.md).*

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
