# Building the interpreter

The plan, stage by stage. Each stage is a separate landing: it compiles, it has
its own test binary, and it makes the tool able to do something it could not do
before. Nothing is a prerequisite for its own test.

The stage numbers are the `Stage` enum in `include/satellite/interpret.hpp`, so
`InterpretResult::reached` is a literal progress bar for this document.

```
Stage 0  Lex        built
Stage 1  Validate   built
Stage 2  Parse      ---- everything below here does not exist ----
Stage 3  Compile
Stage 4  Execute    <-- TBB, GTK and a Python bridge are all blocked here
Stage 5  Include    satellite.include actually includes
Stage 6+ capsules as values, spacesuits, scheduler, satellite.thread
```

**The rule this plan does not break.** Until Stage 4 lands, nothing in the
binary, the help text, the README or these docs may say satellite code runs.
Today `satellite run` executes `satellite.cxx` blocks and says so in as many
words. Each stage owns its own honesty: part of the deliverable is updating the
CLI's usage text to describe exactly what it can now do and no more.

---

## Stage 0 — Lex (built)

`src/lexer.cpp`, 414 lines. `test_lexer` covers it with 42 checks.

Produces `std::vector<Token>` from source text. Two rules carry the language:
`satellite` is reserved only when not preceded by a dot, and a newline ends a
statement only when the previous token could finish one (Go's rule, with `()`
and `[]` suppressing termination). `satellite.cxx { ... }` collapses to a single
`Tok::CxxBlock` holding raw C++. `op_code_for()` resolves the text-versus-
arithmetic ambiguity permanently at lex time — a `-` inside a string literal is
charmap code 73, a `-` outside one is code 99, and after lexing they can never
be confused again.

Errors are collected, not thrown, so one bad string literal does not hide the
rest of the file.

**Gap worth closing at Stage 2, not now:** there is no `Tok::Caret`. The charmap
reserves code 103 for `^` as arithmetic, but `^` in source falls through to
`Tok::Unknown` (`src/lexer.cpp:390`). Either add the token or drop the charmap
code; leaving them disagreeing is the kind of thing that costs an afternoon
two years from now.

**Unblocks:** everything.

---

## Stage 1 — Validate (landed)

`include/satellite/interpret.hpp` (90 lines) and `src/interpret.cpp` (245 lines)
are written, with a `test_interpret` binary of 84 checks.

This is the seam: one entry point — `interpret_file(path, opts)` returning an
`InterpretResult` — that a front end can be written against once and keep
working as every later stage lands. Today it reaches `Stage::Validate` and
stops.

**Only `satellite-gui` is built on the seam so far.** `src/main.cpp` still lexes
and drives `cxx::Bridge` directly and does not include `interpret.hpp` at all;
porting the CLI onto `interpret_file()` is outstanding work, not something this
stage already did. Until that port lands the two front ends can drift, which is
exactly what the seam exists to prevent.

What it does with no parser:

- `resolve_satl_path()` — `satellite run hello` finds `hello.satl`.
- `declares_satellite_include()` — scans the token stream for
  `satellite . include ( satellite )`. Five tokens in sequence; no grammar
  needed.
- `declares_main()` — same trick for
  `satellite.capsule satellite.main(`.
- runs any `satellite.cxx` blocks, which is the one thing that genuinely
  executes today.
- collects everything into `Diagnostic`s with line and column, formatted the
  way a compiler formats them.

**Test:** feed it a file with the include line and a file without, assert the
reached stage and the diagnostic. Feed it `hello` and assert it resolves to
`hello.satl`. Feed it a file with a cxx block and assert the output.

### `satellite.include` is CHECKED, not performed — deliberately

Version 001 requires every main file to contain `satellite.include(satellite)`
and rejects one that does not. It does not read, resolve or load anything.

This is not laziness, it is sequencing. An include that actually includes has to
answer three questions at once: where does the file come from, what names does it
introduce, and into what namespace do they land. The third question has no
answer until Stage 3 exists, because "the namespace" *is* the slot table, and
there is no slot table yet. Implementing include before then means implementing
it twice.

Checking it now still buys something real: it makes the requirement enforceable
from day one, so no `.satl` file ever written for satellite is missing the line,
and Stage 5 does not arrive to find a corpus of files it has to be lenient with.

