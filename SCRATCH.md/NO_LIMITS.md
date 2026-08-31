# No limits — the interpreter must not stop at a depth, and today it does

**Written 2026-08-31, immediately after M7 landed (`e76443b`, `3dca061`), on the
author's instruction.** Nothing here is built yet. This file is the record of a
decision the author took and the plan for carrying it out, so that neither has to
be arrived at twice.

**Delete it when the work lands** and the permanent documents carry it —
DESIGN §7.5, PLAN §2.5 and PLAN §8's M9 entry are the three that change.

---

## 0. The author's decision, in their words

> *"I don't want there to be any limits like 20,000 and 8 MiB — this is the
> reason we re-worked the entire language, it wasn't supposed to have any
> limits."*

> *"if the recursion shuts the program off after 20,000 then that is not a
> working interpreter, that's a broken interpreter!"*

Asked how far to take it, the author chose **everything, including §7.5**: the
static passes AND the evaluator, no recursion ceiling anywhere, and M9's
definition changes to match.

**This is DESIGN §1.1's tie-breaker applied to the interpreter itself.** *Do
absolutely everything for the user, and pay for it in performance rather than in
their attention — but never do anything behind their back.* A process that
disappears with signal 11 is the worst possible version of "behind their back":
no code, no span, no sentence, no exit status a script can read.

---

## 1. Three things were conflated, and only one of them is a limit

The M7 report called all three "limits". That was wrong and it made the tree
sound like it has ceilings it does not have. Corrected here:

### 1.1 The 8 MiB is the operating system's, not the language's

`ulimit -s`. DESIGN §7.5 already takes a position on it in the author's own
words: *"`ulimit -s` is therefore the knob for how deep a program may recurse,
and it is **outside the language on purpose**."*

Measured 2026-08-31, same binary, same 40,000-deep program:

