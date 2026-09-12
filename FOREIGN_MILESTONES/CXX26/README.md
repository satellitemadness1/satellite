# C++26 → satellite, feature by feature

**The standard this folder is written against:**

> **C++26 — not yet published.** At the time of writing the working paper is in
> flight: some papers below are voted into the draft, some are expected, and the
> final feature set is not settled. **A row here is a guess about C++ and a
> decision about satellite** — the second half is binding, the first is not.

**Written 2026-09-12 against knowledge current to May 2026.** If a feature below
was dropped from the standard after that date, the satellite decision beside it
still stands on its own reasoning; if a feature landed that is not here, it is
missing rather than refused.

**Why this folder is separate from [CXX23/](../CXX23/).** C++23 is a published
standard and a reader who types its syntax is typing something real. C++26 is a
moving target, and a satellite milestone promised against a draft is a promise
against something that may not ship. Nothing in this folder appears in
`foreign.cpp` until the corresponding paper is in a published standard.

---

## The catalogue

### The two that matter most here

| # | C++26 | satellite | answer |
|---|---|---|---|
| 1 | **reflection** (P2996) — `^^`, splicing | — | **[M41](M41.md)** — and satellite is better placed than C++ |
| 2 | **contracts** (P2900) — pre/post, assert | — | **[M42](M42.md)** — and more natural here than in C++ |

**Read M41 first.** Every path in satellite is already a number; `--words`,
`--resolve` and `--arms` are all reflection, performed by the tooling. What is
missing is a *program* being able to ask. C++ is bolting reflection onto a
language that resisted it for forty years; satellite's whole design is a
registry that simply cannot be read from the inside yet.

**M42 is the one the error philosophy asks for.** ERROR_HANDLING.md §3 says a
refusal must name what would have made it not happen — and a capsule currently
has no way to state what it required. Contracts are that sentence, written by
the program instead of by the interpreter.

### Concurrency

| # | C++26 | satellite | answer |
|---|---|---|---|
| 3 | `std::execution` / senders (P2300) | — | [M43](M43.md) — composing concurrent work above raw threads |
| 4 | hazard pointers (P2530) | — | [M43](M43.md), and deeper than [M40](../SATELLITE/M40-locks-and-atomics.md) |
| 5 | RCU (P2545) | — | [M43](M43.md) |
| 6 | atomic min/max | — | [M40](../SATELLITE/M40-locks-and-atomics.md) |

### Compile time and source

| # | C++26 | satellite | answer |
|---|---|---|---|
| 7 | `#embed` — a file's bytes at compile time | — | [M44](M44.md) |
| 8 | constexpr exceptions | — | [M33](../SATELLITE/M33-catching-a-refusal.md) + [M34](../SATELLITE/M34-compile-time-evaluation.md) |
| 9 | constexpr placement new | — | never — no manual allocation |
| 10 | erroneous behaviour (replacing some UB) | — | never — satellite has no undefined behaviour to tame |

### Language conveniences

| # | C++26 | satellite | answer |
|---|---|---|---|
| 11 | pack indexing `pack...[i]` | — | [M31](../SATELLITE/M31-user-generics.md) |
| 12 | variadic friends | — | never — no friends, no access to break |
| 13 | placeholder `_` | — | never — every name is declared and used |
| 14 | `=delete("reason")` | — | never — nothing is deleted; a path either exists or refuses |
| 15 | structured bindings as conditions | — | [M35](../SATELLITE/M35-structured-bindings.md) |

### Library

| # | C++26 | satellite | answer |
|---|---|---|---|
| 16 | `std::simd` (P1928) | — | **never** — see below |
| 17 | `std::linalg` (P1673) | — | never — a library, not a language feature |
| 18 | `std::inplace_vector`, `std::hive` | — | never — there is one list, and its storage is not a program's business |
| 19 | saturating arithmetic (`add_sat`) | — | never — numbers are arbitrary precision and do not wrap |
| 20 | `std::text_encoding` | — | [M45](M45.md) — strings are bytes today and the encoding is undeclared |
| 21 | trivial relocation | — | never — refcounted handles, nothing to relocate |
| 22 | **`std::debugging`** / `breakpoint()` | — | **never — decided 2026-09-12, see below** |

---

## The "never" decisions, and who made them

**`std::debugging` — never.** Asked directly, the author's answer was: *"I am not
really about debugging at all, actually... I just display my own variables!"*

That is a design position and not an omission, and it is consistent with the
rest of the language: satellite makes you declare every name and its type, and
`satellite.console.display` is a first-class path rather than a library
afterthought. A language that forces everything into the open is a language
where printing the value IS the debugger.

**It also has better company than it might look.** Kernighan, 1979: *"The most
effective debugging tool is still careful thought, coupled with judiciously
placed print statements."* Rob Pike works the same way; Linus Torvalds has
argued against kernel debuggers on the grounds that they encourage fixing the
symptom instead of understanding the system. And C++'s own emphasis — Stroustrup
on strong typing, RAII and const-correctness — is about catching errors *before*
a debugger is involved, which is the same instinct satellite acts on by making
every name and type explicit.

### What satellite already has, and what it actually lacks

**A refusal already prints a call stack**, innermost first — `report.cpp` renders
`problem.frames` under the caret:

    satl: f.satl:105:23: error S0713: `write_line` wants a string and this one is number
     105 |         out.write_line(state.call_store_subjects())
         |                       ^
           in save_checkpoint, called at line 187
           in satellite.main

So "no frame inspection" would be wrong. Line, caret and stack come for free at
the moment of failure, which is most of what a debugger is reached for.

**The gap is the bug that does not refuse**, and 2026-09-12 produced a textbook
one. `satellite.system.memory.this.used("gb")` read the thread stack rather than
the process, answered 0 forever, and the build loop never terminated. Nothing
failed, so there was no frame to print — and `display` only helps if you already
suspect the variable. It took reading the interpreter's source to find it.

**That is the honest scope of what is missing**: not stepping and not
breakpoints, but a way to watch a value across iterations without editing the
loop — which is closer to logging than to debugging, and would be a much smaller
thing than P2900's `breakpoint()`. It has no milestone here because nobody has
asked for it; this paragraph is the record that the case exists.

**`std::simd` — never.** Satellite walks numbered ops in an evaluator; there is
no vector unit to reach and no code buffer to emit into. Exposing SIMD would
mean a JIT, which is a different project rather than a milestone — the same
argument [../ASM/](../ASM/) makes about inline assembly, one layer up.

**Saturating arithmetic — never.** `satellite.variable.number` is arbitrary
precision (`satellite_number/limbs.cpp`), so it does not wrap and there is
nothing to saturate. The C++ feature exists because `int` overflows; satellite's
answer is that its numbers do not.

---

## Satellite milestones this folder asks for

| milestone | brings | asked for by |
|---|---|---|
| [M41](M41.md) | a program reading the language's own numbering | 1 |
| [M42](M42.md) | contracts — a capsule stating what it required | 2 |
| [M43](M43.md) | composing concurrent work, and lock-free reclamation | 3, 4, 5 |
| [M44](M44.md) | a file's bytes at parse time | 7 |
| [M45](M45.md) | a declared string encoding | 20 |

**None of these are in `foreign.cpp` and none should be** until the paper behind
them is in a published standard — ERROR_HANDLING.md §10's rule is that a
milestone named in a message is a promise, and a promise against a draft is one
satellite cannot keep on its own schedule.