**Unblocks:** a front end written against the seam stops being rewritten every
stage. (True of `satellite-gui` today; true of the CLI once it is ported.)

---

## Stage 2 — Parse

The first stage that does not exist. Turns tokens into structure.

### What has to parse

| form | example |
|---|---|
| library path | `satellite.library.game.score` |
| declaration | `satellite.variable.string name` |
| capsule | `satellite.capsule satellite.main(...) { }` |
| spacesuit | `satellite.spacesuit Ship { }` |
| call | `satellite.console.display(x)` |
| index | `arguments[0]` |
| assignment | `x = a + b * 2` |
| expression | full precedence, parenthesised |
| cxx block | passthrough — one token, one node |

A dotted path is the shape everything else hangs off. The lexer already does the
hard part: `satellite` leading a path is `Tok::Satellite`, and every `satellite`
*after* a dot is `Tok::Ident` with text `"satellite"`. So the parser reads
`Satellite (Dot Ident)*` and never has to special-case the escape hatch —
`satellite.library.my_capsule.satellite` parses as a four-element path with a
perfectly ordinary last element.

### The precedence table

Nine levels, lowest binding first. This is a full commitment — changing it later
silently changes what existing programs mean, which is the worst class of
breaking change.

| level | operators | assoc | notes |
|---:|---|---|---|
| 1 | `=` | right | statement-level; see below |
| 2 | `or` | left | short-circuits |
| 3 | `and` | left | short-circuits |
| 4 | `not` | right, unary | binds looser than comparison, so `not a == b` is `not (a == b)` |
| 5 | `== != < > <= >=` | left, non-chaining | `a < b < c` is a parse error, not `(a<b)<c` |
| 6 | `+ -` | left | `+` with a string on either side yields a string |
| 7 | `* / %` | left | `/` on a string splits, `*` on a string repeats |
| 8 | unary `-` | right | |
| 9 | `.` `[]` `()` | left, postfix | tightest; `a.b[0](x)` chains |

Three choices in there worth defending:

- **`not` below comparison** is Python's placement, not C's. C's `!` binds
  tighter than `==`, which makes `!a == b` mean `(!a) == b` and surprises
  everyone. Since satellite spells it `not`, the word reads as a sentence
  modifier and should behave like one.
- **Comparisons do not chain.** `a < b < c` is either a mathematician's range
  test or a nonsense boolean comparison, and guessing wrong is silent. Rejecting
  it costs one line in the parser and removes a whole bug class.
- **`=` is a statement, not an expression.** No `if (x = 1)`. satellite has no
  need for assignment-in-expression and every language that allows it regrets it.
  Parse assignment at statement level only; the table entry above exists so the
  answer to "what is its precedence" is written down.

### Pratt, not a ladder of functions

Precedence climbing with a table lookup, one `parse_expr(min_bp)` function.
Nine nested functions (`parse_or` calls `parse_and` calls `parse_cmp`...) is the
textbook alternative and it works, but every expression then walks nine stack
frames to reach an integer literal, and adding an operator means editing two
functions and getting the chain order right. The Pratt version adds an operator
by adding a row.

Roughly: a `binding_power(Tok)` table returning `{left, right}`, unary handlers
keyed by token, postfix handlers for `(`, `[`, `.`. About 120 lines for the whole
expression grammar.

### AST or single-pass — the honest tradeoff

The README's "Next" list and `docs/PLAN.md` both say *single-pass compiler*. I
want to argue for changing that, and say clearly that it is a change.