| `ulimit -s` | result |
|---|---|
| 8192 (this machine's default) | segfault |
| 65536 | parses fine |
| unlimited | parses fine — **and so does 300,000 deep** |

Nothing in satl chose 8 MiB.

### 1.2 The 19,000 / 20,000 / 30,000 figures are not limits — they are CRASHES

This is the thing to fix. Four walkers recurse on the C++ stack with **no bound
at all**, and those numbers are only where the machine ran out. A limit refuses
in words; this disappears.

### 1.3 The 2,000 is a real limit and it was added at M7, by me

`resolve::kMaxDepth` in `src/name_resolver/resolve.hpp`, with **S0501** in
`errors.def`. It is a **fixed constant**, which is the wrong shape twice over:
DESIGN §7.5's own history is that the first guess (10000) was wrong *precisely
because it was fixed*, and §7.5's answer is a ceiling **derived** from
`RLIMIT_STACK`. M7 copied §7.5's number without its derivation.

**Under this decision it does not get derived — it gets deleted.** So does
S0501.

---

## 2. What actually happens today — corrected twice, and the second time reversed it

**THE FIRST VERSION OF THIS SECTION OVERSTATED THE PROBLEM AND THE AUTHOR CAUGHT
IT.** It reported crash depths as though they bore on real work. They do not, and
three measurements say why.

### 2.1 A real program nests at depth 6

`/home/madness/code/satl/view_forge/view_forge_main.satl` — 2,459 lines, 53
capsules, the largest satellite program that exists:

| | measured |
|---|---|
| max brace nesting | **6** |
| max paren nesting | **3** |
| `satellite.statement.while` loops | **103** |

The crash figures below start at 19,000.

### 2.2 Calls are not depth, which is why it runs forever

The author's question was why the old satl runs `view_forge` infinitely, calling
capsules millions of times, without dying. **Because a loop costs zero stack
depth** — the frame is reused every iteration — and that program is 103 `while`
loops. A million calls that each RETURN is depth 1. Depth only grows when calls
have not returned yet.

### 2.3 The old satl does not crash because it HAS the limit

`old_versions/first_satellite/src/evaluator/helpers_limits.cpp:38`:

```cpp
constexpr int DEFAULT_MAX_DEPTH = 2000;
```

clamped by a ceiling derived from `RLIMIT_STACK`, reachable through
`satellite.library.system.max_depth`, and `evaluator/expr.cpp` refuses with a
satellite error rather than dying:

> `expression nests deeper than satellite.library.system.max_depth (2000)`

**So v1 has exactly the 2000 M7 reproduced** — M7 copied it out of DESIGN §7.5,
which recorded it from v1. Nobody has ever seen it fire because nothing anybody
writes goes near depth 2000.

### 2.4 The crashes, before and after the stack was widened

`ulimit -s 8192`, one expression nested N deep. **Before** is the tree as M7 left
it; **after** is with `machine_limits`' `kWantedStackBytes` in place (§4.1).

| N | `--check` | `--unparse` | `--satc` | after, all three |
|---|---|---|---|---|
| 19,000 | ok | **SEGFAULT** | ok | ok |
| 20,000 | ok | SEGFAULT | **SEGFAULT** | ok |
| 32,000 | **SEGFAULT** | SEGFAULT | SEGFAULT | **ok** |
| 100,000 | — | — | — | **ok** |
| 500,000 | — | — | — | **ok** |

Nested `satellite.statement.if` blocks: 40,000 killed every command before;
200,000 now parses, and `--unparse` aborts on `bad_alloc` rather than
segfaulting — **which is a different failure and an honest one.** The unparser
indents four spaces per level, so its output is quadratic in nesting depth:
200,000 levels is ~80 GB of spaces. That is the machine running out of memory to
hold an answer, not a walker running off its stack.

## 3. Every recursive walker in the tree

Checked by reading, 2026-08-31. The column that matters is whether the depth is
**controlled by the user's program** — a walk over a fixed table cannot be made
deep by anybody.

| where | what recurses | user-controlled? |
|---|---|---|
| `parser/parser_expressions.cpp` | `expression` → `unary` → `postfix` → `primary` → `expression` (parens), and `expression(min_precedence)` climbing | **yes** |
| `parser/parser_statements.cpp` | `statement` → `block` → `statement` | **yes** |
| `parser/parser_control_flow.cpp` | `if_stmt` / `while_stmt` / `for_stmt` → `statement` | **yes** |
| `parser/parser_types.cpp` | `type` → `generic_arguments` → `type` | **yes** |
| `parser/parser_declarations.cpp` | `suit_body` → `suit_member` → `capsule_decl` → `block` | **yes** |
| `abstract_syntax_tree/unparse.cpp` | `expression`, `type`, `statement` → `block` → `statement` | **yes** |
| `satellite_cache/write_expressions.cpp` | `expression` | **yes** |
| `satellite_cache/write_declarations.cpp` | `statement` → `block` | **yes** |
| `name_resolver/walk.cpp` | `expression` ↔ `statement` (bounded today by `kMaxDepth`) | **yes** |
| `name_resolver/names.cpp` | `member` / `call` → `expression` | **yes** |
| `name_resolver/numbers.cpp` | `type_of` → generic arguments | **yes** |
| `satellite_words/dump.cpp` | the trie walk | no — 254 fixed nodes, depth 6 |
| `machine_limits/dump.cpp` | none that nests | no |

**`satellite_cache/paths.cpp`'s `flatten()` is already iterative and is the model
to copy.** It walks a postfix chain with a `for` loop up the tree into a flat
vector, and its own comment says why: *"the alternative is a matcher that looks
up the tree at every step, which is the same walk written twice."* It cannot be
made to crash.

---

## 4. What the answer is, and it is already in PLAN

**PLAN §2.5 — "Why the explicit control stack is deferred, not dropped."** A
walker that keeps its own stack **on the heap** has no depth at all: the bound
becomes memory, the same way a list's bound is memory. That is what "no limits"
means concretely, and it is the only thing that means it — every other answer is
a bigger number.

§2.5 defers it **for the evaluator**, on a 2–3× cost figure it admits in its own
sentence is *"borrowed rather than measured, which by §9's own rule means it
decides nothing here until this project measures it."* It says nothing at all
about the static passes, which is where all four of today's crashes are.

**Under the author's decision §2.5 is un-deferred.** The cost figure has to be
measured rather than quoted (PLAN §9), and if it is real it is paid — §1.1 says
pay in performance rather than in the user's attention, and a crash is the
user's attention.

---

## 4.1 What was BUILT: satl raises its own stack, 2026-08-31

**The author's question was "why wouldn't we just multiply this 8 MiB by 1024 to
get 8 GiB? What is the argument against doing that?"** Measured, and there is
almost none.

**The 8 MiB is a shell's soft default, not a kernel wall.** This machine reports
`8192 KiB soft, unlimited hard`, and systemd's `DefaultLimitSTACK` is `infinity`.
A process may raise its own soft limit up to the hard one **without root**.

**And it works, which was the thing to check rather than assume.** A recursion
that died before 100,000 frames at the default ran past **2,600,000** after
`setrlimit` inside `main()` — so the folklore that a runtime raise is unreliable
because of mmap placement is false on this kernel.

**It costs nothing measurable:**

| | VmSize | VmRSS |
|---|---|---|
| after `setrlimit` 8 GiB | 6.3 MiB — **unchanged** | 3.5 MiB |
| after M6's 24 pool threads | 230.3 MiB — **identical with and without** | 3.6 MiB |

A stack is lazily committed, so the reservation is address space and not memory.
**And the pool does not multiply it**: glibc fixes the default stack size for new
threads at library init, before `main()` runs, so raising it afterwards leaves
the 24 pool threads at 8 MiB each. Startup cost is one syscall — `satl
--version`'s share moved 0.179 → 0.186 ms, inside the noise.

**Built as `facts::widen_stack()` (the mechanism, in `system_facts/`) called from
`limits::begin()` with `kWantedStackBytes` (the policy, in `machine_limits/`)**,
which is the seam LAYOUT.md draws between those two directories. `satl --limits`
prints it, because M6's rule is that every value satl holds to says where it came
from:

```
  the stack         8.0 GiB (RLIMIT_STACK), 3.8 KiB in use on this thread
                    satl raised it from 8.0 MiB -- the soft limit is a default
                    and the hard limit was not in the way
```

### 4.1.1 What it does NOT do, and this is why §5 still exists

**It is a bigger number and not the absence of one.** 8 GiB is about 2.6 million
frames — 1,300× what v1 refuses at, and still a number. DESIGN §7.5's rule is
unchanged and unmet.

**Three things it cannot give:**

1. ~~**A refusal in words.**~~ **WRONG, AND MEASURED WRONG THE SAME DAY.** This
   said the C++ stack *"is not something satl allocates and therefore not
   something it can count."* **Touched stack pages are resident memory**, so
   `process_memory_bytes()` counts them and M6's watchdog was already watching.
   A 2,000,000-deep parse reaches **1.36 GiB resident**, and under
   `MEMORY_MAX=1GiB` satl prints
   `stopping -- this run is using 1.0 GiB and MEMORY_MAX is 1.0 GiB (the file)`
   and exits 4.

   **What is actually left is two narrower things.** The DEFAULT case — with no
   config `MEMORY_MAX` is the whole machine, so 8 GiB of stack goes first and
   that run still segfaults — and the SENTENCE, which names memory rather than
   recursion.

   **And the cheap answer is not a heap stack.** `facts::thread_stack_bytes(used,
   total)` already exists, M6 built it, and `satl --limits` prints it. A watchdog
   that watched that ratio too would refuse a runaway recursion in its own words,
   on the default configuration, with machinery already in the tree. **That is
   the next thing to build here and it is far smaller than §5.**
2. **Uniformity across threads.** The main thread gets 8 GiB and the pool's 24
   get 8 MiB, so "how deep may I go" depends on which thread you are on. It does
   not matter today, because everything runs on the main thread; it matters the
   day evaluation moves off it (M23's threads, M24's window).
3. **Portability.** `setrlimit` is POSIX. The Windows cross-build in
   `SCRATCH.md/PORTING.md` sets a stack reserve in the PE header instead
   (`/STACK:`), which is a link-time flag and a different mechanism.

### 4.1.2 The number should be a share of the machine, not 8 GiB

**The author's point, 2026-08-31: *"we will be totally geared towards the
terabytes of ram that are coming out in the future."*** `kWantedStackBytes` is a
constant, and satl reads `facts::mem_total_bytes()` a few lines later in the same
startup. 8 GiB is a quarter of a 32 GiB laptop and a four-hundredth of a 4 TiB
machine; a share would be right on both.

**And asking for more is free.** The reservation is address space, not memory —
measured above at 0.0 MiB of VmSize — so a terabyte machine can be handed a
terabyte-shaped request at the same cost this one pays. There is no reason for
the number to be small and no reason for it to be fixed.

**Two decisions first, neither hard.** *What share of what*: total memory, or
`MEMORY_MAX` — which is the number satl is actually allowed, and is read AFTER
the raise today, so `begin()`'s order would have to change. And *what floor*, so
that a share of a small machine never comes out below the 8 MiB it would have had
anyway. `machine_limits/limits.hpp` carries it beside the constant.

**This is the cheapest item in this file and probably the next one to do.**

**So the urgency is gone and the work is not.** Every depth a person could
plausibly reach now works; §5 is what makes the rule true rather than nearly
true, and it is no longer something to drop everything for.

## 5. The plan, in order, and why this order

Each step leaves the tree green. **The order is by how easy it is to be sure the
rewrite is correct**, not by how bad the crash is — the printers crash sooner
than the parser but the parser is the one every other pass sits behind.

### 5.1 `name_resolver/` — delete the limit, make the walk iterative

Smallest, and it is the only *limit* in the tree rather than a crash. One work
stack of `{action, node}` replaces `expression()`/`statement()`'s mutual
recursion, pushed in reverse so it pops in source order. The actions that are
not "visit a node" are the ones the recursive version got for free and now have
to be explicit — and they are the whole subtlety:

- `CloseScope` — a `Block` and a `For` open one and must pop it after their
  children, not before.
- `Declare` — a `VarDecl`'s name enters scope **after** its initialiser is
  resolved, which is what decides `number x = x`. `tests/resolve_test/names.cpp`
  asserts it.
- `Assign` resolves its value before its target, for the same reason.
- `member()` and `call()` do work **before** their children (the path attempt)
  and **after** (the one-hop selector, which needs the receiver's type). Both
  halves need an action.

Then `kMaxDepth`, `Depth`, `too_deep()` and **S0501 all go**, and
`tests/resolve_test/frames.cpp`'s S0501 assertions become an assertion that a
2,200-deep program **resolves**, not that it is refused. `errors.def` loses a row
— `tests/reporter_test/codes.cpp`'s count goes 59 → 58, and that check exists to
make a deletion visible, so it will fire and that is it working.

### 5.2 `abstract_syntax_tree/unparse.cpp` — the printer

Harder than resolve because it builds a string compositionally rather than
walking for effect. Every node is `prefix + children + suffix`, so the work
stack carries **emit actions** as well as nodes: push the suffix, push the
children in reverse, push the prefix. The output is a single `std::string` built
left to right, which it already is.

The one real subtlety is `bracketed()` — brackets come back from precedence
rather than from memory (`ast.hpp`'s `precedence_of` has two readers for exactly
this reason), so the decision needs the parent's level and which side the child
is on. That is two more fields on the work frame, not a change to the rule.

**`unparse` is what proves the parser is right** (PLAN M4's fixpoint), so
`tests/parser_test/roundtrip.cpp` is the check that this rewrite did not change
what it prints. It must stay green character for character.

### 5.3 `satellite_cache/write_*.cpp` — the `.satc` writer

The same shape as 5.2, and `write.cpp`'s own header says the writer *"is
`unparse.cpp` with the paths substituted"* — so whatever shape 5.2 takes, this
takes the same one, and `tests/satc_test` is the check. Doing 5.2 first is what
makes this one mechanical.

### 5.4 `parser/` — the real work

Recursive descent, and the deepest recursion is
`expression → unary → postfix → primary → '(' expression`. Two pieces:

- **Expressions.** Precedence climbing becomes an explicit operator/operand
  stack — the standard shape, and the tree it builds is the same tree because
  the arena is already built bottom-up by index.
- **Statements, blocks, suit bodies and types.** A work stack of what is
  part-built, the same as the passes above.

**The parser must not lose its error recovery**, which is the thing to watch:
`synchronise()`, `panic_`, `fresh_` and the one-error-per-synchronisation rule
are what make `tests/reporter_test/parsing.cpp` and the two `example/` files that
must NOT parse behave the way M4 and M5 pinned them. Those are the tests that
say the rewrite kept the parser's manners.

**And `brackets_` already exists.** The parser counts open brackets today; what
it does not do is stop using the C++ stack for them.

### 5.5 The evaluator — M9, and this is what changes about it

M9 has not started, so this is not a rewrite: it is a **constraint on what M9 may
be**. PLAN §8's M9 entry says *"Recursion depth is bounded here"* and PLAN §2.5
says to *"bound the recursion depth from M9 onward so deep recursion produces a
clean `capsule call too deep` error rather than a segfault."* Both sentences are
now wrong in the same way — the answer is not a cleaner refusal, it is not
refusing.

**What M9 must build instead:** closure compilation onto an explicit control
stack, so a satellite program's recursion depth is bounded by heap and nothing
else. §2.5's whole first paragraph is the argument *for* it — pausable and
resumable execution, which is what green threads, generators, a stepping
debugger, Ctrl-C at an arbitrary point and driving the interpreter from a GTK
idle callback all need. Those were listed as things a CEK machine would *buy*;
under this decision they arrive as a side effect of the thing that was going to
be built anyway.

---

## 6. What changes in the permanent documents

Do these **with** the code, never before it — a document that claims a property
the tree does not have is worse than one that admits the gap.

| document | what changes |
|---|---|
| **DESIGN §7.5** | Retitled and rewritten. *"Recursion is bounded, and the bound is derived"* becomes the opposite. The measured content stays as **history** — 3169 bytes per activation at -O2, the two stack cliffs, the 10000 that could never fire — because it is the evidence for why a fixed guess is wrong, and it is the author's own measurement. What goes is the conclusion. |
| **DESIGN §7** | §7.5's neighbours cite it; M7 added a paragraph separating the resolver's bound from the evaluator's, and that paragraph should collapse to one sentence when neither exists. |
| **DESIGN §12** | The deferral list is *"only useful if the things missing from the language are on it, and equally only if the things on it are still missing."* The explicit control stack is not on it and now should be — as **taken**, not deferred. |
| **PLAN §2.5** | Un-deferred. The 2–3× figure has to be **measured** before it is quoted again (PLAN §9's own rule, which §2.5 already invokes against itself). |
| **PLAN §2.6** | The order of adoption gains it. |
| **PLAN §8, M9** | *"Recursion depth is bounded here"* → the explicit control stack is built here, and depth is bounded by memory. |
| **PLAN §8, M7** | Its done-when names the resolver's bound as a clause. That clause is withdrawn, and the review says why. |
| **MILESTONES/M7.md** | §4.5's table is the measurement that started this, and §6 item 2 named the fix as *"a bound in the parser"* — which is the wrong fix under this decision. Both need a line pointing here, and then at whatever lands. |
| **errors.def** | S0501 deleted. Its block note says a code *"has to survive a row being deleted"*, so the number is not reused. |
| **`tests/reporter_test/codes.cpp`** | 59 → 58, which that check exists to make visible. |

---

## 7. What "done" means

**A satellite program with no depth in it at all.** The acceptance test is that
every command answers, at every depth, until the machine is out of **memory** —
and that running out of memory says so in words with an exit status, because
M6's watchdog already owns that path (`EXIT_LIMIT = 4`, and DESIGN §9's rule that
a refusal is a sentence).

Concretely, at the default `ulimit -s 8192`:

- `--check`, `--unparse`, `--satc`, `--resolve` on 100,000 nested brackets: all
  four answer, none dies.
- The same on 100,000 nested `satellite.statement.if` blocks.
- `--unparse` still round-trips every file in `example/` character for character.
- All seven suites green, and the new depth fixtures are in
  `tests/parser_test/` and `tests/resolve_test/` rather than in a scratch file,
  because `065-tests.mk`'s rule is that a test binary is where a claim gets
  checked on every build.

**And the number in the fixtures is deliberately absurd.** A 100,000-deep
expression is machine-generated and no person will write one; the point is that
the interpreter's answer does not depend on who generated the file, which is what
"no limits" means.

---

## 8. Open, and only the author can answer

1. **What happens when memory really does run out?** M6's watchdog stops the
   process at `MEMORY_MAX` with one line and exit 4. A walker that has filled the
   heap with its own stack is the same event arriving from a different direction,
   and it is worth deciding whether it is the same sentence. §7 assumes it is.
2. **Is the 2–3× real, and is it paid everywhere or only where it is needed?**
   PLAN §2.5's figure is borrowed. It has to be measured on this tree (PLAN §9),
   and the answer might be that the static passes take an explicit stack — they
   run once per program and nobody will see it — while the evaluator's shape is
   decided on a number rather than on a principle. The author chose *everything*;
   this records what "everything" costs before it is spent.
3. **What does `satellite.library.system.max_depth` `1 14 2 2` mean now?** It is a
   **numbered path**, so WORD_NUMBERS §1.2 — *never renumber, never reuse* —
   means it cannot be deleted and has to mean something. PLAN §8 promised it as
   M9's recursion ceiling with M16's search walk as a second consumer; the
   ceiling is gone and the search walk is not, so the dial has one certain reader
   and an unclear job. Three readings and they are the author's to pick:

   - **a memory ceiling on the control stack** — a count of bytes rather than of
     frames, which keeps the name honest and makes it a sibling of `MEMORY_MAX`
     rather than of `ulimit -s`;
   - **a diagnostic aid** — say something when a program passes this depth and
     carry on, which is a debugging knob and not a limit;
   - **a runaway detector** — the only reading under which an infinite recursion
     still terminates before it exhausts memory.

   **The third one is the question hiding inside this decision.** `fact(n)` with
   no base case used to hit a ceiling and stop; with no ceiling it fills the heap
   and then meets M6's watchdog, which kills the process at `MEMORY_MAX` with one
   line and exit 4. That is a working answer and it is a *worse sentence* than
   the one a depth error could have given, because it names memory rather than
   recursion. Whether that trade is acceptable is a decision and not an
   implementation detail.

4. **Does the parser rewrite become its own milestone?** It is the largest piece
   here by a distance, and PLAN §8's numbers are positions rather than names
   since 2026-08-30, so inserting one is cheap. The alternative is that it lands
   inside M8 or M9 and those milestones stop being about what they say.