**Single-pass** (Lua's design): the parser calls the code generator directly and
never materialises a tree. It is fast — one pass over the source, no allocation
per node, no tree walk — and Lua compiles faster than almost anything as a
result. It also makes the compiler a single tangled function per construct,
where parsing state and register allocation state are interleaved, and it makes
constant folding and any real optimisation awkward-to-impossible because you
have already emitted the instruction before you know what follows it.

**An AST**: parse to nodes, then walk. Costs an allocation per node and a second
pass. Buys: the parser is testable *without a compiler* (parse, print, compare
to a golden string — that is Stage 2's entire test strategy and it does not
exist in the single-pass world), a bug is localisable to "parser" or "compiler"
instead of "the thing", and multiple passes over the tree are free.

**Recommendation: build the AST.** Two reasons, and the second is the load-bearing one.

1. Compile speed does not matter here and will not for years. satellite programs
   are tens of lines. Lua's single pass was designed for a language embedded in
   games that compiles scripts at level load; satellite is not in that situation
   and is trading its most valuable property (debuggability of an unfinished
   compiler) for a property it cannot use.

2. **Forward references make a single pass wrong for satellite specifically.**
   `satellite.library` makes every variable globally reachable by name. A capsule
   at the top of a file can legally reference `satellite.library.game.score`
   declared at the bottom — and at Stage 5, declared in another file entirely. A
   single-pass compiler must emit a placeholder operand and backpatch it, keeping
   a fixup list per unresolved name. A two-pass compiler over an AST just walks
   the tree once to collect declarations and once to emit. The slot-index scheme
   in Stage 3 — the thing that decides whether satellite is fast — is *much*
   easier to get right when every declaration is known before any instruction is
   emitted. Backpatching global slot indices is a bug farm.

If compile time ever becomes a real complaint, the AST can be kept and the tree
walk fused into the parser for a fast path later. Going the other direction —
recovering a tree from a single-pass compiler — is a rewrite.

`README.md` "Next" item 2 should be updated to say *two-pass compiler over an
AST* so the two documents do not contradict each other.

**Test:** `test_parser`, golden-string round trip. Parse a source, pretty-print
the AST back to a canonical form, compare. Every precedence question in the
table above becomes one assertion: `parse("a + b * c")` prints `(+ a (* b c))`.
Error recovery gets its own tests — a missing `)` must produce one diagnostic
with the right line, not a cascade.

**Size:** `ast.hpp` ~150 lines, `parser.hpp` ~80, `src/parser.cpp` ~650,
`tests/test_parser.cpp` ~300 lines / ~70 checks. Call it 3–4 days including the
error-recovery tests, which are half the work and the half people skip.

**Unblocks:** Stage 3. Also, immediately and independently useful: `satellite
check` can stop counting tokens and start reporting real syntax errors, and
`satellite ast file.satl` is a shipping subcommand that proves the parser
without a compiler behind it.

---

## Stage 3 — Compile to bytecode

Walk the AST, emit instructions for a register machine.

### Register-based, not stack-based

A stack VM turns `a = b + c` into four dispatches (push, push, add, store); a
register VM turns it into one (`ADD a, b, c`). The Lua 5 authors reported ~30%
fewer instructions dispatched for the same program after the switch, at the cost
of larger instructions and a real register allocator. That number is *theirs*,
not measured here — and this repo has already caught received wisdom being wrong
on this hardware once, when the dispatch table lost to an if-chain
(`bench/bench_dispatch.cpp`). Build a stack-VM straw man only if the register
allocator turns out to be the schedule risk; otherwise take the 30% and move on.

Registers are a window into one contiguous `Container` stack. A `Container` is
16 bytes and refcounted, so "register" means a slot in that array, not a machine
register. Allocation is the standard expression-stack discipline: a temporary
lives in the lowest free slot, freed at the end of the statement. Locals get
fixed slots at the bottom of the frame. Max 256 registers per frame so operands
fit in a byte.

### THE decision: slot indices, not hash lookups

This is the one choice in the whole document that determines whether satellite
is a fast language.

satellite's central idea — `satellite.library` makes every variable reachable
from anywhere by name — is exactly the idea that destroys interpreter
performance if implemented naively. The naive implementation is a hash map from
dotted-path string to value, probed at every read and every write.

Do the arithmetic against numbers this repo already measured. An `Int + Int`
through `add()` is **3.85 ns** (README, Xeon E5-2670 v3). Hashing a string like
`"satellite.library.game.score"` and probing a table is 20–50 ns depending on
cache luck. So under a hash-lookup design, reading a global costs **5–13x** as
much as the arithmetic it feeds, and since in satellite *every* variable is a
global by construction, essentially every operand pays it. The container work —
the 16-byte layout, the hybrid dispatch, the overflow-free integer path, all of
it already built and measured — would be invisible under the name lookup.

So: **resolve `satellite.library.x.y` to a `uint32` slot index at compile time.**

- The declaration pass builds a `SlotTable`: fully-qualified dotted path →
  index. Hashing happens once per name per compile, never at runtime.
- The bytecode carries the index. `GETGLOBAL rA, slot` is one indexed load from
  a flat `std::vector<Container>` — the same cost as reading a local.
- A name that cannot be resolved at compile time is a **compile error**, not a
  runtime lookup. This is worth insisting on: it turns typos into diagnostics.
- Genuinely dynamic access (a name computed at runtime, if satellite ever wants
  one) gets its *own* opcode, `GETDYNAMIC`, so the expensive path is visible in
  a disassembly instead of hiding inside the common one.

There is a second payoff already written down: `docs/PLAN.md` builds its
`parallel_for` race analysis on exactly this. "The compiler already resolves
`satellite.library.x.y` to slot indices, so it can classify every variable a
parallel body touches" — loop-local, read-only, disjoint-index, or shared-and-
rejected. That classification is impossible with runtime hashing, because you do
not know what a body touches until it runs. Stage 3 is what makes Stage 8's
compile-time race rejection possible.

### Instruction set

32 bits per instruction: `op:8 a:8 b:8 c:8`, with a wide form `op:8 a:8 bx:16`
for constants, globals and jump offsets. Twenty-six opcodes covers the language
as currently specified.

| opcode | operands | effect |
|---|---|---|
| `MOVE` | a, b | `R[a] = R[b]` |
| `LOADK` | a, bx | `R[a] = K[bx]` (constant pool) |
| `LOADNIL` | a | `R[a] = nil` |
| `LOADBOOL` | a, b | `R[a] = (bool)b` |
| `GETGLOBAL` | a, bx | `R[a] = G[bx]` — one indexed load |
| `SETGLOBAL` | a, bx | `G[bx] = R[a]` |
| `GETINDEX` | a, b, c | `R[a] = R[b][R[c]]` |
| `SETINDEX` | a, b, c | `R[a][R[b]] = R[c]` |
| `NEWLIST` | a, b | `R[a] = list of b elements from R[a+1..]` |
| `LEN` | a, b | `R[a] = length of R[b]` |
| `ADD SUB MUL DIV MOD` | a, b, c | `R[a] = R[b] op R[c]` — calls into `ops.cpp` |
| `NEG` | a, b | `R[a] = -R[b]` |
| `NOT` | a, b | `R[a] = not R[b]` |
| `EQ NE LT LE GT GE` | a, b, c | `R[a] = bool` |
| `JMP` | sbx | unconditional |
| `JMPIF` / `JMPIFNOT` | a, sbx | branch on `R[a].truthy()` |
| `CALL` | a, b, c | call `R[a]` with b args at `R[a+1..]`, c results to `R[a]` |
| `RET` | a, b | return b values from `R[a]` |
| `CXX` | a, bx | run `K[bx]` through `cxx::Bridge`, result to `R[a]` |

Notes on three of them:

- **Comparisons produce a boolean into a register** rather than fusing
  test-and-jump the way Lua does. Fusing saves an instruction per comparison in
  an `if`, but it means a comparison in value position (`x = a < b`) needs a
  different code path. Start unfused; it is a peephole optimisation to add later
  and it is measurable before and after.
- **Arithmetic opcodes call the existing `add()`/`sub()`/... inlines**, so the
  hybrid dispatch, the overflow promotion to `Big` and the string behaviours
  come for free and stay in one place. The VM does not reimplement any of it.
- **`CXX` means Stage 4 subsumes everything `satellite run` does today** on its
  first day, rather than replacing it. A cxx block becomes an expression in a
  real program instead of a top-level curiosity.

### Test — and this is why Stage 3 stands alone

Write the disassembler in the same commit as the assembler. Then the test is
textual and needs no VM: compile a source, disassemble, compare to a golden
listing.

```
satellite compile -S hello.satl

  0  GETGLOBAL  r0, G[0]      ; satellite.library.arguments
  1  LOADK      r1, K[0]      ; 0
  2  GETINDEX   r0, r0, r1
  3  CALL       r0, 1, 0      ; satellite.console.display
  4  RET        r0, 0
```

That listing is checkable, reviewable and diffable. It is also the thing you
will stare at for the whole of Stage 4, so it is worth making readable now — put
the resolved name in a comment column, always.

**Size:** `opcode.hpp` ~120 lines (define the opcodes with an X-macro, so the
disassembler, the switch fallback and the computed-goto table are all generated
from one list and cannot drift apart), `src/compile.cpp` ~800, `src/disasm.cpp`
~120, `tests/test_compile.cpp` ~250 / ~60 checks. 4–5 days.

**Unblocks:** Stage 4. Ships `satellite compile -S` on its own.

---

## Stage 4 — Execute

The VM. This is the stage that makes satellite a language.

### Computed goto

GCC's `&&label` / `goto *ptr` extension: instead of one `switch` at the top of
the loop, each opcode's handler ends with its own indirect jump to the next
opcode's handler.

The reason it wins is branch prediction, not instruction count. A `switch` has
one indirect branch shared by every opcode, so the predictor sees a single site
with 26 possible targets and mispredicts constantly. With computed goto there
are 26 branch sites, each of which learns *what usually follows this particular
opcode* — and bytecode has strong local structure (`GETGLOBAL` is usually
followed by `GETINDEX`, `LT` by `JMPIFNOT`). Reported gains are 20–25%.

Two caveats, both mandatory:

- **Measure it here.** This repo's one benchmark of received wisdom found the
  opposite of the literature (`bench/bench_dispatch.cpp`: the dispatch table
  lost to an if-chain by 1.1–1.3x on this Xeon). A `bench_vm.cpp` comparing the
  two dispatch strategies over the same bytecode is a two-hour job and it either
  confirms 20–25% or saves you from carrying a GCC extension for nothing.
- **Keep the switch.** `#ifdef __GNUC__` selects computed goto, everything else
  gets the switch. Both are generated from the `opcode.hpp` X-macro so a new
  opcode cannot be added to one and forgotten in the other.

### Call frames

One contiguous `std::vector<Container>` for the whole value stack; a frame is a
window into it, identified by a base index. A separate preallocated frame array,
not a `std::vector<Frame>` pushed per call:

```
struct Frame {
    const Chunk* chunk;      // bytecode being run
    const uint32_t* pc;      // saved instruction pointer
    uint32_t base;           // R[0] of this frame in the value stack
    uint8_t  dest;           // caller register receiving the result
    uint8_t  want;           // results the caller wants
};
```

`CALL` pushes a frame and re-enters the dispatch loop by `goto`, not by C++
recursion. Recursing the interpreter into itself burns a C++ stack frame per
satellite call, caps recursion depth at whatever the OS stack allows, and makes
the whole loop non-inlinable. Staying in one loop also means a satellite stack
overflow is a *satellite* diagnostic with a satellite traceback, at a depth you
choose, instead of a SIGSEGV.

### The return protocol, and a language question Stage 4 forces

`satellite.return(satellite)` is in every example in the README, and Stage 4 is
where somebody has to say what the argument means. Three candidate readings:

1. bare `satellite` is nil — `return(satellite)` returns nothing;
2. bare `satellite` is the enclosing capsule — returning self;
3. bare `satellite` is a success sentinel, distinct from nil.

Pick one before writing `RET`, write it into this file, and add a test. It is
cheap now and it is in every program ever written afterwards. (Reading 1 is the
simplest and makes `satellite.return(satellite)` mean "return, no value", which
matches how it reads in the hello-world.)

Mechanically: `RET a, b` copies b values to the caller's `dest`, pops the frame,
restores `pc`. Falling off the end of a capsule body is an implicit `RET 0`.

### Errors

**Do not use C++ exceptions inside the dispatch loop.** Not because they cost on
the happy path — they do not — but because a `try` region spanning the loop
constrains what the optimiser can do with it, and unwinding through computed
goto is exactly the kind of thing that works until it does not. Instead:

- the VM carries `Container error` plus a status enum;
- opcodes that can fault (`DIV` by zero, `GETINDEX` out of range, `CALL` on a
  non-capsule) set it and jump to a single `on_error` label;
- `on_error` walks the frame array to build a satellite-level traceback with
  file, line and capsule name, then returns to the host.

Line numbers need a per-chunk `pc → line` table, built at Stage 3. It costs 4
bytes per instruction and it is the difference between a usable error and
`runtime error`. Build it in Stage 3, not retrofitted here.

**A decision that changes existing behaviour:** the README currently records
"unsupported operand pairs return `nil` rather than crashing. Real error
reporting comes with the interpreter." This is the interpreter. `nil + list`
should become a diagnostic. That means the fallback entries in `ops.cpp`'s
tables need a way to signal failure rather than returning nil — either a
distinguished error container or an out-parameter. Decide it at the start of
Stage 4, because changing it afterwards touches every table row.

### Test

This is the first stage whose test is *running programs*. `tests/programs/*.satl`
each paired with a `.expected` file; the test binary runs each and diffs stdout.
Start with the README's hello-world — the fact that it does not currently work
is the whole point.

Plus unit tests for: arithmetic through every opcode, recursion depth, error
tracebacks, a `CXX` block used as an expression, and refcount balance (run a
program, assert no leaked containers — a VM that leaks on every call is easy to
write and hard to notice).

**Size:** `vm.hpp` ~150, `src/vm.cpp` ~900 (the dispatch loop itself is maybe
400; frames, errors and the tracebacks are the rest), `src/builtins.cpp` ~350
(`satellite.console.display`, list and string operations exposed as callables),
`tests/test_vm.cpp` ~400 plus the program corpus. Call it a week of writing and
a second week of debugging — the debugging tail on a VM is always longer than
the writing, and anyone who tells you otherwise has not written one.

### What Stage 4 unblocks — this is the answer to "what next"

Every deferred project in this repo is waiting on exactly this stage:

| project | where it is written down | why it is blocked |
|---|---|---|
| `satellite.statement.parallel_for` on TBB | `docs/PLAN.md` §1 step 5 | "Needs the VM to exist first." The loop body is satellite code; something has to run it per index. |
| GTK callbacks | `CMakeLists.txt:38` builds `satellite-gui` | A callback's whole job is to invoke satellite code from a C callback. There is nothing to invoke. |
| Python bridge | not yet written down | Both directions need a call protocol: Python calling a capsule needs `CALL` and the return protocol; a capsule calling Python needs a frame to return into. |
| Embedded Clang | `docs/PLAN.md` §2, `docs/EMBEDDING.md` | Explicitly sequenced after the VM: "the VM is what makes satellite a language rather than a value type with a lexer." |
| Scheduler / `satellite.thread` | `README.md` threading section | A worker executes bytecode. |
| Ropes for `a + b` | `README.md` Next §4 | Not strictly blocked, but there is no program to measure the improvement on. |

The `parallel_range` wrapper from `PLAN.md` §1 ("first milestone: `parallel_range`
plus a benchmark against a serial loop, before any language syntax") is the one
piece of that list that can genuinely be done in parallel with Stages 2–4,
because it is pure C++ and touches nothing. If two people are working, that is
the second thread of work.

---

## Stage 5 — Include, for real

Now `satellite.include(x)` loads files.

What it does: resolve a name to a path (same rules as `resolve_satl_path`, plus
a search path), lex it, parse it, compile it, and merge its declarations into the
one global slot table. Because slots are global by construction, an included file
does not get its own namespace — which is the whole idea of `satellite.library`
and also the source of every rule below.

Rules that have to be settled here:

- **Cycles.** A includes B includes A. Mark files in-progress during the walk and
  report a cycle with the full chain, not just "cycle detected".
- **Diamonds.** A includes B and C, both include D. D must be compiled *once* —
  compiling it twice would declare every one of its names twice into a flat slot
  table. Key the include set by canonical absolute path.
- **Redeclaration.** Two files declaring `satellite.library.game.score` is an
  error, and the diagnostic must name both files. In a language with one global
  namespace this will happen constantly; the error message is the feature.
- **`satellite.include(satellite)` itself** finally means something — it includes
  the built-in library. Until Stage 5 it is a required incantation that is only
  checked (Stage 1); here it becomes the thing that puts `satellite.console`,
  `satellite.container` and friends into scope.

### Where the parallel include scanner lands

The README's threading section measured three regimes and found that the
I/O-bound one is where massive oversubscription actually wins: **92.5 ms → 5.9 ms,
15.7x**, precisely because blocked threads do not consume cores. Reading and
lexing N source files is exactly that regime, and it is the first place in
satellite where the original 200-worker design has a real job.

The structure that exploits it:

- **Phase 1, parallel:** walk the include graph, and for every file read it and
  lex it. Files are independent — a `Lexer` shares nothing — so this is
  embarrassingly parallel with no locking beyond the visited-set.
- **Phase 2, serial:** compile in dependency order. Slot assignment is inherently
  ordered and trying to parallelise it would reintroduce the backpatching problem
  Stage 2 was designed to avoid.

Two honesty notes. Phase 1 can also parse in parallel, since parsing one file
needs nothing from another — but only if Stage 2's AST route was taken, which is
a second reason for it. And: a three-file program will not notice any of this.
Do not build the parallel scanner until there is a program with enough files to
measure it on, and when you do, put the number in the README next to the others.

**Size:** `src/include.cpp` ~350 lines serial, ~150 more for the parallel scanner
and its benchmark. `tests/test_include.cpp` ~200 / ~40 checks, mostly cycle and
diamond cases. 2–3 days serial; the parallel scanner is a separate half-day plus
a benchmark.

**Unblocks:** multi-file programs, and a standard library written in satellite
rather than in C++.

---

## Stage 6 and after

Sketched, not planned. Each of these should get its own version of the treatment
above before anyone starts it.

**6 — Capsules as first-class values.** A capsule in a container, passed to a
capsule, stored in a list. Needs `Type::Capsule`, a `CLOSURE` opcode, and a
decision about capture: do capsules close over anything, given that
`satellite.library` already makes everything reachable? If the answer is no, this
stage is small and there are no upvalues at all — which would be an unusually
clean outcome and is worth trying to arrange.

**This is where the garbage-collection question arrives, and it is the largest
unpriced item in this document.** Refcounting cannot collect cycles. Two capsules
that reference each other, or a capsule stored in a list it also references, leak
permanently. The options are (a) prove cycles cannot be constructed — plausible if
capsules do not capture, and by far the best outcome; (b) add a cycle collector,
which is ~1,000 lines and touches every heap type; (c) document the leak, which is
not acceptable for a language. Answer (a) or (b) before Stage 6 code is written,
because it decides the shape of `Obj`.

**7 — Spacesuits.** Classes: fields, methods, construction. Field access must be
slot-resolved per shape, for exactly the reason in Stage 3 — a hash lookup per
field access would undo the whole thing. Inheritance is a separate decision and
can be deferred. ~700 lines across parser, compiler and VM.

**8 — Scheduler and `satellite.thread`.** `docs/PLAN.md` §1 has the design. TBB
supplies work-stealing and grain size, so satellite never picks a thread count;
the `slow/medium/fast` priority classes on `satellite.thread.new` become the
load-bearing hint that maps a capsule onto the right one of the README's three
regimes (few workers for fine-grained CPU work, hardware-thread count for coarse,
hundreds for I/O). The compile-time race classification described in `PLAN.md` is
a Stage 3 debt being collected: it only works because slots were resolved.

**9 — The rest.** Ropes for O(1) large-string concatenation (`README.md` Next §4),
the Clang embedding phases 1–2 from `docs/EMBEDDING.md` (ship compressed headers
over a VFS first; treat static Clang as a separately funded decision with a
source-built LLVM attached), and GTK.

---

## Sizes, together

| stage | implementation | tests | days |
|---|---:|---:|---:|
| 0 Lex | 414 + 108 hdr | 217 / 42 checks | done |
| 1 Validate | 245 + 90 hdr | 372 / 84 checks | done |
| 2 Parse | ~880 | ~300 / ~70 | 3–4 |
| 3 Compile | ~1,040 | ~250 / ~60 | 4–5 |
| 4 Execute | ~1,400 | ~400 + corpus | 10–14 |
| 5 Include | ~500 | ~200 / ~40 | 2–3 |
| | **~3,800 new** | **~1,150 new** | **~3–4 weeks** |

For scale: everything that exists today — container, bignum, charmap, string,
ops, lexer, machine, cxx bridge, CLI, and all four test binaries — is 3,828
lines. **Stages 2 through 5 are about as much code again as the entire project
so far**, and Stage 4 alone is a third of it.

That is the honest number. It is also the number that turns a well-tested value
type with a lexer into a language.
