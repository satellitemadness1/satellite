# satellite — the plan

**This file is permanent**, and it is the second satellite's plan. It says how the
language gets built: the architecture, the build, the install, the milestones, and
what is carried over from the first satellite against what is deliberately left
behind. [DESIGN.md](DESIGN.md) is the other half and says what the language *is*.

It supersedes `PLAN_ONE.md`, which was written on 2026-08-25 before any of the
second satellite existed and was always meant to be thrown away. Everything in it
worth keeping is here. **`PLAN_ONE.md` may be deleted as soon as nothing cites it**
— it is not a reference, it is a draft that has been superseded.

The first satellite lives at `old_versions/first_satellite/`, is not going anywhere,
and is the source this is pulled from. It works and it is fast.

---

## 0. What this is all for

[QUAD.md](QUAD.md) names the program satellite has to be able to express:
`/home/madness/code/cxx/quad_infinity/`, 3029 lines of C++ that run a model of a
mind. It is a general-purpose language and everything below is meant generally, but
that program is the acceptance test.

**Both sides bend, and QUAD bends more** — QUAD.md §0 is that rule, and it is what
keeps the language from being shaped by one program's C++ conveniences. A feature
enters satellite only when QUAD's *meaning* cannot survive without it.

*(2026-08-27.)* QUAD.md §3 used to list four unsettled things — floats, a set,
sorting, and a console that could address a screen. **Reading the source closed
three of them and one other**: sorting needs one primitive rather than comparators,
so DESIGN §12's deferral of *a bare name can be a value* survives and §2 stays shut;
the sets and deques are membership tests and bounded ring buffers; the display never
moves a cursor and the real gap was non-blocking input; and QUAD's map keys all fall
inside §6.5's restriction. **`satellite.variable.float` is the one that grew teeth**
and is the whole remaining gap (QUAD.md §3.1, DESIGN §13).

None of it touches M2 through M8 as *code* — but settling it added **71 numbers** to
WORD_NUMBERS.md, taking §2.2 from 144 entries to 215 and eventually 222, and **M2
transcribed all of them on 2026-08-28.** The rest lands on M9 and M10, and QUAD.md §4 proposes a milestone
that does not exist yet: **one mechanism out of `mind.hpp`, running.**

## 1. Where things stand

**Milestone 1 landed 2026-08-26. Milestone 2 landed 2026-08-28.** There is a
`satl` that says what it is, says how a file will be run, refuses to pretend about
the parts that do not exist, and now **holds the whole numbering and can be asked
about it** — `satl --words`. There is still no interpreter behind it.

What exists: the `Makefile` as an index over ten fragments under `make_support/`,
**twenty-five C++ files totalling 3,148 lines** plus `words.def` at 542, the tree's
first `tests/` directory, and `satellite_enterprise/`, the Enterprise Linux
installer and the artwork. *(Counted 2026-08-28.)* The largest C++ file is
`tests/words_test/authority.cpp` at 261 and the largest under `src/` is
`src/satellite_words/words_invariants.hpp` at 244. [LAYOUT.md](LAYOUT.md) lists all of it.

*(Recounted 2026-08-28. This paragraph said "five C++ files… the largest C++ file is
137 lines", which was true of M1 alone and stopped being true the next day.)* M1's
five are `src/system_facts/version.hpp` (76), `src/programs/opening.hpp` (45) and
`.cpp` (68), `src/programs/main.cpp` (137) and `src/programs/cpu_level.cpp` (133).
**Five more landed ahead of their milestones on 2026-08-27** —
`src/programs/terminal.{hpp,cpp}` (35, 236) and `window.cpp` (209), which are
`satl-term` and M11.A, and `src/satellite_random/random.{hpp,cpp}` (106, 108),
which is `satellite.random` and has no milestone at all. **The largest C++ file is
`terminal.cpp` at 236 lines**, inside the 300-line rule and the number to watch.

Two things came out different from the **first satellite**, and both were right:
`--help` exits 0 rather than 2 (a request correctly made is not an error), and that
satellite's claim of "119 shared objects" turned out to be **78 on this machine** and
is not repeated anywhere. Neither was a change against `PLAN_ONE.md`, which had
already called both.

**M2 cost nothing measurable at startup**, which §4.3's floor exists to check.
Measured on this machine 2026-08-28, best of seven runs of 200 invocations, all
three binaries on the same filesystem: a bare `int main(){return 0;}` at
**1.59 ms**, `satl` built from M1's sources at **1.60 ms**, this `satl` at
**1.61 ms**, and a repeat of the bare binary at 1.58 ms — so the noise floor is
about 0.02 ms and the M1-to-M2 difference is inside it. That is what the encoding
buys: the tables are `constexpr` and land in rodata, so nothing runs before
`main()` and a program that never asks about a word never pays for one.

*(The first attempt at this measurement put the bare binary in `/tmp` and the
others in the tree, and reported `satl` as **faster** than an empty `main` — a
result that cannot be true and was a different filesystem rather than a finding.
Recorded because §9's rule is to measure on this machine, and a measurement whose
setup differs between arms is not one.)*

**Next: milestone 3**, the lexer (§8).

### 1.1 The finding this whole plan hangs off

Read the first satellite end to end and one thing shapes everything else:

**It already built the number table, and the evaluator never read it.**

`src/bytecode_format/format.def` holds **107 word entries** (ids running to 110, so
three are spent and unused) and **29 paths**, with frozen ids and static_asserts
enforcing density, ascension and uniqueness. It is careful work. *(Counted
2026-08-26; `PLAN_ONE.md` said 108 and 30.)*

Meanwhile the actual dispatch for `satellite.console.display(x)`, the most-called
thing in the language, is: flatten the member chain into a `vector<string>`,
heap-allocate a joined string **on every module call including the arms that never
read it**, then try seven arms in a **load-bearing order** — the file says so, "the
arms are not disjoint" — each doing `full == "satellite…"` or
`path[0] == "satellite" && path[1] == "…"` string comparisons, with `module_console`
tried **sixth** and the free function `search_threshold_call` seventh.

So the fastest identity the language had was compiled into a header, asserted over,
documented at length, and then the hot path compared strings.

Frames tell the other half of the story. Resolve turns local names into integer slot
indices statically, once, before anything runs — and it is the fastest part of the
interpreter, measured at 7.1× (DESIGN §7.2).

**Names got numbers and got fast. Paths got numbers and stayed strings.** The whole
thesis of the second satellite is: finish the job the numbering started. DESIGN §4
is that design.

---

## 2. Architecture

### 2.1 The options considered

Bytecode was ruled out by the brief — meaning the *machinery*: an instruction
stream, a decode loop, a compile step. **The part of a bytecode that earns its
keep is kept**, and DESIGN §4 is where it went: every language-owned operation has
a small integer, so dispatch is an array index. What remains:

| | approach | verdict |
|---|---|---|
| 1 | naive tree-walk (`variant` visit, `shared_ptr` children) | what v1 is — the baseline we are beating |
| 2 | **closure compilation** — walk once, emit a tree of callables with every static decision baked in | **adopt** |
| 3 | **flattened arena AST** — nodes contiguous, children by `uint32_t` index | **adopt** |
| 4 | **self-specializing nodes / inline caches** | **adopt, on call sites** |
| 5 | explicit control stack (CEK) — pausable, resumable | **defer** — §2.5 |
| 6 | graph reduction | no — for lazy functional languages |
| — | copy-and-patch (pre-compiled machine-code stencils) | out of scope — it is a compiler |

### 2.2 The arena, first

The first satellite's tree is `shared_ptr<const Expr>` with `sizeof(Expr) == 96`,
guarded by a static_assert. That means:

- a **cache miss per child**, since children are wherever the allocator put them
- **96 bytes per node**, most of it paid by every node to fit the widest alternative
- a refcount on every node that **buys nothing**: the tree is immutable, lives as
  long as the program, and nothing ever frees a node early

*(Corrected 2026-08-26.)* This list used to claim an atomic refcount touch per node
visit and cross-thread contention from it. Neither is true of the first satellite:
`eval` takes `const Expr &` and `exec` takes `const Stmt &`, and every descent
dereferences rather than copying the handle, so the walk touches no refcount at all.
The layout costs are real; the atomic ones were not, and a plan that beats a
strawman cannot tell you whether it won.

An arena of PODs indexed by `uint32_t` fixes all three. Multi-threaded walking
becomes **atomic-free, not merely safe** — which matters because
`satellite.variable.thread` is on the roadmap (DESIGN §10.4).

It also kills a documented data race. `Name::slot` is `mutable int` on a
`shared_ptr<const Expr>`, and the comment holding the race off reads: *"resolve()
must finish, on one thread, before any evaluation begins."* With an arena and a
separate resolved side-table indexed by node id, that race is **structurally
impossible** rather than documented.

### 2.3 Closure compilation, and why it is not bytecode

**One thing here *is* the bytecode, and it is not this.** DESIGN §4's numbering does
the job an opcode table does — every language-owned operation gets a small integer
so dispatch is `handlers[path_id]`, one array index. What satellite never builds is
the *rest* of a bytecode VM: the linear instruction stream, the decode loop, the
serialised format, and the compile step a user waits for. The numbers exist before
any program does, because they belong to the namespace rather than to a program.

With that said, closure compilation:

- no linear instruction stream
- no opcode decode loop
- **no serialised form of the closure tree** — it is built in memory every run
- **no compile step the user ever runs or waits for**

*(Corrected 2026-08-27.)* This list used to say "no serializable format" flat, and
that is no longer true: [SATC.md](SATC.md) specifies `.satc`, a written-down form
of a program with its language-owned words replaced by their numbers. The
distinction the line was reaching for is real and now stated precisely — what is
never serialised is the **closure tree**, because writing that down would freeze an
implementation. `.satc` serialises the layer above it, which is just the source
with DESIGN §4's dictionary already applied, and it is a cache: deleting every one
of them costs a walk and nothing else.

The closure build is the same pass as resolve, measured in microseconds, and it
happens between "parsed" and "running" exactly the way resolve already does. From
outside, `satl file.satl` is as interpreted as it ever was.

What it buys: every decision that *can* be made before execution *is*. Which frame
slot. Which PathId. Which handler. How many arguments, already checked. At runtime a
node is one indirect call with no tag test and no re-resolution. 2–5× over a naive
walk is the usual figure — **to be measured here, not quoted.**

### 2.4 Inline caches

A `Call` node caches the resolved PathId and handler pointer on first execution,
behind a guard. This is what permanently retires the seven-arm chain of §1.1: the
second execution of a call site does no lookup at all.

### 2.5 Why the explicit control stack is deferred, not dropped

A CEK machine would make execution **pausable and resumable**, which is what you
want for green threads, generators, a stepping debugger, Ctrl-C at an arbitrary
point, and driving the interpreter from a GTK idle callback with no second thread.

It also costs against direct recursion — the figure usually quoted for it is 2–3×,
and **that one is borrowed rather than measured**, which by §9's own rule means it
decides nothing here until this project measures it. It is genuinely hard to write.
So: not now. But **bound the recursion depth from M7 onward** so deep recursion produces a
clean `capsule call too deep` error rather than a segfault. DESIGN §7.5 has the
numbers, and the first satellite's `system_facts/stack_facts.cpp` is half the
machinery already.

### 2.6 Order of adoption

**Arena first** — it is a data-layout decision and everything else rides on it. Then
closure compilation. Then inline caches. Doing them in the other order means doing
the arena twice.

---

## 3. The line rule

**Try to build for 300 lines.**

*(Softened from "no C++ file exceeds 300 lines" on 2026-08-28, by the author, and
the word that changed is `try`.)* It is a target to write toward, not a ceiling
that fails a build — the number is tighter than the first satellite's 325 and it
is aimed at the shape a file comes out in, which is a thing a person judges.

The first satellite arrived at its ceiling late, by splitting a 2208-line
`eval.cpp` and two 700-line functions *after* they had been written — and the
splits are visible in the result, with headers named `eval_internal.hpp` existing
to hold what an anonymous namespace used to, and comments explaining that "the
bodies below are UNCHANGED; they moved."

**Splitting a file after the fact preserves its shape. Writing to a target changes
the shape.** So the target is what you aim at from a file's first commit rather
than something applied at the commit where somebody notices — and that is the
whole of what it is for. **A file that goes over is not wrong; a file that got
long because nobody was aiming is.** Split by SUBJECT when you split at all: a
seam chosen to satisfy an arithmetic is a seam in the wrong place, which is the
failure the first satellite's `eval_internal.hpp` records.

Consequences to plan for rather than discover:

- **An umbrella header is a legitimate answer.** One door that includes its parts in
  an order that compiles, so every consumer's include line stays the same.
- **A dispatch table is a legitimate answer and a chain of `if` arms is not.** This
  is the same change §1.1 demands for other reasons — `handlers[path_id]` does not
  grow with the number of paths, so the file holding it does not either.
- **`words.def` is the one file that may exceed 300 lines.** It is data, not code,
  and splitting a numbering whose meaning is registration order is the one split
  that could silently change what a program means (DESIGN §4.3).

Markdown is not C++. This file and DESIGN.md are not bound by the rule.

The same argument produces the same shape one level up: **the `Makefile` is an index
over `make_support/`, and `satellite_enterprise/install.sh` is an index over
`install_support/`.** Both start that way rather than arriving there.

---

## 4. The build

### 4.1 Two version numbers, moving at different rates

`SATELLITE_VERSION` (003) is the **language**; it changes rarely and deliberately.
`SATELLITE_REVISION` (01) is **this build of it** and goes up as work lands. 001 was
what the first satellite carried while its design was being written, 002 is what it
became, and 003 is the second satellite — the number moved because the language is
being rebuilt, not because this build is newer.

They arrive as `-D` on the four compile recipes that need them — `main.o` and
`opening.o` in each of the two variants — and are **deliberately not in `CXXFLAGS`**, because `CXXFLAGS` is what `.cxxflags-stamp` records and a build stamp
that changes every second would recompile the whole tree on every invocation,
permanently silencing the one check that exists to catch a real flag change.

`satl --version` prints **both the path make invoked and the `__VERSION__` the
compiler reported**, because they are different facts. This project has already been
bitten once by the difference: `LLVM_BIN` pointed at a directory that no longer
existed, `c++` answered instead, and every figure attributed to clang was GCC's with
nothing anywhere saying so.

It caught a second, milder case on 2026-08-26: `CXX=clang++` is exported in this
machine's environment, so `origin CXX` is `environment` rather than `default` and
`010-compiler.mk`'s wildcard never fires. `__VERSION__` proved it was the same
compiler anyway. **That is what printing both fields is for.**

### 4.2 Two microarchitecture builds, and the program that chooses

*Landed 2026-08-26.* `make` produces **three binaries on x86-64 and one everywhere
else**:

| binary | built with | what it is |
|---|---|---|
| `satl` | no `-march` | the baseline build; runs on any x86-64 |
| `satl.haswell` | `-march=x86-64-v3 -mtune=haswell` | for Haswell (2013) and newer |
| `satl-cpu-level` | no `-march`, deliberately | prints `haswell` or `baseline` |

`satellite_enterprise/install.sh` runs `satl-cpu-level` and installs the answer as
`$HOME/.satl/satl`. The machine gets the fastest build it can actually execute, and
neither build has to be a compromise for the other.

**`-march=x86-64-v3` and not `-march=haswell`, and that is the correctness argument
for the whole mechanism.** `-march=haswell` additionally licenses FSGSBASE, PCLMUL, RDRND and
XSAVEOPT on both of this project's compilers, plus INVPCID on clang 24 and HLE on
gcc 17. *(Measured 2026-08-26 by diffing each compiler's own enabled-feature set;
AES is in neither, and needs an explicit `-maes`.)* Every real Haswell has them, so the flag is not *wrong* — but the check
on the other side, `__builtin_cpu_supports("x86-64-v3")` in
`src/programs/cpu_level.cpp`, does not cover them. The set the compiler may emit
from would then be a strict superset of the set that was verified, and **that gap is
where a SIGILL on somebody else's machine comes from.** `-march=x86-64-v3` closes it
by construction. *Check what you compiled for; compile for what you check.*

`-mtune=haswell` is separate, because tuning is not licensing: it reorders and
schedules for Haswell and emits no instruction `-march` did not already allow, so a
Zen 4 running this build runs correct code that was merely scheduled for a different
pipeline. It is the right guess, since Haswell is the oldest machine that can run it.

**The detector is compiled at the baseline and must stay there.** It is the one
binary that runs before anything is known about the machine, so an `-march` that let
the compiler emit a single AVX2 instruction *there* would turn "you get the portable
build" into a SIGILL on the machine that needed to be told that.

**The build asks the compiler what it targets, not `uname` what the machine is** —
`$(CXX) -dumpmachine`. The two answers differ under every cross build, and the flags
go to the compiler, so the compiler decides. On aarch64 and ppc64le — both of which
Enterprise Linux ships — `-march=x86-64-v3` is not a flag the compiler will take, and
there is nothing for a second build to be second to; those machines build one `satl`.
An unrecognised or empty triple takes the one-build branch too, because one build is
the answer that is never wrong and two builds on a machine we could not identify is a
guess.

**The installed binary is its own record.** `SATELLITE_BUILD_FLAGS` is baked in per
variant, so `satl --version` prints which of the two it is for the rest of its life.
The install therefore writes **no manifest**, and cannot have one that disagrees with
the file it describes.

Measured on this machine, 2026-08-26 — Intel Xeon E5-2670 v3, which is Haswell-EP:

- `satl-cpu-level` prints `haswell`, and `satl-cpu-level --explain` prints the three
  levels it read — `x86-64-v2 yes  x86-64-v3 yes  x86-64-v4 no` — so this box gets
  the haswell build.
- The flag reaches the compiler: a vectorizable test loop compiles to **17 `ymm` /
  `vfmadd` / `vmovups` instructions with `-march=x86-64-v3` and 0 without.**
- `satl` contains **no v3 instruction at all**; `satl.haswell` contains **21** — 16
  `vmovups` on 256-bit `%ymm0` and 5 `vzeroupper` — and every one of them is inside
  `satellite::version_text`, where the compiler vectorised a string copy. That is
  the honest state: the flag reaches the code and changes it, on the one function
  M1 gave it to work with, and there is still no speed claim to make, because
  nothing measurable runs there. The payoff arrives with DESIGN §8.1's base-10⁹
  loops.

Building the variants before there is anything to speed up is the same decision as
measuring startup at M1 (§4.3): put the guardrail in before there is a language to
regress it.

Where it lives: `make_support/045-microarchitecture.mk` (the flags, the triple test
and the object lists), `src/programs/cpu_level.cpp` (the detector), and a second
stamp file `.cxxflags-stamp-haswell` — two stamps rather than one, because a shared
stamp would rebuild the baseline objects whenever the haswell flags changed, and
because a single file's contents could only ever describe one of the two variants.

### 4.3 The startup budget

Measured at M1, with `satl` doing nothing, so that every later milestone has a floor
to be compared against and a regression has somewhere to be attributed. Best of five
runs of 200 invocations:

```
bare int main(){return 0;}          1.74 ms
satl (M1, opening information)      1.75 ms
satl --version                      1.75 ms
```

So satl's own share of starting up is about **0.01 ms**, and `ldd satl` lists **6
shared objects.**

The numbers live in `make_support/040-sources.mk`, beside the decision they justify
— not in a commit message, where nobody looks for them again.

**This binary links no GUI, and that is measured rather than tidy-minded.** The first
satellite's `satl-term` resolves 79 shared objects and maps 78 on this machine
against `satl`'s 6, and the dynamic linker loads every one before `main()` on every
run. The first satellite measured that at **25.9 ms with the GTK link against 2.5 ms
without**, of which 23.4 ms is the linker and the interpreter's own share of hello
world is 0.3 ms. Rendering was never the problem.

### 4.4 The window needs `dlopen`, not the binary split

The first satellite fixed the startup cost by making `satl` and `satl-term` two
binaries. That works for a terminal host, which never interpreted anything anyway.

**It does not work for `satellite.window.new()`**, which runs inside a user program,
which runs in `satl`. You cannot split that out.

The answer is to **`dlopen` a `libsatellite_window.so` the first time a window path
executes.** Startup stays at this tree's own floor — 1.75 ms (§4.3) — for every
program that never opens a window, and the 23 ms is paid only by programs that do.

This is a port rather than an invention, and the first satellite deserves the
credit: its `design/11-build-order.md` already schedules "a `dlopen`ed shim" as its
own M7, with the done-when condition that `satellite.include(satellite.window)`
opens a window *and* `ldd satl` still lists six objects. It was planned there and
never built; it gets built here.

`satl-term` stays, and keeps its name. `satl` keeps its name.

---

## 4.5 The machine limits, and the file that holds them

*(Asked for 2026-08-27. Specified here; not built.)*

satellite reads a **`satellite_config.ini`** and holds itself to what it says:

```ini
THREAD_COUNT=24     # hardware threads to use
CORE_COUNT=12       # physical cores
MEMORY_MAX=...      # satl must not exceed this
```

**This machine, measured 2026-08-27** — `lscpu` and `/proc/meminfo`: Intel Xeon
E5-2670 v3, 1 socket, **24 hardware threads, 12 physical cores**, MemTotal
64946428 kB = **61.9 GiB**.

### 4.5.1 What gets threaded, and what must not

The request was that converting `satellite.something.something` into integers be
threaded — *"create that many threads and then hand them each a line that begins
with satellite, or the fastest possible way to do it, whatever that may be."*

**The line-per-thread part is right and the startup part is not**, and §4.3 is why.
satl starts in 1.75 ms of which its **own share is 0.01 ms**. Spawning 24 threads
costs **690 µs measured** — seventy times satl's entire current startup — and the
fixed word table is 254 nodes, which a single thread walks in microseconds.
**Threading the table at startup is a guaranteed loss**, and M2 settled it a second
way: the table is `constexpr` and lands in rodata, so there is no startup work left
to thread.

Threading the walk over a *user's source* is a different question, because that
work scales with the program and the table does not. The number that decides it is
the **crossover**: how many satellite-rooted source lines a program needs before 24
threads beat 1, counting thread creation. **Measured 2026-08-28, below.**

So the shape to build toward: a pool that is **created lazily, on first real
threaded work**, sized from `THREAD_COUNT`, and shared by everything that needs
threads — the console's printer thread (DESIGN §10.1), parse-time interning, and
`satellite.variable.thread` at M12. One pool with three tenants amortises a cost
that none of them could justify alone, and a program that never threads never pays.

**Measured 2026-08-28, and the pool is not an optimisation — it is the thing that
makes the request worth honouring at all.** M2 built the walk, so the crossover
this section had been holding open since 2026-08-27 could finally be taken against
the real code path rather than a stand-in: `words::walk()` over real paths of the
real language, on this machine, 24 hardware threads, best of 15 runs, load average
under 1 before starting.

| satellite-rooted lines | 1 thread | 24 fresh threads | 24 from a pool |
|---:|---:|---:|---:|
| 150 | 41 µs | 695 µs (0.06×) | 48 µs (0.85×) |
| **180** | 51 µs | 692 µs (0.07×) | **48 µs (1.07×)** |
| 300 | 90 µs | 684 µs (0.13×) | 47 µs (1.90×) |
| 2,500 | 676 µs | 717 µs (0.94×) | 129 µs (5.25×) |
| **2,800** | 778 µs | **709 µs (1.10×)** | 125 µs (6.21×) |
| 50,000 | 13,264 µs | 1,688 µs (7.9×) | 1,124 µs (11.8×) |

**Two crossovers, and they are fifteen times apart:**

- **Spawning 24 fresh threads breaks even at about 2,650 lines.** Creating them
  costs **~690 µs** flat — which is the 0.5–1.5 ms this section estimated,
  measured — and that cost does not move with the size of the program, so the
  whole of the left-hand column is spent paying it back.
- **Waking 24 threads that already exist costs ~47 µs and breaks even at about
  170 lines.**

**THE 170 IS A SECOND-BATCH NUMBER, AND SAYING SO IS THE WHOLE OF WHAT THIS
MEASUREMENT IS WORTH.** *(Corrected 2026-08-28, hours after it was first written
here.)* The pooled column above was timed with the pool **already built**, because
that is what "waking threads that already exist" means. **Creating those parked
threads costs ~590 µs** — measured separately, best of five — which is the spawn
cost again, near enough. So a pool's FIRST batch pays it:

| | 2,500 lines | 2,800 lines |
|---|---:|---:|
| 1 thread | 676 µs | 778 µs |
| pool, **cold** — create + wake + work | 712 µs | 708 µs |
| pool, **warm** — wake + work | 129 µs | 125 µs |

**A cold pool crosses at ~2,650 lines, which is the spawn figure**, and it could
not be otherwise: a cold pool *is* 24 fresh threads plus a cheap wake.

**So the pool does nothing for a program that threads exactly once**, and that is
the common case this section was reasoning about — `satl program.satl` parses one
file and exits, and parse-time interning is likely to be the FIRST threaded work in
the run, so it pays the creation itself and crosses at ~2,650 like everything else.

**What the pool actually buys is the second tenant onward**, which is the argument
this section already made from first principles and can now put a number on:
*"one pool with three tenants amortises a cost that none of them could justify
alone."* The tenants that collect the ~170 figure are the ones that are not first —
the console's printer thread if it started earlier, `satellite.include` of another
file, M11.B's prompt parsing repeatedly, and M12. **The lazy pool is right and the
reason is amortisation across a run, not a cheaper parse.**

### 4.5.1.1 Warming the pool at startup — measured, and it beats the lazy rule

**The author's question, 2026-08-28: if satl is started we must assume a `.satl`
file is coming, so should the pool not be warming from the moment satl starts?**
It is the right question and it makes §4.5.1's *"created lazily, on first real
threaded work"* the wrong rule. Three arms, same corpus, same machine, best of
seven, load average 0.02:

| satellite-rooted lines | single | lazy — build at the parse | **eager** — warm from startup |
|---:|---:|---:|---:|
| 100 | **27 µs** | 672 µs (0.04×) | 48 µs (0.57×) |
| 1,000 | **309 µs** | 742 µs (0.42×) | 491 µs (0.63×) |
| 2,000 | **602 µs** | 716 µs (0.84×) | 622 µs (0.97×) |
| 2,500 | 733 µs | 745 µs (0.98×) | **641 µs (1.14×)** |
| 4,000 | 1,113 µs | 715 µs (1.56×) | **675 µs (1.65×)** |
| 50,000 | 13,745 µs | 1,882 µs (7.3×) | **1,825 µs (7.5×)** |

*Eager is: spawn ONE thread at startup, which builds the other 23; the main thread
starts walking immediately and the pool takes the remainder once it exists.*

**Three findings, and the first two say the author is right.**

- **Lazy is dominated. Eager beats it at every size measured**, because the main
  thread never waits for the pool. Kicking the build off costs the main thread
  **~20 µs**, not the ~590 µs it costs to build the pool yourself.
- **The crossover moves in, from ~2,650 to ~2,300 lines.** By the time the pool is
  ready — ~600 µs — a single thread has already walked ~2,150 lines, so anything
  smaller finishes before the pool exists. That is not a flaw in the scheme; it is
  the ceiling on what any scheme can do.
- **But warming is NOT free, and this is the part the argument gets wrong.**
  Eager runs at **0.42×–0.60× of single-threaded between 100 and 1,000 lines** —
  reproduced twice. Creating 23 threads is `mmap` and `clone`, which contends with
  the main thread's own allocation and page faults on the process's memory locks
  while it walks. **The main thread is slowed by warming a pool it will never use.**

**And file I/O hides none of it.** Reading `example/hello_world.satl` takes
**4.4 µs** and reading a 35 KB file takes **8.1 µs** — measured. There is no
startup window to hide 600 µs of thread creation behind. The only thing long
enough to hide it behind is the walk itself.

### 4.5.1.2 The pool starts at startup, always — the author's decision

**Decided by the author 2026-08-28, and it supersedes both §4.5.1's "lazily, on
first real threaded work" and the size rule §4.5.1.1 first proposed:**

> *satl almost must start the THREAD_COUNT in satellite_config.ini because we
> almost have to assume the user will call parallel_for, so that requires a warm
> pool no matter what; in other words, MOST satellite code will require 24, so
> start them now.*

**This changes the question, and the measurements above were answering the wrong
one.** Everything in §4.5.1.1 measures the pool against *parse-time interning* —
27 µs of work for a small program, which nothing can usefully thread. The pool's
real tenant is the **running program**, and a program's parallelism has nothing to
do with the size of its source: ten lines can run a million-iteration loop. **A
size trigger predicts parse cost and is therefore the wrong trigger**, and it is
also the kind of hidden threshold DESIGN §1.1 refuses — *do absolutely everything
for the user, and pay for it in performance rather than in their attention.*

**The arithmetic supports it once the base is stated in absolute terms rather than
as a ratio.** §4.5.1.1's frightening "0.42×–0.60×" is a ratio on a very small base:

| | eager costs a program that never threads |
|---|---:|
| 100 satellite-rooted lines | **+21 µs** |
| 500 | **+47 µs** |
| 1,000 | +182 µs |
| against satl's whole process startup (§4.3) | 1,750 µs |

So a program that never threads pays **1–3% of startup**, and a program that does
avoids **~590 µs of blocked execution** at its first parallel call. The trade is
worth taking well before "most" — one program in four is enough.

**Two implementation requirements this decision carries**, both measured above:

- **The main thread must not build the pool itself.** Spawn one thread and let it
  build the other 23: that is ~20 µs on the main thread instead of ~590 µs.
- **`THREAD_COUNT` has to be readable before the pool is built**, and
  `satellite_config.ini` does not exist yet (§4.5.4). Until it does, the count
  falls back to what the OS reports — 24 on this machine, and **not** cores × 2,
  which is this CPU's SMT ratio and not a rule.

**Two facts the author should have in view, neither of which blocks the decision:**

- **`parallel_for` does not exist.** It is in no numbering, no milestone and no
  document; the language's whole parallelism surface today is
  `satellite.thread.new` `1 23 1` and `satellite.variable.thread.start()` /
  `.join()` `1 6 13 1`–`1 6 13 2`. The decision above assumes a construct that has
  to be designed, numbered and built, and that work is not scheduled anywhere.
- **QUAD spawns zero threads.** The only `<thread>` use in its 3,029 lines is
  `sleep_for` (QUAD.md §2, checked against the source). The one program this
  language exists to express would never touch the pool — which does not make the
  decision wrong, but does mean *"most satellite code will require 24"* is a
  statement about where the language is going rather than about what it runs today.

**The `.satc` write is untouched by all of this and stays threaded always.**
SATC.md §5 puts the write on its own thread so the run never waits for it — that
is **one** thread hiding disk latency, not twenty-four splitting work, and it is
worth its ~25 µs at any program size. *"Thread the conversion no matter what"* is
right about the write and wrong about the walk, and they are different jobs.

**And below ~170 lines the walk must stay single-threaded**, which is a rule the
pool's owner has to enforce rather than a suggestion: at 80 lines the pooled arm is
still 0.48× — it takes twice as long as doing the work — and hello world is one
line. The speedup tops out near 12× on 24 hardware threads, which is 12 physical
cores answering, exactly as §4.5's table says the machine is built.

*(The measurement that matters is the pooled one, and it is the one this section
proposed before it had a number. The benchmark is
`SCRATCH.md`-adjacent scratch and is not in the tree; what is in the tree is the
result, here, beside the decision it justifies.)*

### 4.5.2 Knowing what satl is using

**The first satellite already built most of this**, in
`old_versions/first_satellite/src/system_facts/memory_facts.cpp`:

- `process_memory_bytes()` reads `/proc/self/statm` field 2 × page size, fresh
  every call, never cached — its own comment says *"the question is what this
  program is using NOW."*
- `start_memory_watchdog()` runs a detached thread that wakes once a second, reads
  a threshold, compares, and exits through a hook that restores the terminal first.

**Its policy is the opposite way round.** v1 watches the *machine's* available
memory against a floor (`min_free_mb`, default 4096); MEMORY_MAX bounds *satl's
own* use. Both are defensible and they are not the same guarantee — the second is
the stronger promise and the easier one to explain, and `process_memory_bytes()` is
already the call that implements it. **Keep the thread, keep the exit path, change
what it compares**, and ideally keep both checks.

### 4.5.3 A file is not the only place a setting lives

v1's watchdog does not read a file. It reads
`satellite.library.system.min_free_mb` — the language's own namespace, which a
running program can read and retune — and `Number` does the same for
`division_digits`. Both are numbered: `1 14 2 3` and `1 14 2 1`.

The two answer different questions and are not in conflict: **the file is where a
machine's settings live before a program starts**, and is what an installer writes
and a person edits; **`satellite.library.system.*` is how a running program reads
and changes them.** The obvious arrangement is that the file seeds the namespace at
startup and the namespace is what everything reads afterwards — one authority at
runtime, one place to edit at rest. Not yet decided.

### 4.5.4 Open

- **What unit is `MEMORY_MAX` in, and what is the default?** 61.9 GiB and 64.9 GB
  are the same memory. Should the shipped default be the whole machine or a
  fraction, and does a machine with less than the file claims win?
- **A setting is not a fact.** `arguments.machine.threads` (DESIGN §7.7) asks what
  the machine *has*; `THREAD_COUNT` says what satl may *use*. If they are ever
  allowed to differ they need two names, and a program asking the first must never
  get the second.
- **The file does not exist yet.** Nothing reads it and nothing writes it. The
  installer (§5) is the natural author.

---

## 5. Installing

`satellite_enterprise/` is the Enterprise Linux installer — written and tested on
AlmaLinux 10.2, aimed at the RHEL family, and it **says so and carries on** if it
finds itself elsewhere, because refusing on the strength of a name in
`/etc/os-release` would be a policy dressed up as a check.

`install.sh` is an index over eight fragments in `install_support/`, POSIX sh, and
**needs no root ever** — everything it writes is under `$HOME`. That is a real
simplification over the first satellite's installer, which had to compile as the
human and copy as root; the fixed per-user root is what buys it.

**The root is `$HOME/.satl` and it is not a prefix.** The binary is
`$HOME/.satl/satl` rather than `$HOME/.satl/bin/satl`: a prefix layout exists so
many packages can share `bin/`, `lib/` and `share/`, and nothing shares this
directory. One name to remember, one directory to delete. `share/` underneath it
keeps the XDG-relative paths so that publishing to the desktop is a link with the
same relative path on both sides.

Twenty files: the chosen `satl`, the `.satl` mime packet, and nine icon sizes in
both `apps` and `mimetypes`.

### 5.1 The rules it inherits

- **Every command that changes anything goes through `run()`**, which is what makes
  `--dry-run` a complete and honest transcript rather than an approximation. Purely
  read-only helpers — reading `/etc/os-release`, sorting the install list — run
  directly and deliberately, because a transcript of them is noise, and because a
  `--dry-run` that could not read the machine could not tell you what it would do.
- **Nothing edits `.profile`, `.bashrc` or any other file the user owns**, and
  nothing tells the user to export a variable. A child process cannot change its
  parent's environment, so an installer that "exports PATH" exports it into a shell
  that exits one line later; editing a login file is a permanent change made by a
  program somebody ran once, and `--uninstall` cannot reliably undo it.
- **`rmdir`, never `rm -rf`.** An uninstall removes files by name and directories
  only when they are empty, so anything the script did not install survives and is
  *reported* rather than discovered later.
- **The install tree is declared exactly once**, in `060-install-tree.sh`, and read
  by both the install and the uninstall. Two lists is how an install tree rots. When
  the Makefile grows an `install` target, that function is what should be **deleted**
  rather than duplicated.

### 5.2 What it will not do on its own

`--link` (a symlink in `~/.local/bin`) and `--desktop` (the icons and file type in
`~/.local/share`) are **off by default**, and the reason is measured rather than
principled.

This machine already has `~/.local/bin/satl` — 968536 bytes, the **first**
satellite's interpreter — plus `satl-term`, the nine hicolor sizes, an `index.theme`
and the mime packet, all installed there by that satellite's own installer. There is
a third install at `/usr/local/bin/satl`, root-owned, and because `/usr/local/bin`
precedes `~/.local/bin` on this PATH, **`satl` resolves to the first satellite** for
anything that searches PATH. *(Not in this user's interactive shell, where `.bashrc`
aliases `satl` to this tree's build and an alias beats PATH — so a terminal test and
a script test answer differently. Noted 2026-08-26.)*

Defaulting those flags on would have replaced a working interpreter with an M1 build
that cannot interpret anything, on the first plain `./install.sh`. So they are
opt-in, and even when asked for the script **refuses every path it does not already
own** — verified: all twenty refused, nothing overwritten. It creates only symlinks,
which is what makes ownership unambiguous when another install owns real files at
the same paths.

That is DESIGN §1.1's rule applied to an installer: do everything for the user, and
never anything behind their back.

### 5.3 The artwork

`satellite_enterprise/icons/` is the installable tree, whose layout mirrors the
install destination exactly, so installing is a copy and not a translation.
`satellite_enterprise/icon_artwork/` is the source work, including the `.xcf`.
Everything was copied byte-for-byte from the first satellite and verified with
`cmp` — 43 files, no re-encoding.

Two names must not drift: `org.satellite.terminal` (the app, matching the
GApplication id and the `.desktop` filename) and `application-x-satellite` (the mime
icon, matching the type with `/` replaced by `-`).

**The SVG and the PNGs must never both be installed.** The icon theme spec lets
either satisfy a lookup, so shipping both makes which one a shell draws
unpredictable. The artwork is a photograph, which has no scalable form, so the PNGs
are the ones that ship and `org.satellite.terminal.svg` travels in the tree
uninstalled. The `.desktop` entry was likewise held back until a binary existed for
it to launch — a launcher for a missing program is a menu entry that does nothing.
**M11.A built that binary on 2026-08-27**, so the entry is now installable and the
remaining step is naming it in `060-install-tree.sh`, which is the one declaration of
what gets installed.

`satellite_enterprise/icons/application-x-satellite.xml` carries several findings in
its own comments —
why `generic-icon` points at our own icon, why `--` is banned inside its comments,
why the magic offset window is 4096. **Read them before changing anything about
icons or the mime type.** Each was found the hard way.

---

## 6. What we keep from the first satellite

Ported, adapted, or taken as-is:

- **The syntax and the generating rule.** It is coherent and it is the identity.
  DESIGN §1–§6.
- **Resolve and frames.** Names to integer frame slots, statically, before anything
  runs. The best engineering in the first satellite, and measured. DESIGN §7.
- **Frames as isolation, the library as the atomic global.** The `SLOT_GLOBAL` /
  `SLOT_CAPSULE` / `SLOT_METHOD` / `SLOT_SUIT` / `SLOT_FIELD` sentinel scheme.
- **Exact-decimal `Number` on base-10⁹ limbs, with `double` constructors deleted.**
  Refusing binary floats at the C++ type level is a genuinely good call and it is
  the foundation `satellite.variable.float` needs. DESIGN §8.1.
- **The Console with its own printer thread**, and the `drain()` barrier before
  reading input. DESIGN §10.1.
- **The SIGINT handling** — installed without `SA_RESTART`, using `eof()` to tell a
  real closed stdin from an interrupted read. Hard-won; do not rediscover.
- **The search power** — the comparator/walker split, deliberately free of the
  Evaluator class and *callable from anything*. The ten-level ladder is a general
  power that applies to any value the language has, present or future. Port it close
  to unchanged.
- **The X-macro registry mechanism**, ids frozen, append-only, static_asserts in the
  header so every consumer inherits them.
- **`format.def`'s rule**, verbatim: prose may explain a number, it may never be the
  only place the number lives.
- **Spans on every node.**
- **PCG, and the fast / normal / ultra tiers.** DESIGN §11.
- **The two-binary split and the startup measurement discipline.**
- **The comment culture.** Comments that state a number and where it came from,
  rather than an intent. Rare and valuable. Keep writing them.

---

### 6.1 `satellite_number` and `satellite_string` come across close to unchanged

*(Surveyed 2026-08-27.)* `src/satellite_number/` and `src/satellite_string/` exist
in this tree and are empty. What fills them is the first satellite's, and it very
nearly ports as-is:

| | files | lines | largest |
|---|---:|---:|---:|
| `satellite_number` | 10 | 1509 | `limbs.cpp` at 284 |
| `satellite_string` | 2 | 249 | `satellite_string.cpp` at 155 |

**Every file is already under §3's 300-line ceiling**, which is worth noticing
given §3's argument that a ceiling applied after the fact preserves a file's shape
rather than changing it. Here the shape already fits.

`satellite_number` is internally closed — every include is a sibling or the
standard library — so it ports alone. `satellite_string` needs `system_facts/`,
because its code table is not only characters: **codes 95–100 are live values**
resolved at decode time, and 97, 98 and 99 are `threads`, `mem_total_mb` and
`mem_used_mb` — the same facts §4.5 and DESIGN §7.7 reach for by two other routes.

Four things to settle before copying, and `SCRATCH.md/PORTING.md` has the detail:

1. **Does `Number` keep its reach into `satellite.library`?** It reads
   `division_digits` from there, which would drag the library registry in at M2,
   years before §8 schedules it. A compile-time default now and the lookup restored
   later is the alternative.
2. **Does the code table stay 16-bit?** The header argues it well and nothing in
   this design contradicts it. Port as-is unless something does.
3. **Where does `satellite.random` live?** It is `satellite_number/random.cpp` in
   v1; DESIGN §11 gives it a section of its own here.
4. **`sizeof(Number)` on arrival.** DESIGN §8.2 budgets a `Value` at 40 bytes with
   a static_assert to come. That is the one number that could make this port not
   fit, and it is cheap to check first.

---

## 7. What we throw away

- **String-keyed dispatch.** All of it: the joined path built per call, the ordered
  arms, the `full == "…"` ladders. This is the change everything else hangs off
  (§1.1).
- **`shared_ptr<const Expr>` for the tree.** §2.2.
- **`mutable int slot` on `Name`.** §2.2.
- **`EvalError{std::string, Span}`** and the 199 `fail()` call sites that each
  compose their own message string. DESIGN §9.
- **"Record the error and return `nullptr`."** The decision not to throw is kept and
  is measured; the 199 sites where a missed null check is a segfault are not. DESIGN
  §9.1.
- **Switching on raw `std::variant` indices.** The code documents this as a landmine
  — "APPEND ONLY", "silently renumber every alternative after it". Make adding an
  alternative a compile error.
- **The `bytecode_format` module *as named*.** The ids are right; the framing is
  wrong. It is not a bytecode format, it is the language's word-and-path registry,
  and it belongs in the hot path rather than in a serialiser. New home:
  `src/satellite_words/`.
- **The fixed four-segment `SAT_PATH`.** No word limit — the trie has no depth limit
  (DESIGN §4.5).
- **The `100ms` special case** in `eval_call`, which matches on the argument
  *expression* before evaluating arguments and hardcodes a string comparison against
  `satellite.console.display`. With a real registry, `display` **declares** that it
  accepts a pace argument.
- **Housekeeping.** No `.o` files committed beside sources. No tarball blobs in the
  tree. No scratch test directories at the root. Build output lives out of source.

---

## 8. Milestones

Each milestone is a thing that **works and can be demonstrated.** No milestone is
"the parser is half done."

**The numbers are assignment order and the list is build order, and since
2026-08-28 they differ.** Read the list top to bottom; the numbers are names, not
positions. That is WORD_NUMBERS §1.2's rule applied to the plan rather than to the
language: **never renumber, never reuse, and record the order where it can be read.**

**M8 is split, and only its second half moved.** *(2026-08-28.)* The empty
`satellite.container.list` that `satellite.main`'s parameter binds to was ruled
M10's, which would have put the whole of M8 after M10 and left **M9 and M10 with no
console to print through** — against this section's own opening rule. The parameter
was the only part that ever needed a list, so:

    M8.A   the console, satellite.main, satellite.return    between M7 and M9
    M8.B   hello world — DESIGN §3, and the parameter       after M10
    M8.5   satellite.help                                   after M8.B

**A bare "M8" in text written before 2026-08-28 means M8.A**, the console, which is
where nearly every citation of it points — MILESTONE.md's console rows, the console
draft in MILESTONE_DRAFTS.md, QUAD §4's *"nothing before M8"*. The exceptions are
the ones that name **hello world or DESIGN §3 as a done-when**, and those mean M8.B.
The permanent documents have been corrected; **the dated adversarial findings in
`SCRATCH.md/MILESTONE_DRAFTS.md` have not**, because they are verbatim records of
what a review said on a day and rewriting them would falsify the record.

**Eleven milestones were added on 2026-08-28, and the list below is the result.**
`SCRATCH.md/MILESTONE.md` had counted **121 of WORD_NUMBERS.md §2.2's 222 numbered
paths reached by no milestone at all** — more than half the language — and four
milestones drafted on 2026-08-27 to cover the largest blocks had been sitting in
`SCRATCH.md/MILESTONE_DRAFTS.md` ever since, because a lens found real errors in
every one of them and *a draft with a known error in it is worse in §8 than out of
it.* Those four are corrected and are here. The rest of the 121 are covered two
ways: by seven further milestones, and by **naming clauses added to milestones that
already owned the work and did not say so** — this document's oldest recurring
failure, and its cheapest fix, applied for the fourth and fifth time.

**They take the next numbers rather than decimals, and that is this section's own
rule doing what it promised.** M4.5, M6.5, M8.5 and M9.5 are interpolations from
before the rule at the top of this section existed. Once the numbers are names and
the list is build order, an inserted milestone does not need a number between its
neighbours' and is worse for having one: the console draft called itself **M9.25**
and then spent a paragraph arguing about what its own name becomes if
`MILESTONE.md` §0.4 moves M9.5 ahead of M9. **A name that changes when its
neighbours move is a position wearing a name's clothes.** M14 upward have no such
problem, and WORD_NUMBERS §1.2 is the same argument one level down.

    M14   the machine limits — the file, the pool, the ceiling   after M5
    M15   the variant, and what "nothing" is                     after M9
    M16   the clock and the dice                                 after M15
    M17   the console's other half                               after M16
    M18   persistence — files and directories                    after M8.5
    M19   the machine's facts in the language                    after M18
    M20   a piece of QUAD, running                               after M19
    M21   another file — satellite.include and satellite.analyze after M13
    M22   spacesuits                                             after M21
    M23   the network                                            after M22
    M24   Satellite Orbit and the wire format                    last

**"Later, in no fixed order" is gone, and emptying it is most of what this pass
did.** Every one of its eight entries now has a milestone that names it, which is
the only thing that was ever wrong with it: a pile is not an order, and this
section's own closing sentence had been calling it *"not a milestone"* while
`satellite.random`'s sixteen rows of working, tested v1 code sat in it.

**What this pass did not do is make the language smaller or the work smaller.**
Naming a path is not building it. Several of the milestones below are mostly a list
of decisions only the author can take, and they say so in their own done-when the
way M9.5 does; **§8.2 counts them** rather than letting a future audit rediscover
them. The claim this pass makes is narrower than "everything is scheduled" and is
the one the ledger asked for: *every numbered path is now named by exactly one
milestone, and every milestone that cannot start says what it is waiting for.*

**M1 — `satl` exists and says how to use it.** *Landed 2026-08-26.* §1.

**M2 — the namespace trie and the path interner.** *Landed 2026-08-28.*
- `src/satellite_words/words.def`, written as a **tree**: each entry names its
  parent, and its position among that parent's children *is* its number. It is a
  transcription of [WORD_NUMBERS.md](WORD_NUMBERS.md) and nothing else — that file
  is the authority and this one is the copy a compiler can check.
- the trie, the spelling interner, and `PathId`.
- `words.hpp` as the consumer, with the static_asserts **in the header** so every
  future consumer inherits them. What they check, given per-parent numbering: every
  parent's children are dense from 1 with no holes and no duplicates, every named
  parent exists, and no node is its own ancestor.

  **As first written that check failed, and it failed on the numbering it was
  checking.** *(Found 2026-08-27, resolved 2026-08-28.)* WORD_NUMBERS.md §2.2 holds
  **three duplicate numbers** and they are deliberate: `satellite.random.fast.range(min,
  max)` is `1 7 5`, `.normal.range` is `1 7 8` and `.ultra.range` is `1 7 11`, each an
  **alias** of the call shape above it (§2.3). They are the *only* duplicates in the
  language — every one of the 218-versus-215 disagreements across these documents is
  these three rows and nothing else.

  **So "no duplicates" is false as stated, and the check is these four properties
  instead**, which is the form QUAD.md §4 states and the form that survives:

  1. **No holes.** Every parent's children are dense from 1.
  2. **No duplicates *among non-aliases*.** Two entries may share a number only when
     one is declared an alias of the other.
  3. **No orphans.** Every named parent exists, and no node is its own ancestor.
  4. **No alias points at a number that does not exist.**

  **The load-bearing part is property 2's escape clause, and it is a `words.def`
  requirement before it is an assert requirement.** An alias has to be *declarable*
  in the file, or the check cannot tell a deliberate duplicate from a typo — and a
  check that cannot tell those apart is worth nothing, because the three real
  aliases would train whoever hits it to loosen the assert. §2.3's model is what the
  syntax must express: **an alias is one node with a second spelling, not a second
  node.** That also settles the count — 218 rows, 215 numbers — and the same
  mechanism carries §7.7's six spellings of `arguments`, which are the same shape at
  a different scale.

  **This no longer blocks M2.** Write the four properties, give `words.def` an alias
  form, and the transcription can start.
- **a digest over `words.def`**, so a `.satc` can name the numbering it was written
  against and a changed numbering stops every stale cache being read on the same
  instant. SATC.md §2 is why; M4.5 is where it gets used.
- **a live child counter on every node**, so any node can be asked for the next
  number free under it. See §8.1 below — this is not optional and it is what the
  milestone did not originally know about.
- `satl --words` dumps the tree with each node's number — **the registry gets a
  consumer in the same milestone it gets written**, which is the one thing the first
  satellite did not do. It shipped three commits where the registry had zero
  consumers, which is how two sections assigned kind 4 to different things and
  neither noticed.
- a test proving `satellite.console.display` walks to `1 5 1`,
  `satellite.random.normal` to `1 7 2`, and that both intern to stable `PathId`s.
  The numbers come from WORD_NUMBERS.md and the test is how we know the
  transcription did not drift.

**What landed, and the four things the milestone decided on its way.**
*(2026-08-28.)* `src/satellite_words/` is nine files plus `words.def`;
`tests/words_test/` is the tree's first test; `satl --words` is the consumer.

- **`words.def` holds 254 nodes and 9 aliases**, and 254 is a count no document
  had. §2.2's 219 numbers do not include the bare shape a `(0)` marker names —
  the marker rides on the row of the node it belongs to, so it is a number that
  section carries without counting. 216 numbered rows + 38 bare shapes = 254.
- **A number is a position and is not a column.** The `// 1 5 1` ending each row
  is a comment nothing reads. That closes FORMAT/CXX.md §9's first question and
  it is what makes the next point necessary.
- **Only one of this milestone's four properties is a `static_assert`, and that
  is the encoding paying off rather than three being forgotten.** No holes and no
  duplicates are BY CONSTRUCTION — a number is a position, so the file cannot
  express either. No alias pointing at a number that does not exist is BY THE
  COMPILER, because an alias names an identifier. **Only "no orphans, no node is
  its own ancestor" needed asserting.** `words_invariants.hpp` says which is
  which, and adds eight more the encoding needs and §8 could not have known to
  ask for.
- **The transcription is the one thing no assert can reach, so the test reads
  the authority.** A row left out of `words.def` does not leave a hole; it
  silently renumbers every sibling after it, and *both files stay internally
  consistent.* `tests/words_test` therefore opens WORD_NUMBERS.md, parses §2.2
  and walks all 222 paths. **Verified by mutation on 2026-08-28**: deleting one
  row — `satellite.console.typed()` `1 5 5` — compiles clean with every assert
  passing, and the test names the missing row and the four siblings it shifted.

**The call-shape question is answered and the two depths are one rule.**
WORD_NUMBERS §4 asked for confirmation *before* `words.def` encoded it, and §1.3's
definition of `(0)` on 2026-08-28 had already settled it: **a word reached bare
holds its shapes as children (`include()` is `1 1 0`); a word only ever called
does not exist apart from them, so its shapes are siblings (`input()` is
`1 5 2`).** `words.def`'s header carries the argument and `match_shape()` is the
rule in ten lines.

**M2 is not threaded, and that is a decision rather than an omission.** §4.5.1
already argues that threading the fixed table at startup is a guaranteed loss, and
M2 settled it twice over: the tables are `constexpr` and land in rodata, so there
is no startup work left to thread at all.

**And M2 is what finally made §4.5.1's crossover measurable**, because the walk it
is a crossover *of* did not exist until this milestone. Taken 2026-08-28 and
written into §4.5.1: **~2,650 satellite-rooted lines** before 24 freshly created
threads beat one, and **~170** before 24 threads from a warm pool do. That is the
number this plan had been holding open since 2026-08-27, and it says the
line-per-thread request only pays against the pool §4.5.1 already proposed.

**The seed is wide** — the whole first-satellite word surface, not just what M8–M10
needs. *(Settled 2026-08-27.)* This closes what this section used to hold open.
`SCRATCH.md/WORD_SURFACE.md` is the inventory it is seeded from: 111 real paths
found by sweeping the v1 registry, the v1 evaluator, every v1 `.satl` program, the
v1 design documents and this tree's own two, with each source swept once and then
attacked by a second reader looking for what the first missed.

**It got wider on 2026-08-27**, when settling QUAD.md §3 added 71 numbers and took
WORD_NUMBERS.md §2.2 from 144 entries to 215. Most of them are **selectors rather
than paths** — the map's nine methods and the string's sixteen, where both had none
at all — and WORD_NUMBERS.md §1.5 is the distinction: a path is numbered in a `.satc`
and a selector is numbered only for dispatch, because PLAN M4.5 writes the file from
the parse tree and a selector's identity is not known until M6's resolve. Both kinds
go in `words.def`; only one kind ever appears in a cached program.

Nothing executes. This is the spine.

### 8.1 The numbering is partly dynamic, and that changes M2

A user's capsules and spacesuits get numbers too — **the next one free under the
node that owns them, allocated when the name is first met** (WORD_NUMBERS.md §3,
DESIGN §4.3). `satellite.library.main` is `1 14 1` because the language put it
there; a user's `x` is `1 14` followed by whatever is free.

So M2 builds two things that look alike and are not:

| | the language's words | the user's names |
|---|---|---|
| numbered | ahead of time, in `words.def` | at parse time, as met |
| frozen | forever, across every program | for one run |
| checked by | `static_assert` in the header | nothing a compiler can see |
| a `PathId` is | stable and quotable | valid inside one run only |

Three things follow, and each is a way to get this wrong:

- **The header's static_asserts cover the frozen half only.** That has to be said
  in the header itself, or it reads as a guarantee about the whole trie when it is
  a guarantee about part of it.
- **Anything that persists a PathId must record the name instead.** Satellite Orbit
  and the wire format are the two that will want to, and both are far enough out
  that the rule needs writing down now rather than remembering later.
- **`satellite.library.<name>` means the library registry is reachable at M2**, at
  least as a counter, years before the milestone that builds it. ~~Decide whether M2
  owns a real allocator or a stub, and say which in the code.~~ **Decided
  2026-08-28: a real allocator.** The counter was never optional — this section
  requires it — and once it exists, refusing to hand out the number it is holding
  buys nothing and leaves a second thing to build later.
  `src/satellite_words/words_runtime.hpp` is the record, as this asked.
  **Its caller arrives at M4**, which now says so.

**M3 — the lexer.** ← next. Tokens, spans, the reservation rule. Known words carry their
node identity out of the lexer; user-owned bare words carry their text. DESIGN §5.

**It also owns `satellite.variable.binary` `1 6 5` and `.hex` `1 6 11`**, and that was
unsaid until 2026-08-27. DESIGN §8.5 makes them real types **with literals** —
`x00FF` and `b1010`, where **the width is part of the value**, so `x0009` is not `x9`.
A literal is lexed, so the lexer decides them whether or not a milestone says so.
`hexadecimal` is one of the language's aliases for `hex` (WORD_NUMBERS §2.3), which
the lexer's spelling table has to know.

**"One alias" was wrong and it was wrong in a way that hid work.** *(Corrected
2026-08-28.)* §2.3 has **three rows**, not one: `hexadecimal`; the three
`satellite.random.<tier>.range(min, max)` spellings, which are the only duplicate
numbers in the language and are M16's; and `arg` `args` `argz` `argument`
`arguments` `argumentz` — *one node, six spellings* (DESIGN §7.7), which is M19's
headline demonstration and M6's to recognise at resolve. **M2's `words.def` landed
holding nine aliases**, so the mechanism exists and what this milestone owes it is
the lexer's half of the spelling table.

**M4 — the arena AST and the parser.** `uint32_t` node indices into a contiguous
arena, no `shared_ptr` anywhere in the tree. `satl --unparse file.satl` round-trips,
which is how we know the parser is right before anything can run.

**It is also the first caller of M2's name allocator.** *(2026-08-28.)* §8.1
requires every node to keep a live count of its children so a user's capsules and
spacesuits can take the next number free under the node that owns them, allocated
**when the name is first met** — and the parser is what meets a name for the first
time. `words::Words::intern(parent, name)` is built, tested and called by nothing
until here. Two things M4 inherits with it: a name the language already owns under
that parent is **refused** rather than renumbered, and DESIGN §2's reservation rule
decides at M6 whether that refusal is the right policy; and a user's `PathId` is
valid **inside one run only**, so M4.5's `.satc` writer must record the name.

**It owns all eleven of DESIGN §6.1's segment-1 words, and naming them is a fix rather
than an addition.** *(2026-08-27.)* §6.1's table is authoritative: a segment-1 word
either has a parse rule of its own or it does not, and these are the ones that do —

> `variable`, `container` · `library` · `statement` ·
> `include`, `capsule`, `spacesuit`, `return`, `returns`, `protected`, `public`

Until now this milestone described the arena and the round-trip and referenced none of
them, so eight of the eleven appeared in `SCRATCH.md/MILESTONE.md` as work with no
milestone when the truth was **a milestone owned them and did not say so.** That is a
worse failure than an unscheduled namespace, because nothing looks wrong.

**§6.1's table is the checklist**, and it is how the last gap of this kind was found:
a word present in that table and missing from the numbering is a word the parser
cannot reach. All eleven are now numbered — `returns` at `1 21` was the last, and
`statement`'s four children are `1 13 1`–`1 13 4`.

`satellite.returns` is the one to be most careful with. DESIGN §13 records the
return-type syntax as **decided** — an optional `satellite.returns(TYPE)` after the
parameter list, defaulting to the `satellite` type, which leaves hello world
byte-identical — and §6's grammar already has the rule written. It is not new work;
it is work that had no name in this list.

**M4.5 — `.satc`.** The cache [SATC.md](SATC.md) specifies: check for a `.satc`
before walking a source, read it when its three header lines match, and write a
fresh one afterwards on its own thread. It lands **after M4** because it serialises
a parsed program and there is nothing to serialise before the parser exists, and
**before M5** because a malformed `.satc` is the first thing in the language that
has to say something to a user in plain words. Its digest covers `words.def`, so
M2 has to be able to produce one.

**M5 — the error reporter.** Built **before** the evaluator, deliberately. Codes,
spans, a source excerpt with a caret, notes with their own spans, and "did you mean"
over the trie level that failed. Every milestone after this reports properly from its
first commit. Retrofitting this is exactly how the first satellite ended up with 200
bespoke message sites.

**M14 — the machine limits: the file, the pool and the ceiling.** *(New
2026-08-28. After M5, before M6.)* `satellite_config.ini` (§4.5), the thread pool
§4.5.1.2 rules starts at startup **always**, the memory watchdog §4.5.2 describes,
and the three fact readers all of it is built out of. It closes three of
`SCRATCH.md/MILESTONE.md` §3's rows at once, and those three were the oldest
un-milestoned work in this plan — specified in §4.5 on 2026-08-27 and never
scheduled.

**Five numbered paths, and it owns one of them as behaviour**:
`satellite.library.system` `1 14 2 (0)` and its four dials — `division_digits`
`1 14 2 1`, `max_depth` `1 14 2 2`, `min_free_mb` `1 14 2 3`, `float_digits`
`1 14 2 4`. The node and the storage land here; **`min_free_mb` is the only one
whose meaning is this milestone's.** `division_digits` is M6.5's, because `Number`
reads it (§6.1, open question 1); `max_depth` is M7's by DESIGN §7.5, with M10's
search walk as a second consumer that must say it reads the dial M7 built rather
than inventing one; `float_digits` is M9.5's, and DESIGN §13 has already redefined
it from "the dial" into **the default length of a float's right half**. A milestone
that quietly built all four would be building three other milestones' decisions.

- **The file.** `THREAD_COUNT`, `CORE_COUNT`, `MEMORY_MAX`, read at startup and
  refused in M5's plain words when it is malformed rather than ignored — which is
  the same argument M4.5 makes about a bad `.satc`, one milestone earlier in the
  reading order and one later in this list.
- **The pool**, with §4.5.1.2's two implementation requirements as its
  specification rather than as advice: the main thread **spawns one thread which
  builds the other 23** — ~20 µs on the main thread instead of ~590 µs — and
  **below ~170 satellite-rooted lines the walk stays single-threaded**, which
  §4.5.1.2 says is a rule the pool's owner enforces. Both numbers were measured on
  2026-08-28 and both are in §4.5.1.1 beside the decision they justify.
- **The watchdog.** §4.5.2's instruction is *"keep the thread, keep the exit path,
  change what it compares, and ideally keep both checks"*, so both are here:
  `process_memory_bytes()` against `MEMORY_MAX`, which is the stronger promise and
  the one nothing has ever demonstrated, and `mem_available_mb()` against
  `min_free_mb`, which is v1's check ported as a regression.
- **The fact readers**, `old_versions/first_satellite/src/system_facts/`:
  `memory_facts.cpp` (236 lines), `host_facts.cpp` (44) and `stack_facts.cpp` (52).

**It lands before M6 because three later milestones cannot be built without it and
none of them says so today.** M6.5's `Number` reads `division_digits`. M7 bounds
recursion, and DESIGN §7.5 derives that ceiling from `RLIMIT_STACK` rather than
fixing it — which is `stack_facts.cpp`'s `stack_limit_bytes()`; M7's `Str` needs
`mem_total_mb()`, `mem_used_mb()` and `hardware_threads()` besides, because §6.1
records that `satellite_string`'s codes 97, 98 and 99 are **live values resolved at
decode time**. M8.A's printer thread is the pool's first tenant. That is the
machine draft in `SCRATCH.md/MILESTONE_DRAFTS.md` turned inside out: it claimed
these three files for a milestone after M10, and its own lens found every one of
them consumed at M6.5 or M7. **The seam is between the readers and the language
surface** — the readers are here, and `satellite.system`'s twenty-eight paths are
M19.

**`parallel_for` does not exist, and this milestone is where that stops being
invisible.** §4.5.1.2's decision — start the pool always, because most satellite
code will call `parallel_for` — rests on a construct that is in no numbering, no
document and no milestone, including this one. The pool is still right without it,
for the reason §4.5.1 gives on its own terms (amortisation across a run, the second
tenant onward), and **naming the missing construct is the honest form of the
dependency.**

**Blockers, all three of them §4.5's own and none of them this milestone's to
take:**

- **What unit `MEMORY_MAX` is in, and what its default is** (§4.5.4). 61.9 GiB and
  64.9 GB are the same memory. The watchdog's headline check cannot be written
  until this is answered, which is why it is a blocker and not an open question.
- **Whether the file seeds the namespace** (§4.5.3, *"Not yet decided"*). It is the
  difference between one authority at runtime and a reconciliation rule.
- **Whether a setting may differ from a fact** (§4.5.4). `THREAD_COUNT` says what
  satl may *use*; `arguments.machine.threads` (DESIGN §7.7) says what the machine
  *has*. The answer decides whether M19 builds one reader or two.

**Done when** `satl --limits` prints every value it is holding to and where each one
came from — the file, or the OS fallback §4.5.1.2 specifies, which is what the OS
reports and **not** cores × 2; a malformed file is refused with a code, a span and
a caret; `satl --words` shows the pool parked before the walk begins and the walk
itself single-threaded, because 254 nodes is far under the 170-line floor; and
**the watchdog fires** — `MEMORY_MAX` set below what satl is already using, exit
status 2, one line in plain words on stderr, and the process gone within a second
of the threshold being crossed. A watchdog that never fires is indistinguishable
from no watchdog, so the demonstration is the one that kills the process, and §9
means the *installed* binary.

**The terminal-restoring hook is not here, and that is a correction to the draft
this milestone comes from.** v1 leaves through `run_emergency_exit_hook()`, whose
only registrar anywhere in v1 is the line editor's raw mode — M11.B's. Until a
prompt exists nothing has put the terminal into raw mode and the hook has nothing
to undo, so a done-when clause asserting *the terminal is still usable* would be a
test that cannot fail. This milestone registers no hook; **M11.B inherits the exit
path and adds the registration**, and its line says so.

**M6 — resolve.** Names to integer frame slots. Capsules, frames, the `SLOT_*`
sentinels. Resolved data in a side table indexed by arena node id, not `mutable` on
the node. DESIGN §7.

**Three things it owns that its own line did not say.** *(2026-08-28.)*

- **`satellite.capsule` `1 2 (0)`.** M4 parses `capsule_decl`; this is where a
  capsule name becomes a slot, and §7 already orders the work — resolve runs every
  capsule name first, then every spacesuit name (M22), which is what makes forward
  reference work without a second pass.
- **The literal-option fold, WORD_NUMBERS §1.5.** It is a **resolve-time** decision
  that changes which `PathId` a call site interns — `sort("down")` becoming
  `sort_down()` `1 4 2 5` — so it is this milestone's and not M7's.
  `SCRATCH.md/MILESTONE.md` §1 filed it as *"M6 or M7 — nothing says whether resolve
  or closure compilation owns it"*, and the answer is that closure compilation is
  already past the point where the option is a literal. **M18 is waiting on this**:
  whether `satellite.file.open`'s four mode words fold decides whether its bad-mode
  message is M5's suggester or a runtime check.
- **The six spellings of `arguments` become the special variable here.** WORD_NUMBERS
  §2.3's third alias row is one node with six spellings; M2 and M3 hold the spelling
  table that says so, and **resolve is where a parameter named `argz` is recognised
  as §7.7's object rather than as a user's name.** M19's demonstration rests on it,
  and until this pass no milestone claimed it.

**M6.5 — `satellite.variable.number`.** *(Its own milestone as of 2026-08-27; it was
a bullet inside M7.)* The port of the first satellite's `satellite_number` — 10 files,
1509 lines, internally closed, every file already under §3's ceiling (§6.1) — plus the
one thing the port does not bring with it.

**The sign becomes an explicit `satellite.variable.bool` named `positive`, defaulting
to `true`**, and the magnitude never carries one (DESIGN §8.1). A number with no sign
written is positive; something has to flip the flag for it to be otherwise.

**It lands here rather than inside M7 because M7's `Value` contains one**, and because
it is the milestone that builds the sign **both** numeric types share. M9.5's float
inherits it rather than defining a second one, which is the whole reason the two can
be milestones apart instead of one large one.

Done when: exact arbitrary-precision arithmetic runs, negation and `abs` and ordering
of negatives are right, there is no negative zero, and `sizeof` is inside DESIGN
§8.2's 40-byte `Value` budget — which §6.1 names as the one number that could make
this port not fit, and which is cheap to check first.

Four things to settle before copying, in `SCRATCH.md/PORTING.md`, and **the sign is a
fifth**: where v1 currently keeps it has to be checked against DESIGN §8.1 before the
copy rather than after.

**Eleven numbered paths, and the four it does not take.** *(2026-08-28.)*
`satellite.variable.number` `1 6 4 (0)` and ten of its fourteen methods land here —
`shift_left` `1 6 4 1` through `round` `1 6 4 9`, and `shift_right(n)` `1 6 4 11`.
**`power(a, b)` `1 6 4 10`, `modulus(a, b)` `1 6 4 12`, `truncate(a)` `1 6 4 13` and
`sqrt(a)` `1 6 4 14` are M9.5's**, because none of them can be finished before the
rounding rule is chosen: `sqrt` is irrational in general, `modulus` cites DESIGN
§8.6, and truncating a float is its left half. That is
`SCRATCH.md/MILESTONE.md` §0.4's *"either those four move to M9.5 or M9.5 moves ahead
of M9"* answered the cheaper way — **four methods move, and no milestone is
reordered.**

**It reads `satellite.library.system.division_digits` `1 14 2 1` and M14 built the
node it lives on.** §6.1's open question 1 is what makes the dial `Number`'s; M14
owns the file, the storage and `min_free_mb`, and this is the first milestone to give
one of the four dials a meaning.

**It also owns the uniform draw, and its own line did not say so.** *(2026-08-28,
and this is the fourth instance of that failure after M3/M4's eleven words, M10's
thirty-four methods and M9's `satellite.bool`.)* `satellite_number/random.cpp` — 121
lines: the limb-aligned draw in base 10⁹, the rejection sampler that exists because a
bare `%` would skew 2:1, and `MAX_RANDOM_DIGITS = 100000` — is one of the ten files
this port brings across. `SCRATCH.md/PORTING.md` has the row; this milestone had it
and never repeated it, and **M16 needs to be able to say it is inheriting the bignum
half of the dice rather than writing one.**

**This closes half of `SCRATCH.md/MILESTONE.md` §3's porting row**; M7 closes the
other half with `satellite_string`.

**M7 — the value model and closure compilation.** `Value` (40 bytes, the
static_assert comes too) and `Str`. `Number` arrives at M6.5 and this milestone is its
first consumer. The arena AST compiles to a closure tree.
Module calls dispatch through `handlers[path_id]`, and the **inline caches of §2.4
land here too** — third of the three adoptions §2.6 orders, and the milestone that
owns them. Recursion depth is bounded here.

**Five things it owns and had never written down.** *(2026-08-28. Four of them are
consumed by later milestones that had each assumed somebody else built them.)*

- **`Str` is the port of `satellite_string`, and it needs M14's fact readers.** §6.1
  records that the code table is not only characters: **codes 95–100 are live values
  resolved at decode time**, and 97, 98 and 99 are `threads`, `mem_total_mb` and
  `mem_used_mb`. So this milestone calls `hardware_threads()`, `mem_total_mb()` and
  `mem_used_mb()`, which M14 ports. **This closes the other half of
  `SCRATCH.md/MILESTONE.md` §3's porting row**, whose whole complaint was that M7
  needs `Number` and does not say the port happens here.
- **The recursion ceiling is derived, not fixed** — DESIGN §7.5 takes it from
  `RLIMIT_STACK`, which is `system_facts/stack_facts.cpp`'s `stack_limit_bytes()`,
  also M14's. *"Recursion depth is bounded here"* is what that sentence meant and
  did not say.
- **`satellite.library.system.max_depth` `1 14 2 2` is this milestone's dial**, and
  **M10's search walk is its second consumer** with a depth error of its own. Both
  entries now say so, because "M7 builds it and M10 reuses it" is fine and "neither
  milestone ever says it" is how this gets built twice.
- **The variant is append-only, and three appends are already known.** v1 appended
  `ArgsRef` and `ResultRef` and `sizeof(Value)` stayed at 40 with the static_assert
  holding. The three are: **`Value`'s reference-type handle arm, which M18 appends**
  — DESIGN §8's table makes a file a reference type, two variables holding one file
  share one descriptor, and v1 carries it as `FilePtr`, the eighth of its twelve
  arms; **the arguments object's arm, which M19 appends**; and whatever Orbit's
  result becomes at M24. **Each re-runs this milestone's static_assert.** DESIGN
  §8.2 budgets the 40 bytes and §6.1 calls `sizeof` the one number that could make
  the port not fit, so the rule belongs here rather than in the milestone that trips
  over it.
- **The dispatch table carries DESIGN §6.4 qualification 2's receiver-binding tag.**
  It is the only reason `satellite.file.new(path)` `1 8 1` and
  `satellite.variable.file.new` `1 6 2 1` can coexist — WORD_NUMBERS §4 calls that
  pair *"the kind of thing that gets decided by accident at M10"* — and M18 needs it
  built rather than legislated from three milestones later.

**M8.A — the console, and the first program that runs.** *(Split from M8 on
2026-08-28.)* Console with its printer thread, `satellite.main`, `satellite.return`.
**This is the milestone at which satellite executes anything at all**, and
everything from M9 on depends on it for the same reason every milestone after M1
depends on there being a binary: without it there is nothing to print through and
nothing can be demonstrated.

**Why the split.** M8 was one milestone until the empty `satellite.container.list`
that `satellite.main`'s parameter binds to was ruled M10's, which pushed M8 after
M10 and left M9 and M10 with no console — against §8's opening rule. **The
parameter is the only part that ever needed a list.** So the console, `main` and
`return` stay here, and hello world itself is **M8.B**, after M10.

**Its done-when cannot be DESIGN §3**, which is the thing worth saying out loud:
§3's hello world declares the parameter, and the parameter is M8.B's. What runs here
is the **bare `satellite.main()` form**, which is still legal and always was — §6's
grammar reads `"(" [ param_list ] ")"` and DESIGN §3 keeps both shapes. **`example/`
holds no bare-main program**, so this milestone has no acceptance file yet; writing
one is the author's, and until then its done-when is prose, which §8's opening calls
the weaker kind.

**Its seven paths, and the two unnumbered mechanisms M17 consumes.**
*(2026-08-28.)* `satellite.main` `1 3 (0)`; `satellite.console` `1 5 (0)` and
`display` `1 5 1`; `satellite.return` `1 15` with all three shapes — `return()`
`1 15 0`, `return(satellite)` `1 15 1`, `return(value)` `1 15 2`, where DESIGN §4 is
why the middle one means success. The other eight children of `console` are
**M17's**. What is not numbered and is still this milestone's: **`display`'s
un-newlined form**, which lives under `1 5 1`, and **the `drain()` barrier**, which
§6 keeps in as many words — *"the Console with its own printer thread, and the
`drain()` barrier before reading input"* — and which DESIGN §10.1 justifies by *"a
prompt written with no trailing newline."* **Both exist for input**, three milestones
before anything reads any, and M17 consumes them rather than rebuilding them.

**The printer thread is the pool's first tenant and takes a thread from M14** rather
than spawning one of its own. §4.5.1's whole argument for a pool is *"one pool with
three tenants amortises a cost that none of them could justify alone"*, and this is
the tenant that arrives first in build order.

**M9 — scalars and control flow.** `satellite.statement.if` `1 13 1`, `.for` `1 13 2`,
`.while` `1 13 3` and `.else` `1 13 4` — **their parse rules land at M4** (above);
what lands here is running them. `satellite.variable.bool`, `.number`, `.string` and
their methods.

**`satellite.bool` `1 17` is a different node and IS this milestone, as of
2026-08-28.** The type is `satellite.variable.bool` `1 6 6`; the module constants
`satellite.bool.false` `1 17 1` and `.true` `1 17 2` hang off a top-level namespace
`1 17 (0)` that DESIGN §6.1 cites as the module-constant case. Until this pass the
paragraph here read *"is not this milestone"* and no other milestone claimed them, so
three paths were refused by the only milestone in a position to build them — which is
the M3/M4 failure with the sign flipped, and worse, because it was written down
deliberately. **The distinction it was drawing is still true and still worth
keeping**: two nodes, two numbers, and reading `satellite.bool.true` as a way of
spelling the type would be a real error. What was wrong was the conclusion.
**Building them costs a `handlers[path_id]` entry each**, because a module constant
is a path that evaluates without a call, and this milestone already builds the type
they answer with.

**It also owns the SIGINT half of Ctrl-C, and that was nobody's.** *(2026-08-28.)*
v1's `install_interrupt_handler()` — 249 lines in `system_facts/interrupt.{hpp,cpp}`,
installed **without `SA_RESTART`**, which §6 marks *hard-won; do not rediscover* —
works by having the first SIGINT set a flag and *"let the walk stop itself at the
next statement, which is what makes an interrupted program report the line it was
on."* **The first walk long enough to be stopped is `while` `1 13 3`, here.** M11.B
owns the other half — the prompt's Ctrl-C, which arrives as the byte `0x03` because
raw mode turns ISIG off, so it never reaches a handler — and M17's `1 5 2`–`1 5 4`
need this half to tell an interrupted read from a closed stdin.

**M15 — the variant, and what "nothing" is.** *(New 2026-08-28. After M9.)* Two
numbered paths — `satellite.variable.variant` `1 6 14` and
`satellite.variable.expression` `1 6 9` — and it builds one of them. It is a small
milestone that exists first for a reason that has nothing to do with its size:
**two later milestones ask the same question in the same words, and it has to be
answered once, in front of both, or it gets answered twice.**

> `satellite.console.typed()` `1 5 5` — *a line, or nothing.* (M17.)
> `satellite.variable.file.read_line` `1 6 2 3` — *one line, or nothing at end.*
> (M18.)

**Nil is already in the value model and is not already in the language.** DESIGN
§8.2 requires that `bool` and nil never allocate, so M7's `Value` carries the state;
DESIGN §6.4 qualification 3 names it — *"two non-dispatchable states needing
different messages: an undeclared variable, and a declared variable holding
nothing"* — and that is a rule about **error messages**, not a way for a program to
ask. Between M7 and here a satellite program can be handed nothing and has no
sentence it can write about it. That is the gap this milestone closes, and DESIGN
§1.1 is why it is not acceptable to leave it to whichever of M17 and M18 lands
first.

**`satellite.variable.variant` is the type the question belongs on.** A variable
declared as one type either holds a value of it or is the declared-and-empty state
§6.4 names; a variable declared `variant` is the one that can truthfully answer
*what are you holding*, which is the answer both call sites above need. DESIGN §8's
table carries the row with a dash for its representation and *"deferred; PLAN §8,
Later"* as its whole specification, so **the representation is this milestone's
first piece of work and DESIGN §8 gains a real row here.**

**Blocker, and only the author can take it: is "nothing" a state every type has, or
a value only a `variant` can hold?** The two readings produce different languages
and both are consistent with what is written down today. Under the first,
`satellite.variable.string line = my_file.read_line()` is legal and `line` may be
empty-or-nothing; under the second, that declaration is a type error at end of file
and `read_line` returns a `variant`. **§6.4's "declared variable holding nothing"
reads as the first** and DESIGN §8's deferral of `variant` reads as the second, and
nothing reconciles them. M17 and M18 both inherit whichever answer is given, so the
answer is worth more than either milestone.

**`satellite.variable.expression` `1 6 9` stays numbered and unbuilt, and saying so
is the work.** Its entire provenance is one row in `SCRATCH.md/WORD_SURFACE.md`
sourced to `v1docs` — no representation, no method, no v1 code, no sentence in
DESIGN. WORD_NUMBERS §1.2 means it cannot be withdrawn without leaving a hole, and
M2's density check refuses holes, so it keeps its number whatever happens to it.
This is `satellite.variable.duration` `1 6 8`'s shape exactly (M16), and both are
here rather than in a future audit's table.

**Done when** one program declares a `satellite.variable.variant`, puts a number in
it, then a string, then nothing, and prints a different and correct answer at each
step through `satellite.console.display` — and when a `variant` that holds nothing
is refused by name if a method is called on it, which is §6.4 q3's second message
becoming something a person can read. Nothing here needs a float, a container, a
thread or a file; **it needs M7's `Value`, M8.A's console and M9's scalars, and
that is the whole of its dependency list.**

**M16 — the clock and the dice, the two sources of nondeterminism.** *(New
2026-08-28, corrected from the 2026-08-27 draft. After M15.)* **Twenty numbers on
twenty-three rows**, named individually here because a milestone that says only
`satellite.random` leaves twelve children owned by nothing, which is the failure
this whole pass exists to end:

- `satellite.random` `1 7 (0)` and `1 7 1` through `1 7 12`.
- `satellite.time` `1 9 (0)`, `.now` `1 9 1`, `.new` `1 9 2`, `.sleep(n)` `1 9 3`.
- `satellite.variable.time` `1 6 3 (0)`, `satellite.variable.date` `1 6 7` and
  `satellite.variable.duration` `1 6 8` — **three siblings under
  `satellite.variable`**, not a subtree. A child of `1 6 3` would be `1 6 3 n`, and
  §1's per-parent rule is the reason that sentence has to be written out rather
  than abbreviated with leading dots.

**`satellite.random` is thirteen numbers on sixteen rows.** `.range` is a second
spelling of the two-argument shape, not a fourth segment and not a path
(WORD_NUMBERS §2.3): `fast.range(min, max)` **is** `1 7 5`, `normal.range` **is**
`1 7 8`, `ultra.range` **is** `1 7 11`. **Those three are the only duplicate
numbers in the language**, and they are the whole of the difference between the 222
rows §2.2 holds and the 219 distinct numbers it carries. That reconciliation was
written down only in `SCRATCH.md/MILESTONE.md` §5, which is scratch; it lives here
now.

**M2 already built the alias, so this milestone inherits it rather than finding
it.** The draft this comes from reported the three rows as a finding *about* M2 —
that a tree whose position is its number cannot express `1 7 5` for a node named
`range` under `fast`, and that the density assert would fire. Both halves were true
of M2 as specified and neither survived M2 as built: `words.def` holds **254 nodes
and 9 aliases**, an alias is a second spelling rather than a second node, and
"no duplicates" is by construction because a number is a position. `hexadecimal`
and DESIGN §7.7's six spellings of `arguments` are the same mechanism at two other
scales. What is left for this milestone is to use it and to test it, which
done-when clause 2 does.

**DESIGN §11 is stale about how many shapes there are and says so nowhere, and
correcting it is part of this milestone.** §11 says *"two shapes on each"*, which
was true of the first satellite — six random paths in its registry, and a header
saying the public surface is two shapes per tier. §2.2 gives each tier four slots.
§11 is one commit older than the numbering (`095eb49`, against `99fa6d9` and
`cc3f813`) and neither numbering commit touched it. **Building four shapes out of a
section that says two is the M3/M4 failure run backwards.** Everything else in §11
is still true and stays — the tier table, uniform over [0, 10⁴⁰), inclusive at both
ends, the one-in-ten paragraph and all of §11.1. What replaces the stale sentence
is a pointer to §2.2 and a line saying which numbers are specified, **plus a date**,
which §11 carries neither of and is how it went stale without ever looking wrong:

- `1 7 4`, `1 7 7`, `1 7 10` — `<tier>(digits)`. Specified, built in v1, tested.
- `1 7 5`, `1 7 8`, `1 7 11` — `<tier>(min, max)`, spelled `.range` in v1. The same.
- `1 7 6`, `1 7 9`, `1 7 12` — `<tier>(min, max, step)`. Specified in no document.
- `1 7 1`, `1 7 2`, `1 7 3` — blocked below, and the answer decides whether the
  call surface is nine shapes or twelve.

**Nine of the sixteen rows are code v1 built and tested; seven are not.** v1's
registry carries exactly six random paths — its own `format.def` says *"Five words
buy six paths"* — and those six are `fast/normal/ultra(digits)` and the three
`.range` spellings, which is nine §2.2 rows over six numbers. The other seven rows
are `1 7 (0)` and the six numbers this milestone leaves reserved. **"Sixteen rows
of working code sitting in Later" was the draft's sentence and it overstates in the
direction that makes the finding look bigger**, which is the failure the last three
commits before this pass were about.

**Five of these twenty numbers were not in "Later" at all**, which is worth saying
because "Later" is what this milestone empties: all four under `satellite.time`
`1 9 (0)` — a different node from `satellite.variable.time` `1 6 3`, and
`SCRATCH.md/MILESTONE.md` §0.0 exists to correct exactly that confusion — and
`satellite.variable.duration` `1 6 8`, which is in no list anywhere except DESIGN
§12's deferrals.

**The bignum half of the dice is M6.5's and this milestone does not claim it.**
`satellite_number/random.cpp` — 121 lines: the limb-aligned uniform draw in base
10⁹, the rejection sampler that exists because a bare `%` would skew 2:1, and
`MAX_RANDOM_DIGITS = 100000` — is one of the ten files M6.5 ports, and §6.1's open
question 3 says as much while M6.5's own entry did not. **M6.5 now says it.** What
this milestone ports is the other half — `random_numbers/random.{hpp,cpp}`, 260
lines: the tiers, the seed, the fold and the watchdog — plus a dispatch that is a
**rewrite and not a copy**, because v1's `modules_random.cpp` is a string compare on
the tier and on `"range"` and §7 throws exactly that away. What survives is the
argument checking and v1's nine failure texts, reached through `handlers[path_id]`
and reported through M5.

**The seed path is broken as built, and porting it unchanged ports a crash.** v1's
own `plans/pcg_k16384_spec.md` records that under `strace` the built `./satl`
issues exactly one `getrandom` in the whole process — glibc's own 8-byte startup
draw — and that its 65,568 seed bytes come from RDRAND through libstdc++'s default
token, never touching the kernel, while three comments in the code say otherwise.
Worse, that path **throws**: libstdc++ retries RDRAND a hundred times and then
raises, and there is no `catch` anywhere under v1's `src/`, so the failure mode of
a language builtin is `SIGABRT`. The recommended replacement is `getrandom(2)` with
flags 0 in a loop — 12.4× faster, an `errno` instead of an exception, and `ldd`
unchanged. **This is a done-when, not a footnote.**

**The vendored PCG is the project's first third-party dependency and LAYOUT.md has
no row for one.** Three headers, 3,138 lines, Apache-2.0 inside an MIT tree,
header-only so `ldd` does not change. It is reached through `-isystem` for a
load-bearing reason rather than a stylistic one: `pcg_extras.hpp` warns under this
project's `-Wall -Wextra`, and §4's silent from-scratch rebuild depends on that
warning not being ours. **This milestone writes LAYOUT.md's first vendored row**,
and the Apache notice travels with it.

**The tier windows are a design choice and stand; the watchdog is a measurement and
does not.** §9 says measure on this machine and do not quote. v1's figures are a
Xeon E5-2670 v3's: about 153,600 draws per millisecond, `ultra` reaching roughly
249M draws at 2,000 ms and 310M at 3,000 ms against a cap drawn from [500M, 600M] —
a margin of about 1.6–1.9×. **On a machine 1.7× faster `ultra` reaches the cap and
the watchdog stops being a failsafe and becomes part of the mechanism**, silently
shortening the spin. Re-measure before claiming the tiers behave as documented; if
the margin has closed, the per-tier caps v1 recorded and declined are this
milestone's work.

**What this milestone produces for QUAD is a correction, not a mechanism.** QUAD.md
§2 files `random` under *"already decided in satellite, and fine."* It is not.
`quad_core.hpp`'s entire randomness is one primitive — a uniform `double` in [0, 1)
from a seeded `mt19937` — and `Rack::draw` is a roulette wheel over `std::pow`
weights. Three independent reasons `satellite.random` cannot write it: **no float
draw exists in any of the thirteen numbers**, and v1 refuses fractional bounds
deliberately; **there is no seed**, so QUAD's determinism invariant is not
expressible; and **the cheapest tier throws draws away for 50–100 ms before it
answers**, against a 90 ms tick that draws from the rack once and calls the rng
nineteen more times. The shape of an answer is a fourth thing under
`satellite.random` that takes a seed and does not spin, and **`1 7 13` is free** —
this milestone does not assign it, because minting a number is the numbering's and
the author's. M20 is where that debt comes due.

**Blockers — the milestone is not finishable around these:**

- **One clock, or two.** DESIGN §13 requires `satellite.variable.time`, `.date` and
  `satellite.time.now` to agree on **one clock and one epoch**, and says a monotonic
  clock and a wall clock cannot both be it. **v1 answered both halves and the
  answers do not compete**: `system_clock` and the Unix epoch for the *value*,
  because `steady_clock`'s epoch is unspecified and the value has to mean something
  outside this process; `steady_clock` for the *timer* — the spin deadline and the
  sleep — because `high_resolution_clock` is the same type as `system_clock` here,
  `is_steady` is false, both tick at 1 ns with 21 ns granularity so there is no
  precision to trade away, and NTP can step `system_clock` backwards under a
  running deadline. Only one of the two is ever a satellite *value*, so *"one clock,
  one epoch"* is a rule about the type and not about the implementation. **That
  reading measured the author's stated leaning and rejected it, so it is put to the
  author rather than taken here.**
- **What `1 7 1`, `1 7 2` and `1 7 3` name** — the tier node, or a zero-argument
  call. DESIGN §4's example writes all three tiers bare, and M2's acceptance test
  writes `satellite.random.normal` bare at `1 7 2`; the author's note writes the
  digit shape only, `satellite.random.fast(99)`, which is `1 7 4` and settles
  nothing. The parentheses appear first in §2.2's rewrite, whose preamble promised
  that every number was preserved — a promise about numbers, not about what they
  name. Under the call reading, v1 tests that source as an arity error and there is
  no number left for the node the spelling `ultra` lands on; under the node reading,
  the tier is a path that is not a value, which v1 also tests. **The answer is a
  precondition of this milestone's own number list**, which is why the draft could
  count `1 7 2` as covered on one page and reserved on another.
- **The step forms `1 7 6`, `1 7 9`, `1 7 12`.** Assigned by rule, specified
  nowhere, built nowhere, and they collide with the one promise §11 makes: `.range`
  is inclusive at both ends, and `(min, max, step)` reaches `max` only when
  `max - min` is a multiple of `step`. Whether `(1, 10, 3)` can answer 10 is exactly
  what DESIGN §13 warns gets settled by accident.
- **What `satellite.time.new` `1 9 2` takes.** Its only specification anywhere is
  nine words in a deleted note — *"set the arguments for a point in time"* — which
  reads as a component constructor and contradicts v1's rule that an instant is read
  off the clock and never written down as a literal. It cannot be designed apart
  from `satellite.variable.date` `1 6 7`. **DESIGN §13 cites it as an established
  precedent** — *"exactly as `satellite.file.new` and `satellite.time.new` already
  do"* — when it has never existed; the argument still holds on `file.new` alone,
  and §13 should say so.
- **The unit of `satellite.time.sleep(n)`.** QUAD's call is
  `sleep_for(milliseconds(90))` at `quad_main.cpp:260`. The one unit v1 spells is
  `100ms`, a lexed literal converted at parse time, and §7 throws away the special
  case that consumed it but not the literal. A bare number makes the unit invisible
  at the call site, which is the readability failure DESIGN §1.1 exists to prevent.
- **`satellite.variable.date` `1 6 7` has a number and nothing else** — no row in
  DESIGN §8's types table, no representation, no constructor, no method, no v1 code.
  Either the specification work is carried here or the number stays reserved and
  this milestone says which.
- **The two-or-more time methods v1 ships are unnumbered.** `.minus(t)`,
  `.nanoseconds()` and `.to_string()` are all handled on a `Time` in v1, and help
  advertises `.size()` on the next line. `satellite.variable.time` `1 6 3 (0)` has
  **zero** children in §2.2 while its three hand-written siblings have 16, 7 and 14.
  **WORD_NUMBERS has to number them and this milestone must not**; until it does, a
  program can obtain an instant and do nothing whatever with one — which also means
  this milestone cannot time its own tier floors in satellite.

**`satellite.variable.duration` `1 6 8` stays numbered and unbuilt, and saying so is
the work.** The sweep sourced it from v1's documents; the v1 source that uses the
phrase says there is no such type and gives four reasons, and DESIGN §12 still
defers durations today. It cannot simply be struck either: §1.2 leaves a hole where
a child is removed and M2's density check refuses holes. So it keeps its number
whatever is decided about it, and it is listed here rather than left to an audit —
the same treatment `satellite.variable.expression` `1 6 9` gets at M15.

**Done when** this runs inside `satellite.main` and every clause below holds:

```satellite
satellite.variable.number big = satellite.random.ultra(40)
satellite.console.display(big)

satellite.variable.number die = satellite.random.fast.range(1, 6)
satellite.console.display(die)

satellite.variable.number i = 0
satellite.statement.while (i < 10)
{
    satellite.console.display(satellite.time.now)
    satellite.time.sleep(90)
    i = i + 1
}
```

1. `ultra(40)` is uniform over [0, 10⁴⁰), and a hundred runs give **about ten
   answers of 39 digits or fewer**. §11 says the leading-zero fact is stated in the
   header and in the test, so the test asserts the short answers *happen*; one that
   merely tolerates them is not the test §11 asked for.
2. `.range(1, 100)` answers both 1 and 100, `.range(7, 7)` is 7, and
   `.range(-100, -90)` lands inside itself — v1's assertions, ported, and the proof
   that M2's alias rewrite carries a real call.
3. All three tier windows are honoured, timed against `steady_clock` and asserted
   **on the floor only** — a ceiling is a machine's to miss under load, and a test
   that fails when the build box is busy is a test nobody trusts.
4. Draws per millisecond and `ultra`'s real draw count at 3,000 ms are measured **on
   this machine, with the date, beside the watchdog cap they justify**, and the
   margin is stated (§9).
5. **Eight refusals through six ported texts**, plus two that are not random's at
   all: `fast("x")` and `fast(1.5)` share one text, `fast(0 - 1)` and
   `fast(100001)` share another, and the two unlisted texts — `.range` wanting two
   numbers, and the could-not-draw and spans-more-than failures — come across with
   them. **An unknown tier and an unknown tail segment are M5's did-you-mean over
   the failing trie level, not this milestone's strings**; v1 answers both from one
   generic *"no such module function"* and §7 throws that dispatch away.
6. `satellite.time.now` twice in succession returns two different values — the
   one-line proof that int64 nanoseconds is the representation and a `double` is
   not, since 61 bits of epoch against a 53-bit mantissa is 198 ns of resolution.
7. The loop paces at 90 ms, which is QUAD's main loop shape and the whole of what
   QUAD.md §2 asked `sleep` for, naming `quad_main.cpp:260`. *(90 ms is QUAD's
   number, not this machine's; §9 means it is re-measured here or sourced there.)*
8. **No tier is described as secure, anywhere** — not in a header, not in
   `satellite.help`, not in an error message. DESIGN §11.1, and the first
   satellite's reason for wording the refusal that hard travels with it: the word
   "secure" in a language's documentation is load-bearing, because someone will key
   something on it. The answer not taken, `getrandom(2)` at 256 bits for about a
   microsecond and no new dependency, travels with it too.
9. The seed comes from the kernel and cannot abort, or the refusal to change it is
   written down beside the measurement that condemns it.

**Nine of the twenty numbers stay reserved and unbuilt, and are listed rather than
left to a later audit**: `1 7 1`, `1 7 2`, `1 7 3`, `1 7 6`, `1 7 9`, `1 7 12`,
`1 9 2`, `1 6 7` and `1 6 8`. **A milestone that quietly leaves numbers behind it
is how this document came to have a 121-path ledger.**

**It does not depend on M9.5 and must not be placed behind it.** The draft put
itself after the float *"because this one has to report that the dice cannot use
it"*, which is a type confusion: DESIGN §8.1 makes `satellite.variable.number` an
exact arbitrary-precision decimal, so `1.5` is a `Number` and every fractional
refusal v1 tests is a `Number` test. Nothing here needs a float, and M9.5 is the one
milestone in this list that cannot land until an undecided rule is chosen. Its real
dependencies are M6.5 for the bignum, M8.A for `display` and `satellite.main`, and
M9 for `while`.

**M17 — the console's other half: the reader thread and the terminal's facts.**
*(New 2026-08-28, corrected from the 2026-08-27 draft. After M16.)* **Eight paths —
`1 5 2` through `1 5 9`, every child of `console` except `display` `1 5 1`**, which
is M8.A's. §2.2 has no tenth child, so with M8.A this namespace is finished:

- `satellite.console.input()` `1 5 2`, `input(prompt)` `1 5 3` and
  `input(prompt, target)` `1 5 4` — ask, and wait, in three shapes. WORD_NUMBERS
  §1.3 is why that is three numbers rather than one with an arity check: a
  user-owned argument cannot extend a path, so the count takes its own slot and
  **the arity is the identity**, settled at M4's parse before an argument is
  evaluated. `1 5 4` writes a place and returns nothing — the only out parameter in
  the language, and deliberately not the start of a general facility.
- `satellite.console.typed()` `1 5 5` — a line, or nothing, immediately. **The one
  genuinely new mechanism here**; v1 has no non-blocking input of any kind, and
  M15 is what makes *nothing* a thing a program can ask about.
- `satellite.console.width` `1 5 6` and `.height` `1 5 7` — property-shaped in the
  table because they are facts, asked fresh rather than sampled. v1 has no `height`
  at all: `ws_row` appears zero times in it.
- `satellite.console.clear()` `1 5 8` and `.home()` `1 5 9` — call-shaped because
  they are actions, and **both go through the printer's queue**. v1 writes them as
  one escape straight to the fd, which is a frame shredded between two queued lines.

**The reader thread is DESIGN §10.1's printer with the polarity named.** The
printer's producer is the program and its consumer is a dedicated thread; the reader
is that reversed — the dedicated thread blocks on stdin and pushes whole lines in,
and the program asks and is answered at once. The invariant that survives both
directions, and the reason no satellite program ever learns what a terminal mode
is: **the program's own thread never blocks on the terminal.** §1.1 applied to
input. QUAD.md §3.4 is the requirements document, and it is what retires QUAD's
`VMIN=0` poll loop.

**M8.A owns two things this milestone consumes and must not rebuild**, and M8.A's
entry now says so: `display`'s **un-newlined form**, which lives under `1 5 1`, and
the **`drain()` barrier**, which has no number at all and which §6 keeps in as many
words — *"the Console with its own printer thread, and the `drain()` barrier before
reading input"* — while DESIGN §10.1 justifies the flush by *"a prompt written with
no trailing newline."* Both exist **for input** and neither is `1 5 3`. The draft
this milestone comes from wrote that M8 *"owns the output half of `1 5 3`"*, which
would split one numbered path across two milestones — the failure it was written to
prevent, committed in the sentence preventing it. **This milestone owns all of
`1 5 3`.**

**This is not a line editor, and M11.B's prompt is not `satellite.console.input`.**
M11.B's prompt is a second, unrelated reader: v1's `console_input/` is 1,449 lines
of raw mode, key decoding, history and a wrap-aware renderer, and its own header
states that no satellite program can reach anything in it, while
`satellite.console.input` runs through `std::getline` in the evaluator. Two readers,
one language. **Only 14 of those 1,449 lines come here** — `terminal_columns()`'s
ioctl with its 80-column fallback, and the clear — and **M11.B consumes both from
here** rather than reimplementing them, which its entry now says.

**Ctrl-C is two halves and this milestone takes one of them.** DESIGN §10.2 gives
the key two meanings; raw mode turns ISIG off, so the prompt's Ctrl-C arrives as the
byte `0x03` and never reaches a handler — **that half is M11.B's, and M11.B's line
now says `0x03` rather than bare "Ctrl-C"**, because a bare "Ctrl-C" reads as owning
both. The SIGINT half is not this milestone's either, and the draft's claim that it
was is refuted by the draft's own dependency argument: v1's handler *"sets the flag
and lets the walk stop itself at the next statement"*, and the first walk long
enough to be stopped is M9's `while`. **`install_interrupt_handler()` — 249 lines,
installed without `SA_RESTART`, which §6 marks *hard-won; do not rediscover* — is
M9's, and M9's entry now names it.** What is genuinely this milestone's is the
read side: **`eof()` is the entire discrimination between a closed stdin and an
interrupted read**, so `1 5 2`–`1 5 4` cannot report truthfully without it.

**`width` and `height` are not `arguments.machine.*`**, which the ledger wondered
about. DESIGN §7.7's test is three surfaces over one set of facts:
`arguments.machine.threads` `1 14 1 1 1 3`, `.machine.cores` `1 14 1 1 1 1` and
`arguments.memory.total` `1 14 1 1 2 1` — **`memory` is a sibling of `machine`, not
a child of it** — live there because they are also `THREAD_COUNT`, `CORE_COUNT` and
`MEMORY_MAX` and `satellite_string`'s live codes, and *"must not be allowed to
disagree."* Terminal size has no second surface, no config counterpart, and
**changes during a run**, which a startup-sampled object cannot express. And
`arguments.clear()` would already mean something: `arguments` is a
`satellite.container.list` (DESIGN §7.7), so §1.5 resolves the bare `clear` through
the receiver's type to `satellite.container.list.clear` `1 4 2 11`. **That is a
spelling collision and never a numbering one** — §1's lists are per-parent, so the
numbers could not clash.

**Roughly 126 lines port close to unchanged** — `evaluator/modules.cpp` 62–187
(`read_input_line`, `console_input`, `console_input_into`), where the comments are
the specification and come across with the code. **Lines 188 onward are `display`'s
and stay with M8.A**: `named_arg_misuse()` is the message for
`display(text, end="")` and `display_with_end()` begins at 205, so the draft's
62–204 pulled seventeen of M8.A's lines into this port — the ownership blur two
paragraphs above exist to prevent. `set_display_pace` is the `100ms` special case §7
throws away. `typed()` and the reader thread port nothing.
`console_output/console.cpp` is not ported here and **is the file to read before
writing the reader**, because every hazard it documents reappears reversed —
swapping the vector out under the lock rather than erasing the front, and **three**
condition variables (`arrived_`, `emptied_`, and `pace_woken_` on a mutex of its
own) so that a waiting drain and a sleeping printer are never woken for each other's
reason.

**Open, and each of these shapes the milestone rather than decorates it:**

- **One reader of stdin, or two?** A reader thread parked in `read()` and an
  `input()` calling `getline` on the walking thread are two consumers racing one fd.
  Routing `input()` through the same queue is almost certainly right, and it changes
  code §6 marks *do not rediscover*, so it is a decision and not a detail.
- **Ctrl-C across a thread boundary.** §10.2's mechanism was designed for a read on
  the walking thread. Move the read and the signal lands on an arbitrary thread,
  `EINTR` surfaces where there is no error to report, and the
  interrupted-versus-EOF answer has to travel back through the queue.
- **How the thread stops.** The pool starts at startup from M14, so *when it starts*
  is no longer this milestone's question — but a thread blocked in `read()` cannot
  be joined at exit the way `~Console()` joins the printer. A self-pipe wakeup or a
  detach-and-leak differ in whether `satl` exits cleanly. **This is the pool's
  fourth tenant and §4.5.1 names three.**
- **How the registry declares that a parameter is a place.** §7 condemns v1's route
  to `1 5 4` — its first bullet, *"the joined path built per call"*, covers
  `expr_call.cpp:113–114` flattening the path and comparing it against a string
  literal before evaluating arguments, the same hack as the `100ms` case forty lines
  above it. The replacement is a declaration on the word, the way `display` will
  *declare* that it accepts a pace argument. **§6.4 qualification 2's
  receiver-binding tag is the nearest precedent and nothing extends it to a
  non-receiver parameter.**
- **Does `clear()` imply `home()`?** v1 emits both as one sequence. If `1 5 8`
  homes, `1 5 9` is only ever useful alone; if it does not, QUAD's frame draw is two
  calls where `view.hpp` had one. Small, and it is a promise.
- **What the reader does during M11.B's prompt**, which puts the terminal in raw
  mode and thinks it owns stdin. Whichever of the two lands second decides it, and
  saying so now is cheaper than finding it at M11.B.

**Done when** one program run under `pty.fork()` — assert on the screen, not on the
bytes — proves all eight paths, each clause a separate assertion:

1. **A counter keeps rising while a line is half-typed.** Send `/q` a byte at a
   time; the number advances across every pause. QUAD.md §3.4 requirement (1), with
   `VMIN=0` retired, and the one claim this milestone exists to make.
2. **And it does not spin.** CPU near idle between keystrokes, because the blocking
   happens on a thread that waits — *"strictly better than the poll loop the obvious
   alternative produces"* (DESIGN §10.1). **This clause is why M16 comes first**:
   it needs `satellite.time.sleep(n)` `1 9 3`, a busy-spin loop makes it
   untestable, and in the draft that path was reached by no milestone at all.
3. **`typed()` tells an empty line from no line.** Return on an empty line is a
   line; nobody typing is nothing. Two answers, not one empty string — **M15's
   answer, consumed here rather than invented here.**
4. **The prompt appears before the cursor waits.** `1 5 3` prints with no trailing
   newline and with the queue drained; a prompt arriving after the program has
   already blocked is the failure `drain()` exists to prevent, and it is observable
   only here.
5. **`1 5 4` writes a place and yields nothing**, assigning its result is an error,
   and a non-variable second argument fails *before* the prompt prints — so nobody
   types an answer that is then thrown away.
6. **Ctrl-C is not end of input.** At the prompt it reports `interrupted`;
   `satl demo.satl < /dev/null` reports reaching the end of input and exits instead
   of looping. Getting those two backwards is the regression §6 says not to
   rediscover.
7. **`width` and `height` are live.** Resize the pty between two frames and both
   numbers change without a restart.
8. **`clear()` and `home()` never race the printer.** Alternating `home()` with
   `display()` in a tight loop never shows an escape landing between two queued
   lines — proof both went through the queue rather than the fd.

**Clauses 1, 2 and 8 cannot be demonstrated by any earlier milestone and cannot be
demonstrated by the eight paths one at a time**, which is §8's own argument that
this is one milestone rather than a bullet added to two.

**M9.5 — `satellite.variable.float`.** *(Its own milestone as of 2026-08-27. It spent
the morning in "Later, in no fixed order", was moved into M9, and is separated out
here because it is a type with a specification of its own and one undecided rule.)*

A `satellite.variable.bool` and **two `satellite_number`s** — `positive`, then the
integer part and the fractional part, each an exact base-10⁹ magnitude. **DESIGN §8.6
is the specification**: the three invariants, `normalize`, the four operations,
modulus, power, and the classification that says which operations round and which
cannot.

**It costs no new arithmetic.** M6.5 brought `satellite_number` across and built the
sign; a float is composition over two of them plus rounding.

Two documents used to disagree about whether this was urgent — PLAN filed it under
"Later" while DESIGN §13 and QUAD.md §3.1 called it the critical path. It is the
critical path: QUAD is 164 `double`s and cannot be written without it.

**Five numbered paths, and four of them are M6.5's type rather than this one's.**
*(2026-08-28.)* `satellite.variable.float` `1 6 10`, plus
`satellite.variable.number.power(a, b)` `1 6 4 10`, `modulus(a, b)` `1 6 4 12`,
`truncate(a)` `1 6 4 13` and `sqrt(a)` `1 6 4 14` — the four of the number's fourteen
methods that **cannot be finished before the rounding rule is chosen**, which is this
milestone's blocker and not M6.5's. `SCRATCH.md/MILESTONE.md` §0.4 named the choice —
*"either those four move to M9.5 or M9.5 moves ahead of M9"* — and moving four
methods is the smaller move. **`satellite.library.system.float_digits` `1 14 2 4` is
this milestone's dial**, on the node M14 builds, and DESIGN §13 has already redefined
it from *the* dial into **the default length of a float's right half** for a value
that does not state one.

**Done when the four operations run and — the blocker — the rounding rule is chosen.**
Truncate, half-up, or half-even. No representation escapes it: `pow` at a fractional
exponent is irrational, so the fractional half must be rounded to exist. QUAD's
determinism invariant means a program's behaviour depends on the answer.

**M10 — containers and the search power.** `satellite.container.list`,
`satellite.container.map`, **and their methods** — the map's nine `1 4 1 1`–`1 4 1 9`
and the list's twenty-five `1 4 2 1`–`1 4 2 25` — plus the search power ported close
to unchanged.

**That clause is a fix, not an addition.** *(2026-08-27.)* M9 writes "`.bool`,
`.number`, `.string` **and their methods**" and this milestone did not, so
**thirty-four** numbered paths sat under a type name that a milestone mentioned and
were owned by nothing that said so.

*(Was "twenty-nine" until 2026-08-28, in the same paragraph that names nine and
twenty-five two lines above — 9 + 25 is 34. The 29 came from
`SCRATCH.md/MILESTONE.md` §0.3 counting the list at **twenty** when §2.2 gives it
twenty-five; the ledger is corrected too, and its "35 implied" becomes 40. Found
while transcribing the numbering at M2, which is the first time anything counted
those rows rather than repeating the count.)* Same failure as M3/M4 and DESIGN §6.1's eleven words, one
milestone later.

**This milestone also owns the empty list `satellite.main`'s parameter binds to.**
*(Decided 2026-08-28.)* DESIGN §3's hello world declares
`satellite.container.list<satellite.variable.string> arguments` and never reads it,
so what it needs is one empty `satellite.container.list` and no `string` at all.
**That is this milestone's type, not M8's** — an empty list is still a list, and a
milestone that constructs one has built the type. **M8.B therefore runs after this
one**, and it is why M8 was split at all; §8's opening carries the split.

**Thirty-nine numbered paths, and three of them are not under `container`.**
*(2026-08-28.)* `satellite.container` `1 4 (0)`, the map and its nine, the list and
its twenty-five — **and `satellite.system.threshold()` `1 22 5` and `(n)` `1 22 6`,
which move here from a namespace M19 otherwise owns.** They are spelled under
`system` because that is where a knob belongs beside `max_depth`, but they set how
loose a search may be over the ten-level ladder, and v1 says it in its own comment:
*"it is not a system FACT: uname and getpwuid answer what the machine is, and this
sets how the search behaves."* **The search power cannot ship without its dial.**

**`satellite.container.arguments` `1 4 3` and `satellite.container.result` `1 4 4`
are not this milestone's**, and saying so is what stops a reader taking "containers"
as the namespace rather than the two types. `1 4 3` is the type name M19's arguments
object answers to; `1 4 4` is §2.2's *"Satellite Orbit's answer"* and is M24's.

**Its second dial is M7's and it must not invent one.** The search walk reads
`satellite.library.system.max_depth` `1 14 2 2` — v1 has its own depth error text
for it, separate from the recursion ceiling's — and M7 is where the dial is built.
Two consumers of one dial is fine; two milestones each building it is not, and
neither entry said which until this pass.

**The sort primitive is part of this milestone and is not the search power.**
`sort()` `1 4 2 3` through `sort_up(key)` `1 4 2 7` are §1.1's *one primitive rather
than comparators*; the search power is v1's comparator ladder, ported. Two different
things that happen to land together, and saying so is what stops the next reader
assuming "the search power" covered sorting.

**M8.B — hello world.** *(Split from M8 on 2026-08-28; M8.A is the console, between
M7 and M9 above.)* DESIGN §3 runs, byte for byte. What is left once M8.A has built
the console, `satellite.main` and `satellite.return` is **one thing: the parameter**,
and the parameter is why this half is here rather than there.

**`satellite.main` declares `satellite.container.list<satellite.variable.string>
arguments`, and this milestone must say what it hands over.** *(Restored 2026-08-28,
reversing the 2026-08-27 removal.)* For one day M8 read "`satellite.main` takes no
arguments at this milestone, and that is what makes the milestone reachable",
resting on WORD_NUMBERS.md §2.2 writing `satellite.main` as `1 3 (0)` with `(0)`
read as *zero arguments*. WORD_NUMBERS §1.3 defines that marker as **a node reached
both bare and as a parent** — a fact about reaching `satellite.main`, not about
declaring it — so the argument was reading the authority backwards. DESIGN §3 has
the full reversal.

**What it costs is one empty list, and the author has decided whose it is.**
*(Decided 2026-08-28.)* Hello world never reads `arguments`, so no
`satellite.variable.string` value is ever constructed — **M9 is not a dependency** —
and none of M10's twenty-five list methods is reached. What remains is a single
empty `satellite.container.list` bound to the slot. **That list is M10's**, which is
what puts this milestone after M10 and is the whole reason M8 was split at all.

**The reason it went that way rather than the other is that owning it here would
have been M10's type built twice.** An empty `satellite.container.list` is still a
`satellite.container.list`; a milestone that constructs one has built the type, and
the type belongs to the milestone that says so. The alternative — a private
empty-list shape that M10 later replaces — is the kind of thing that looks free and
is discovered later as two implementations of one type.

**It hands `satellite.main` a slot named `arguments` before anything can make it
real**, and §7.7 puts the recognition of the name at resolve, which is M6. A program
written between here and the milestone that builds §7.7 will ask for
`arguments.username` and get an error that no document predicts unless this
milestone writes the handover down. That is the M3/M4 failure caught before it
happens instead of after.

**`example/hello_world.satl` is the done-when**, rather than a paragraph describing
one. DESIGN §3 is a byte-for-byte copy of that file so the two cannot drift. The
`arguments` **object** — `arguments.username`, `arguments.machine.cores` and the
rest of §7.7 — belongs to the milestone that builds §7.7, and **that milestone does
not exist yet**; `SCRATCH.md/MILESTONE_DRAFTS.md` has the draft. **Startup is
measured again against M1's number here**, because this is where DESIGN §3 itself
first runs.

**M8.5 — `satellite.help`, and the trie answering for itself.** *(Its own milestone
as of 2026-08-28.)* Three paths — `satellite.help` `1 19`, `satellite.help()`
`1 19 0`, `satellite.help(x)` `1 19 1` — **moved here out of
`SCRATCH.md/MILESTONE.md` §0.1**, which had them under *"nothing"* and sized them as
the cheapest row in the ledger: *"3 paths and DESIGN §4.6 makes it a walk of the
trie — nearly free once M2 lands."*

**Everything it needs is behind it.** M2 gives the trie and the interner, M5 the
refusal text, M7 the `handlers[path_id]` table, **M8.A** the console to print
through. Nothing later is required, which is the argument for putting it here rather
than at the end: **help that arrives last is help nobody had while the language was
being built.**

**Its real floor is M8.A, not M8.B, and that is worth knowing.** *(2026-08-28.)*
Help needs a console and a `main` to run inside; it does not need the parameter, so
nothing stops this milestone landing immediately after M8.A and giving M9, M9.5 and
M10 a live account of themselves while they are being built. It is left after M8.B
because that is where the split put it and moving it is a second decision — but **if
help is wanted during M9 and M10, this is the one that can move, and it moves
without consequence.**

**Help is a walk, not a document, and v1 is the evidence for why.**
`old_versions/first_satellite/src/evaluator/help.cpp` is 221 lines and
`help_topics.cpp` another 315, all of it string literals, and it had already
drifted from the language it describes. `help_for_module()` answers for six modules
— `console`, `time`, `file`, `directory`, `system`, `random` — and returns the empty
string for every other name, while its own comment states the contract that makes
that a defect: *"An empty answer means the name is not a module, which is how both
callers tell."* `satellite.analyze` is a module in v1, advertised two groups above
in the same file's overview as `satellite.analyze("file.satl")`, and asking help
about it answers as though it does not exist. **A help text that drifts is the
thing DESIGN §4.6 exists to end**, and it ended up inside the file that was supposed
to be the language's own account of itself.

**§4.6 needs one correction before it is safe to build, and this milestone is where
it lands: the trie is what is *numbered*, not what is *built*.** §4.6 says *"help
cannot drift from what exists — the trie **is** what exists."* After M2 the trie
holds every path in `words.def`, so a help that walks it at this milestone
advertises `satellite.network.https(port, cert, key)` `1 20 7` and the other 121
paths the ledger counts as unscheduled. That is **worse than v1**, which at least
only listed what somebody had written, and it breaks DESIGN §1.1 outright — telling
a user a path works when nothing implements it is doing something behind their
back, and *a refusal in plain words beats a guess.*

**The fix needs no new machinery: help prints a node when `handlers[path_id]` is
non-null.** That table is already the dispatch mechanism (DESIGN §4.5, and this
plan's §6 where M7 builds it), so **the same table that decides whether a call runs
decides whether help mentions it.** Help cannot advertise what cannot run and
cannot omit what can. §4.6's sentence then becomes true as written, one word
narrower — the trie is what exists; the handler table is what *works*.

**The value listing stops being a switch, and that closes an open handoff.** v1's
`help_for(const Value &)` dispatches on `value.index()`, one of exactly two places
in v1's tree that read a raw variant index, and the **M10.5 draft in
`SCRATCH.md/MILESTONE_DRAFTS.md` leaves its new arm *"either taken here or left for
whoever gets `satellite.help`."*** This milestone takes neither, because the switch
should not exist: **a value's type is a node and its methods are that node's
children.** `satellite.variable.string` is `1 6 1` and its sixteen methods are
`1 6 1 1`–`1 6 1 16`, so *what does this value answer to* is the same walk started
lower down. The function is deleted rather than extended, and M10.5 appending
`ResultRef` to `Value` stops touching help at all.

**The three shapes are one walk at three depths.** Bare `satellite.help` and
`satellite.help()` start at the root; `satellite.help(x)` starts at `x`'s node. The
parentheses being optional is why WORD_NUMBERS §2.2 carries a bare `1 19` row
alongside `1 19 0` and `1 19 1` — one of only three parents in that table written
without the `(0)` marker (`SCRATCH.md/THREADS.md` finding 211 has the other two).

**Open, and it is the one thing here that is not already decided: a topic is not a
path.** v1 accepted `satellite.help(random)` and `satellite.help("random")` for
seven topic pages — `arguments`, `random`, `fast`, `normal`, `ultra`,
`random.ultra`, `wide` — and none of those is a path with a number. Under §1's
generating rule a bare word is user-owned, so **this is the second place in the
language where a bare identifier means something language-owned**, the first being
§7.7's six spellings of `arguments`. It is the same shape and it should be settled
the same way: the language *recognises* a name rather than introducing one. Whether
the seven survive at all, and where their text lives if they do, is undecided —
**but it must not be a second document**, because a second document is the drift
§4.6 is removing. Per-node text in `words.def` is the shape that keeps the walk the
only source.

**Not this milestone.** `satellite.analyze` `1 16`, which still has no milestone
anywhere, and the topic *pages* as prose. Adding a node's one-line description to
`words.def` is this milestone; writing seven essays is not.

**Done when** `satl` runs a program whose whole body is `satellite.help` and the
output names exactly the paths that are built when it runs — everything through M10
and M8.B, in build order, and no others — so the same unedited program run again at
M13 prints a different and equally correct language. Two
checks make it self-verifying, which no earlier milestone is: the output is
comparable to the non-null entries of `handlers[]` by construction, and
`satellite.help(satellite.network)` **refuses in plain words** rather than printing
seven shapes nobody has written.

**M18 — persistence: files and directories.** *(New 2026-08-28, corrected from the
2026-08-27 draft. After M8.5.)* **Twenty numbered paths**, in three of
`SCRATCH.md/MILESTONE.md` §0.1's rows — the 5th, 8th and 10th of seventeen, not a
contiguous block and not the largest one; `satellite.system` alone was thirty:

- the module face — `satellite.file` `1 8 (0)`, `.new(path)` `1 8 1`, `.open`
  `1 8 2`, `.clear` `1 8 3`, `.new(path, mode)` `1 8 4`, which is WORD_NUMBERS
  §1.3's own worked example of the variadic split
- the handle — `satellite.variable.file` `1 6 2 (0)`, `new` `1 6 2 1`, `open`
  `1 6 2 2`, `read_line` `1 6 2 3`, `write_line(s)` `1 6 2 4`, `read_all`
  `1 6 2 5`, `close` `1 6 2 6`, `exists` `1 6 2 7`
- the directory — `satellite.directory` `1 18 (0)`, `.change` `1 18 1`, `.current`
  `1 18 2`, `.exists` `1 18 3`, `.list()` `1 18 4`, `.list(d)` `1 18 5`
- **`satellite.system.delete` `1 22 1`**, which is the twentieth and which the draft
  argued at length belonged to somebody else — see below.

**Of the nineteen in the first three groups, "Later" reached only
`satellite.variable.file`'s eight, and Later is not a milestone.** The other eleven
were reached by nothing: §0.0 corrected the reading that had Later covering
`satellite.file` `1 8` — Later names `satellite.variable.file` and
`satellite.variable.time`, **different nodes**, which is the same one-spelling-two-nodes
hazard DESIGN §4.4 and WORD_NUMBERS §2.3 describe. **This milestone strikes
`satellite.variable.file` from that list** and closes `SCRATCH.md/MILESTONE.md`
§1's five-methods row.

**It writes no new syntax, and a plan for it that contains a parse rule is wrong.**
M4 owns DESIGN §6.1's eleven segment-1 words, `variable` among them, so
`satellite.variable.file f = satellite.file.open(p, "read")` already parses as a
declaration on `path[1] == "variable"`; §6.1's last row parses `satellite.file.*`
and `satellite.directory.*` as modules, needing a `(`; and §6.2's postfix loop gives
`f.read_line()` and `satellite.file.new(p).close()` for nothing. That is M4 owning
the eleven words and not saying so, one milestone further on — so this one says so.

**What it does not own.** M2 registers all twenty nodes and their numbers, **because
all twenty are in §2.2** — which is M2's own rule, *"a transcription of
WORD_NUMBERS.md and nothing else"*, and not because the v1 sweep found them; five of
them (`1 6 2 3`–`1 6 2 7`) came from settling QUAD.md §3 on 2026-08-27, among the
71 numbers that took §2.2 from 144 rows to 215. M5 owns the shape of every message
here, **but not the mode-word suggestion** — see the open item. M7 owns `Value`'s
reference-type handle alternative and §6.4 qualification 2's receiver-binding tag,
**and M7's own entry now says both**, because a constraint that lives only in a
later milestone's prose is the constraint that gets settled by accident. M10 owns
the list that `1 18 4` and `1 18 5` return, sorted by M10's `sort()` `1 4 2 3` and
not by a second sort here.

**SATC.md §5's atomic write is a different mechanism**, not an early version of this
one: tmp, `fsync`, rename is M4.5 doing the interpreter's own C++ file I/O, and a
reader who has just landed M4.5 could reasonably think the ground was taken.

**`satellite.system.delete` `1 22 1` moves here, and the demonstration is why.**
v1 argues in twenty lines that `unlink` acts on a **name**, so one verb covers a
file and an empty directory and belongs under neither — which is why the number is
under `system` and why M19 does not have it. But v1's body accepts an **open
`satellite.variable.file` handle** as well as a string, so the only milestone that
can give it its second argument shape is this one; and without a removal verb
**this milestone's own done-when runs exactly once.** `1 8 1` is `O_EXCL` — *new
says make one that is not there* — so on a second run the first `new` is the call
that comes back not-ok and the round trip fails, while `1 8 3` clear *"empties a
file that goes on existing"*. A done-when that passes once and cannot restore its
own tree is not a thing that works and can be demonstrated. **Taking `1 22 1` here
is the smaller of the two fixes** and it removes a circular dependency the draft
asserted from the outside.

**`satellite.directory` has no create verb, and that is a real hole under a node
this milestone otherwise finishes.** §2.2 gives it change / current / exists /
list() / list(d) and nothing that makes a directory. The done-when below is written
so that it needs none; **whether one is minted is the numbering's, and it is named
here so it is not discovered inside a demonstration.**

**The port is a rewrite, and what survives it is the arguments.** About 849 lines
under `old_versions/first_satellite/src/`: `modules_file.cpp` (304),
`methods_file.cpp` (324), `modules_directory.cpp` (221), plus `FileHandle` in
`satellite_value/value_types.hpp` and the six file/directory `format.def` rows,
which are the arity evidence WORD_NUMBERS §4 says has to be read out of the v1
evaluator rather than guessed. Two of the three files are already over §3's target.
Every arm is the `full == "satellite.file.open"` string compare §7 throws away;
every failure is `fail()`-then-return-`nullptr`, which DESIGN §9.1 throws away. What
ports is the comment culture §6 says to keep — the `O_RDWR|O_APPEND` paragraph, the
rewind paragraph, the `O_EXCL` paragraph, the `.` and `..` paragraph, the
sort-as-`SatString`s paragraph. The `std::atomic` fd comes across with it, because
DESIGN §8's reference semantics are what make two handles race; **M12 is when that
first gets exercised, not when it gets written.**

**`helpers_listing.cpp` and the two `helpers_file_facts` files do not port** — 418
lines of columnar `ls`-style rendering, with no number in §2.2 and nothing to do
with `.list`, which returns plain sorted names. A reader sweeping v1 for "file"
finds that half first. **It is not only the REPL's echo, though**: v1 also reaches it
as a list method, `.lines()`, and v1's own comment says the rendering exists because
*"the commonest list anyone types at this prompt is `satellite.directory.list()`."*
`.lines()` has no number in §2.2, and **whether it gets one is M10's question or
M11.B's, not this milestone's to settle by declining it.**

**Open, and the first two stand between this milestone and its demonstration:**

- **The failed-open contract has no numbers.** v1's whole answer to DESIGN §9 for
  files is that a failed open is a **value** — the handle comes back holding `errno`
  and the caller asks `.ok()`. `.ok()`, `.path()` and `.error()` have **no rows in
  §2.2** and the v1 sweep missed all three, and v1's handle also has a `clear` of its
  own. **This milestone owns them as behaviour and is blocked only on their
  numbers**, which only the author assigns — so its real size is twenty paths plus
  the three-or-four the failure contract requires, and the moment those numbers
  exist they must not read as fresh uncovered rows under a node this plan calls
  finished.
- **The mode word is not a trie level, so it is not M5's "did you mean".** DESIGN
  §4.6's mechanism is edit distance over one node's *children*;
  `read` / `write` / `append` / `read_append` are string literals in argument
  position and `satellite.file.open` is one number, `1 8 2`, for all four. Either
  they fold — WORD_NUMBERS §1.5's own worked example of the literal-option fold is
  `satellite.file.open("filename", "read_append")`, and the fold gave `sort_down()`
  a row of its own at `1 4 2 5` — in which case the folded rows are the author's to
  assign and **M6 owns the fold** (its entry now says so); or they do not, and the
  bad-mode message is a runtime check this milestone owns, using M5's *reporter*
  rather than M5's *suggester*.
- **Eleven of the twenty rows carry no call shape**, and §1.3 makes the shape part
  of the number. `1 8 2`, `1 8 3` and `1 18 1`–`1 18 3` are the module rows where a
  sibling carries one and they do not; v1 records their arities as legibly as
  `open`'s — `SAT_PATH(P_FILE_OPEN, 1, 25, 31, 0, 2)`, and 1, 0, 1 for
  `change`, `current`, `exists`. **Only the author may write them into §2.2**, and a
  milestone that raises the question for two rows and silently ports the other three
  is how §1.3 gets decided by accident.
- **Which spelling constructs a file.** DESIGN §13's settled `satellite.thread.new`
  entry calls the two-part shape established *"exactly as `satellite.file.new` and
  `satellite.time.new` already do"*, which reads as: `1 8 1` and `1 8 4` are what a
  program writes, and `1 6 2 1` is the dispatch-table row §6.4 qualification 2
  describes. Confirm that reading — `1 6 2 1`'s entire origin is §6.4's example
  sentence `my_file.new()`, and v1 has no `new` handle method at all. `open` is
  **not** a second collision: `1 8 2` opens a path, `1 6 2 2` reopens a handle that
  was closed or whose open failed.
- **`1 8 3` `clear` has no v1 module form to read.** v1's `satellite.file` module is
  `new` and `open` and nothing else — its `clear` is a handle method (`ftruncate`
  then `lseek`) and its delete is `satellite.system.delete`, by v1's own argument.
  So decide whether `1 8 3` is `clear(path)`, the module face of the handle method,
  or a row that should never have left the handle.
- **`read_line` `1 6 2 3` wants the one thing v1 refused.** v1's `.read()` seeks to
  0 on every call, and the recorded reason is that with no `.seek()` in the language,
  *read from wherever the offset happens to be* is a question no satellite program
  can pose. Does `read_line` advance a per-handle cursor, and what does `read_all`
  `1 6 2 5` do to that cursor when both are called on one handle?
- **`write_line(s)` `1 6 2 4` overrules an argument that is written down.** v1's
  `.write(s)` deliberately appends no newline: otherwise a file with no trailing
  newline, and a line assembled from several writes, both become unwritable, and
  §5.4 gave the language a real `\n` so `f.write("x\n")` already says what it means.
  Either `write_line` is a second verb beside a byte-exact write, or that argument
  loses. Say which — DESIGN §5.5 refuses `<<` for good, and this is the only surface
  a `.sky` writer can use.
- **`exists` `1 6 2 7` sits on the type node**, so §6.4 desugars it to a call whose
  first argument binds a receiver — and *does this file exist* is asked before a
  handle exists. Either it asks about an open handle's own path, which is a narrow
  question, or it wants to be a module path under `1 8` the way
  `satellite.directory.exists` `1 18 3` is.
- **`.list` is the one failure in its module that is an error and not a value**,
  because the empty list already means an empty directory and spending it on *there
  was no directory* makes the two indistinguishable. Defensible under DESIGN §9's
  reporter — but re-affirm it rather than inherit it by porting.

**Done when two programs run under `satl --run`.** The first is a round trip over
all twenty paths printing one `PASS`, and it **leaves the tree as it found it**, so
it can run twice: create with `1 8 1`, confirm a second `new` on the same path comes
back as a value that is not ok rather than clobbering the file (DESIGN §1.1 — *never
silently truncate*), write N lines, `close` and read the status, reopen with `1 8 2`
and `read_line` until nothing with the lines identical and in order, `read_all` and
assert it equals them joined, **open a path that does not exist, print the reason,
and keep running** — DESIGN §9's ordinary case, and the one assertion that cannot be
faked — ask for a mode word that is not one of the four and get a code, a span and a
caret naming them, then `1 18 2`, `1 18 4` with the new file in it, `1 18 3` true
then false, `1 18 1` into a directory that exists and a `1 18 1` onto a plain file
that returns false without dying, `1 18 5`, `1 8 3` leaving the file at zero bytes,
and `1 22 1` removing it — **both shapes, the path and the open handle** — with
`1 18 3` false afterwards.

The second is **QUAD's corpus reader** — `quad_main.cpp:184–197` — written in
satellite by hand against DESIGN.md: `read_line` until nothing, skip empty lines, a
line starting `"# "` opens a document (`starts_with(x)` `1 6 1 6`,
`substring(start, end)` `1 6 1 5` — §2.2 has no one-argument shape, so the call is
written with both), count lines and documents, print the two counts, and they must
match the C++ QUAD's on the same file. It needs no float and no `pow`.

**It is not the milestone QUAD.md §4 asks for, and this milestone does not claim to
be.** §4 wants *"one mechanism out of `mind.hpp`, chosen because it exercises floats,
containers, sorting and persistence at once"*; the corpus reader is in
`quad_main.cpp`, touches no float and no sort, and QUAD.md §5, `MILESTONE.md` §4 and
SESSION.md §5.7 all independently name the candidate as `Sky::decay` plus
`Rack::draw`. **That is M20**, and claiming it here would strike the ledger's row
and orphan the one thing three documents agree on.

**Its slot, stated as a relation rather than a decimal.** After M10, because
`.list()` returns M10's list and there is no faking it — a directory listing that is
not a list is not the thing. The other seventeen need only M9's strings: a path is a
string, a mode word is a string, a line is a string, and a `.sky` record is
`split(separator)` `1 6 1 10` and `to_number` `1 6 1 13`. **It inherits M9.5's
undecided rounding rule only through M10**, so if the float stalls, the thirteen
paths under `1 8` and `1 6 2` can run at M9 — that is the seam, and splitting there
orphans `satellite.directory` a second time, which is the precise failure
`MILESTONE.md` exists to record.

**M19 — the machine's facts, in the language.** *(New 2026-08-28, corrected from
the 2026-08-27 draft. After M18.)* **Thirty-seven numbered paths** — the
`satellite.system` namespace, the `arguments` object DESIGN §7.7 specifies, and the
type name an error message needs. It is the second-largest single milestone in
this list by paths, behind M10's thirty-nine once `threshold` moves there — **the
two are within two of each other, and the comparison is over behaviour rather than
over `words.def`, since M2 names all 222.**

- **`satellite.system` `1 22 (0)` and twenty-six below it**: `.environment`
  `1 22 2`, `.home` `1 22 3`, and all of `.memory` `1 22 4 (0)` — `.bit` `1 22 4 1`
  and `.frequency` `1 22 4 2` off SMBIOS type 17, `.main()` `1 22 4 3`, `.swap`
  `1 22 4 4 (0)` with `1 22 4 4 1`–`1 22 4 4 6`, `.this` `1 22 4 5 (0)` with
  `1 22 4 5 1`–`1 22 4 5 5`, then **`.free()` `.total()` `.used()` at `1 22 4 6`–
  `1 22 4 8`, `.main`'s unit shape at `1 22 4 9` — six slots from `.main()`
  `1 22 4 3`, which is §1.2 doing exactly what it promises — and `.free(unit)`
  `.total(unit)` `.used(unit)` at `1 22 4 10`–`1 22 4 12`.** *(That enumeration is a
  correction: the draft glossed the whole seven-wide run as three verbs and their
  unit forms, which is six things, and its total of 27 came out right only because
  the range is seven wide. `1 22 4 9` is the number a reader building from the
  sentence would never build.)*
- **`satellite.library.main` `1 14 1 (0)` and `satellite.library.main.arguments`
  `1 14 1 1 (0)` with its seven** — `.machine` `1 14 1 1 1 (0)`, `.machine.cores`
  `1 14 1 1 1 1`, `.machine.cpu` `1 14 1 1 1 2`, `.machine.threads`
  `1 14 1 1 1 3`, `.memory` `1 14 1 1 2 (0)`, `.memory.total` `1 14 1 1 2 1`,
  `.username` `1 14 1 1 3`. **`memory` is a sibling of `machine`, not a child**, and
  §8.1 discusses `satellite.library.main`'s numbering at M2 while nothing built it.
- **`satellite.container.arguments` `1 4 3`**, the type name an error message uses
  so it does not send the reader to the list's method table.

**Three of `satellite.system`'s thirty are not this milestone's**, and all three are
named here so the subtraction is visible. `satellite.system.threshold()` `1 22 5` and
`(n)` `1 22 6` are **M10's**: they are spelled under `system` because that is where
a knob belongs beside `max_depth`, but they set how loose a search may be over the
ten-level ladder, and v1 says it in its own comment — *"it is not a system FACT:
uname and getpwuid answer what the machine is, and this sets how the search
behaves."* `satellite.system.delete` `1 22 1` is **M18's**, because its second
argument shape is an open `satellite.variable.file` handle and M18 is the only
milestone that can hand it one.

**M14 built the readers; this milestone builds the language over them.** That is the
seam the 2026-08-27 draft was half-arguing for and got the wrong way round:
`memory_facts.cpp`, `host_facts.cpp` and `stack_facts.cpp` are consumed at M6.5, M7
and M8.A, so a milestone here cannot introduce them. **What was never inside
`system_facts/` at all is the language surface**, and it is most of the work:
`modules_system.cpp` (321 lines) is the actual `satellite.system.*` dispatch, the
unit table and every error message — over §3's target, so it splits by subject;
`methods_containers.cpp:118–199`, `subscripts.cpp`, `helpers.cpp:174–190`,
`value_arguments.hpp` and `value_printer.hpp:114–135` are the arguments object.
**`system_facts/system.cpp` (263 lines) splits across two milestones**: its
`arguments_for()` — the assembler nothing else has — comes here with
`arguments_facts.cpp` (113), and its `library_path()` and the
`-DSATELLITE_LIB_DIR` / `VERSION_DEFS` build coupling go to **M21**, which is where
`satellite.include` of another file lands. **`helpers_limits.cpp` is not this
milestone's at all**: its 121 lines are entirely `max_depth` and `division_digits`,
which are M7's and M6.5's dials, and `min_free_mb` — the one dial that was ever in
question — is eight lines inside M14's watchdog loop and needs no file.

**`satellite.system.environment` `1 22 2` is the one path here that is not a port.**
`SCRATCH.md/WORD_SURFACE.md` sources it to `v1docs` alone — not the registry, not
the evaluator, not the programs, which every other `satellite.system` row carries —
and v1's own comment says *"environment in general is
`satellite.system.environment(name)`'s job"*, which is to say it never built it. It
takes an argument and §2.2 has no `(name)` shape for it. So it is **new work with a
decision of its own** — whether an environment answer is the whole block, one named
variable, or a map — and it is in the open list rather than in the port.

**It needs no float, and that is worth saying because everything around it is
waiting on one.** Every unit is `b`/`kb`/`mb`/`gb`/`tb`, default `mb`, every divisor
a power of 1024, and a decimal division by 2ⁿ terminates exactly. M9.5's undecided
rounding rule does not reach this milestone.

**M2 owns the name and this milestone owns the behaviour.** All thirty-seven are in
`words.def` and come out of `satl --words` long before any of them answers anything,
so a sweep that reads the dump as coverage reads `1 22 4 3` as done. Saying it in
both places is what stops the next audit making that mistake — as is the fact that
`satellite.library` `1 14 (0)` parses at M4 as one of DESIGN §6.1's eleven segment-1
words, so `satellite.library.system.min_free_mb = 8192` **parses three milestones
before it means anything**, and M14 is where it starts meaning something.

**`Value` gains one alternative here and M7 has been told.** v1's `ArgsRef` is a
variant arm appended after the fact, and appending it and `ResultRef` both left
`sizeof(Value)` at 40 with the static_assert holding. DESIGN §8.2 budgets that 40
bytes and §6.1 calls `sizeof` *"the one number that could make this port not fit"*,
so **M7's entry now records that the variant is append-only and that these two are
the known future appends**; this milestone appends one and re-runs M7's assert.
v1's other consequence does not follow: its `help_for(const Value &)` switch on a
raw variant index is **deleted rather than extended** at M8.5, because a value's
type is a node and its methods are that node's children.

**Open, and none of these is small:**

- **The 33.** DESIGN §7.7 says each of v1's flat entries needs placing under a
  parent or dropping. v1 took a third option and shipped it: the entry names get
  **no registry ids at all**, the bare selector lowers to `.get(name)`, and
  *"spending a permanent registry id on `kernel_release` would be the registry
  recording a fact about somebody's machine."* §1.2's freeze is forever, so this is
  33 permanent choices and the largest irreversible decision in the milestone.
- **Three call shapes v1 accepts have no number.** The unit block is reached for
  every swap and `this` form, so v1 answers `.swap.used(unit)` and `.this.used(unit)`
  while §2.2 writes `1 22 4 4 3` and `1 22 4 5 3` without the parens their siblings
  carry — the table recording a sweep's arity rather than the code, and v1's own
  acceptance program writes `.this.used("kb")`. **`.environment(name)` is the
  third.** Only WORD_NUMBERS can assign them.
- **`satellite.container.arguments` `1 4 3` has no children and the object answers
  ten selectors** — `.length()`, `.count()`, `.names()`, `.to_string()`, `.lines()`,
  `.has(k)`, `.get(k)`, `.first()`, `.last()`, `.contains(x)`. That is M10's
  thirty-four container methods again, one level down, in a namespace nobody has
  looked at.
- **DESIGN §7.7's live-code mapping is wrong and this milestone is where it is
  fixed.** §7.7 reads *"`arguments.machine.threads`, `arguments.machine.cores` and
  `arguments.memory.total` … the same numbers again as codes 97, 98 and 99"*, which
  positionally makes 98 `cores`. v1's header and PLAN §6.1 both give **97 threads,
  98 mem_total_mb, 99 mem_used_mb**, and there is no live code for cores at all — so
  §7.7's *"three surfaces, one set of facts"* is **two** surfaces for
  `arguments.machine.cores`. Either a code is assigned or the rule is restated, and
  this milestone must not quietly pick a side of a contradiction whose whole point
  is that these must not disagree.
- **The seventh spelling, and `arguments[0]`.** A parameter named `argv` gets a
  plain list with no properties, silently, and DESIGN §9 says that silence is wrong.
  `arguments[0]` v1 already answered — `.length()` and numeric `[i]` cover the
  command line and nothing else, so index 0 is the program name — and the author
  only has to confirm that answer is kept.
- **The arguments object is a startup cost.** v1 builds all 33 eagerly — `uname`,
  `/etc/os-release`, `getpwuid`, `gethostname`, `getcwd`, two `readlink`s,
  `sysconf` — a dozen syscalls and a file parse against satl's own measured 0.01 ms
  share (§4.3). §7.7's redesign is the chance to make it lazy, and §9 says measure it
  here.
- **`.bit` `1 22 4 1` and `.frequency` `1 22 4 2` cannot be demonstrated as an
  ordinary user.** Both come from SMBIOS type 17 through
  `/sys/firmware/dmi/entries/*/raw`, and v1's comment on the failure path reads
  *"root-only, which is the usual answer"*, so both answer 0. **0 is a truthful
  answer and not an error**, and the done-when says so rather than letting two paths
  ship answering 0 with nothing that fails.

**The recognition of the six spellings is M6's and the spelling table is M2's**, and
neither said so before this pass. WORD_NUMBERS §2.3's third alias row is
`arg` `args` `argz` `argument` `arguments` `argumentz` — *one node, six spellings* —
which is the same mechanism `words.def` already carries nine of; **resolve is where
a parameter name matching one of them becomes the special variable**, and M6's entry
now says it. This milestone's headline demonstration rests on that table, and the
draft it comes from asserted the whole mechanism was M6's while M3's own sentence
called `hexadecimal` *"the language's one alias"*. **M3's word was wrong and is
corrected**: §2.3 has three rows.

**`SAT_PATH`'s four-segment limit does not come across.** v1's
`SAT_PATH(ident, s1, s2, s3, s4, arity)` could not hold
`satellite.system.memory.swap.used` `1 22 4 4 3`, which is five. **WORD_NUMBERS §1.3
is what refuses a segment limit** — §1.4's argument is about integer width, that one
`uint32_t` carries the whole path — and §4 credits the arguments subtree, six
numbers deep, as the clearest case for the refusal. That is a fix rather than a
port.

**Done when** one program prints `arguments.memory.total`,
`satellite.system.memory.total("mb")` and `satellite_string`'s live code 98 and
**asserts all three are the same number** — DESIGN §7.7's *"they must not be allowed
to disagree"* turned into something that can fail — and the same for threads across
`arguments.machine.threads` and code 97; when **`argz.machine.threads` answers 24 on
this machine**, which is the one line that proves §2.3's one-node-six-spellings
rather than five words that happen to be registered; when
`satellite.console.display(arguments)` prints all of it (§7.7); when `.bit` and
`.frequency` answer 0 as an ordinary user **and the program says that is the
truthful answer**; and when `satellite.system.home` and the memory verbs match
`example/full_test.satl:605–613`, which ports almost verbatim and is the regression
floor — five assertions over `.home`, `.total`, `.free`, `.main` and
`.this.used("kb")`, and that last line is also the proof that the missing `(unit)`
numbers are a real problem and not a hypothetical one.

**The watchdog is not in this done-when**, and that is the M14 seam holding: the
ceiling, the file and the exit path are demonstrated at M14 against no language at
all, and what is left here is `min_free_mb` being **readable and retunable from a
running program** through `satellite.library.system` — which is §4.5.3's *"the file
is where a machine's settings live before a program starts; the namespace is how a
running program reads and changes them"*, and the first time any milestone can show
both halves.

**M20 — a piece of QUAD, running.** *(New 2026-08-28. After M19.)* **The milestone
QUAD.md §4 has asked for since its first draft and this list did not have**, in §4's
own words: *"one whose done-when condition is a piece of QUAD, running. Not the whole
program — one mechanism out of `mind.hpp`, chosen because it exercises floats,
containers, sorting and persistence at once."* QUAD.md §5, `SCRATCH.md/MILESTONE.md`
§4 and SESSION.md §5.7 independently name the same candidate: **`Sky::decay` plus
`Rack::draw`.**

**It adds no numbered path, and it is not an acceptance test bolted onto the list.**
Every milestone before it demonstrates a mechanism the language provides. This one
demonstrates that a program somebody else wrote in another language can be written
in this one, which is the only claim that cannot be made by building any single
mechanism correctly — and QUAD.md §5 says why it has to be a separate milestone
rather than a clause in M10: *"the gap between 'the language has floats' and 'this
expression is writable' is where languages actually fail."*

**The two halves are not the same size and the milestone says so.**

- **`Sky::decay` uses no randomness at all.** `sky.hpp`'s decay is float arithmetic
  over a live list, and `quad_core.hpp`'s persistence term is `0.15 + 0.85·alt²` with
  no fractional `pow`. **It is M9.5 plus M10 and nothing else**, and it is
  demonstrable the day both have landed.
- **`Rack::draw` is a roulette wheel over `std::pow` weights and cannot be written
  today.** M16 records the three reasons in full: `satellite.random` has **no float
  draw** in any of its thirteen numbers, **no seed** — so QUAD's determinism
  invariant is not expressible — and a cheapest tier that **throws draws away for
  50–100 ms** against a 90 ms tick. `1 7 13` is free and M16 declines to assign it,
  because minting a number is the numbering's.

**So this milestone carries one blocker, and it is a number rather than a
decision.** A fourth shape under `satellite.random` that **takes a seed and does not
spin**, and a draw that can answer a fraction. Until it exists, `Rack::draw` is
writable only by drawing an integer and dividing, which is a `satellite.variable.float`
built out of two `satellite_number`s and is exactly the thing DESIGN §11's tiers
refuse to pretend to do. **Done when the number is assigned and the shape is built,
or when the refusal is written down beside the mechanism it blocks** — the same form
M9.5 uses for the rounding rule.

**Its floor is M9.5, M10, M16 and M18**, one for each thing QUAD.md §4 names: floats,
containers and sorting, the dice, and persistence. **`Sky::save` / `Sky::load` is the
fuller persistence half** and round-trips 164 doubles at six significant digits, which
is DESIGN §13's *"the right half's length IS the precision"* under load; it belongs
here rather than at M18, because M18's corpus reader deliberately touches no float.

**Done when** `Sky::decay` and `Rack::draw`, written in satellite by hand against
DESIGN.md, produce the same numbers as the C++ QUAD on the same input — and when the
day of writing them has produced its own list of what the language still cannot say.
**QUAD.md §5 predicts that list exists and this milestone is what finds it**: *"§3 is
a floor on what is missing and not a ceiling."* A milestone whose output includes new
holes is not a failed milestone; it is the only one in this section positioned to
find them before a user does.

**M11.A — the window.** *(Split from M11 on 2026-08-27, and built the same day —
the window landed ahead of the prompt it will host.)* The GTK4 + VTE binary, and
the `.desktop` entry joins the install here (§5.3), because the entry names a
binary and now there is one.

`satl-term` is a **fourth binary and not a fifth**: it links the window and nothing
of the runtime, and spawns the installed `satl` into its PTY, so it never interprets
and has nothing for `-march` to act on. It is built at the baseline like
`satl-cpu-level`, and it gets the haswell interpreter for free by spawning whichever
`satl` the installer chose. §4.2's table, `050-build.mk`'s "three binaries on x86-64
and one everywhere else", and LAYOUT.md's build-output table all become four and two
at this milestone. Measured on this machine 2026-08-27: **`satl` resolves 6 shared
objects and `satl-term` 79**, which is the split of §4.4 in one line.

**It is the same window the language hands out**, and that is why it is a milestone
rather than a build artefact. A program asks for one with

    satellite.variable.window my_console =
        satellite.window.console.new("window_title", 800, 600)

— a string and two numbers — so the title and the size are **arguments** in
`satl-term` too, reached as `--title` and `--size 800x600`. The binary is the first
caller of that signature and must not be a special case of it; M13's `dlopen`'d
library calls the same three values in. **`satellite.window.console` is `1 24 2` and its
`new(title, width, height)` is `1 24 2 1`** *(assigned 2026-08-28)*, so this
milestone's paths are real. It is a **different node from `satellite.window.new`
`1 24 1`**, which is still M13's and still reached by nothing that names it.

Done when: `satl-term` opens, spawns the `satl` beside it, and renders what it
prints — which today is `satl --repl` saying the prompt is not built yet.

**M11.B — the prompt, and the window stops closing.** The REPL itself: the prompt,
**the prompt's Ctrl-C — the byte `0x03`, because raw mode turns ISIG off and the
signal never arrives** — and the exit words. DESIGN §10.2 is why Ctrl-C is two
different things, and **the other one is M9's**: the SIGINT that sets a flag and lets
the walk stop itself at the next statement. *(The word was bare "Ctrl-C" until
2026-08-28, which read as owning both halves of a mechanism this milestone owns half
of.)*

**Two things it inherits rather than writes.** *(2026-08-28.)* `terminal_columns()`'s
ioctl with its 80-column fallback, and the clear — 14 lines that **M17 builds** for
`satellite.console.width` `1 5 6` and `clear()` `1 5 8`, and that v1's line editor
calls from two places. And **the emergency-exit hook**: M14 builds the watchdog's
exit path and deliberately registers no hook, because until there is a prompt nothing
has put the terminal into raw mode; **this milestone is the only registrar**, exactly
as v1's is, and it is what makes a `_exit(2)` from the watchdog leave a usable
terminal behind.

**M11.A closes the window when the interpreter exits cleanly, and M11.B ends that.**
Until there is a prompt, the child runs for milliseconds and a window that outlived
every one of them would only ever be a window nobody asked to keep — so M11.A closes
on a clean exit and **holds on a failure**, because a failed child is holding the
only copy of the reason and destroying the window destroys the message. That is what
makes M11.A demonstrable before this milestone exists: `satl --repl` answers
"not built yet" and exits `EXIT_NOT_YET`, so the window stays up with the
explanation on it.

Once the prompt is there the question is the other way round. **The window does not
close.** A person who has been typing at a prompt has a screen full of what they
did, and the exit word is the end of a session rather than the end of a window; the
close button is how a window closes. `on_child_exited` in
`src/programs/terminal.cpp` is the one function that changes, and it is written
knowing this — the clean-exit arm is marked as M11.A's and this milestone removes
it rather than discovering it.

Done when: a person can start `satl-term`, type at the prompt, and have what they
typed still on the screen after the interpreter is gone.

**M12 — threads.** `satellite.variable.thread`. The arena makes the walk atomic-free;
the Console already keeps output lines atomic.

**Six numbered paths, and its own line named one of them.** *(2026-08-28.)*
`satellite.thread` `1 23 (0)` and `.new` `1 23 1` — the module face, which is what a
program actually writes — and `satellite.variable.thread` `1 6 13 (0)` with
`start()` `1 6 13 1` and `join()` `1 6 13 2`, the two children assigned on
2026-08-28 out of `example/thread_test.satl`, which writes `my_thread.start()` before
`my_thread.join()`. **The ledger caught this one the day the numbers were minted**:
"M12 names only `satellite.variable.thread`" was true of five of these six.

**And `satellite.variable.capsule` `1 6 16` — the deferred call — is this
milestone's.** `satellite.thread.new(f(x))` takes an unevaluated call expression as
the thread body, which is the packaging semantics DESIGN §12 and §13 lean on to keep
§2 shut, and **nothing earlier needs it**: M10's `sort_down(key)` is §1.1's one
primitive rather than a comparator, which is exactly why sorting did not need
first-class capsules. `SCRATCH.md/MILESTONE.md` §1 filed it as *"M12 at the latest,
and probably earlier"* — it is M12, and if something earlier turns out to need it,
the argument that sorting did not is the thing that has to fall first.

**`parallel_for` is not here, and §4.5.1.2's decision assumes it.** The whole
parallelism surface of the language is the six paths above; the pool starts at
startup because *most satellite code will call `parallel_for`*, and that construct is
in no numbering, no document and no milestone. **This is the milestone it would
belong to**, and it is named here so the gap is visible from the one entry a reader
would look in.

**M13 — windows.** `libsatellite_window.so`, `dlopen`ed on first use. Marshalling to
the UI thread is satellite's job, never the user's (DESIGN §10.3).

**Three numbered paths, and "windows" in prose was reaching none of them.**
*(2026-08-28.)* `satellite.window` `1 24 (0)` and `satellite.window.new` `1 24 1` —
**a different node from `satellite.window.console` `1 24 2` and its
`new(title, width, height)` `1 24 2 1`, which are M11.A's** and which M11.A names —
plus the type `satellite.variable.window` `1 6 15`, which M11.A's own example
declares:

    satellite.variable.window my_console =
        satellite.window.console.new("window_title", 800, 600)

So M11.A wrote a program against a type no milestone built. **The declaration is
this milestone's and the constructor on the right-hand side is M11.A's**, which is
the split DESIGN §4.4 predicts whenever one spelling is two nodes, and it is why
naming all three here is a fix rather than an addition.

**M21 — another file: `satellite.include(spaceship)` and `satellite.analyze`.**
*(New 2026-08-28. After M13.)* **Two numbered paths and one mechanism.**
`satellite.include(spaceship)` `1 1 2` loads another `.satl` file and runs it;
`satellite.analyze` `1 16` reads another `.satl` file and reports on it without
running it. **They are the same act — the front end turned on a file that is not
the one being run — and they were the last two paths in this list that nothing
reached**, `1 1 2` sitting in "Later" as *"`satellite.include` of other files"* and
`1 16` sitting in nothing at all, which M8.5 named and declined in as many words.

**`satellite.include`'s other three shapes are already owned and this milestone
takes only the fourth.** `satellite.include` `1 1`, `include()` `1 1 0` and
`include(satellite)` `1 1 1` are M8.B's: DESIGN §3's hello world writes
`satellite.include(satellite)`, and §4 explains it as *"the one include form that
does nothing — include the runtime, which a running program already has. Every other
form names a **spaceship** and loads it."* **This milestone is "every other form".**

- **A spaceship is a unit of includable code** (DESIGN §13, decided), and
  `include_decl` is already in §6's grammar as a top-level form taking an
  expression, so **M4 parses it** and nothing new is parsed here.
- **What is new is a second program in one run**, and everything hard is in that
  sentence: whose numbering the loaded file's user names take (WORD_NUMBERS §3 — the
  next number free under the node that owns them, *allocated when the name is first
  met*, so two files met in one run allocate from one counter and a `PathId` is still
  valid for one run only); whether a spaceship is loaded once or once per include;
  and what a cycle does.
- **`satl` already knows how to find one.** v1's `system_facts/system.cpp` carries
  `library_path()` and the `-DSATELLITE_LIB_DIR` / `VERSION_DEFS` build coupling —
  *"system.o is the one object the Makefile compiles with `-DSATELLITE_LIB_DIR`"* —
  and **M19 hands that half of the file here** rather than porting it with the
  machine facts it has nothing to do with.
- **It is the pool's clearest tenant.** §4.5.1's own list of the tenants that collect
  the ~170-line figure rather than the ~2,650 one names *"`satellite.include` of
  another file"* first: the second file parsed in a run is the second batch, and the
  pool is already warm.

**`satellite.analyze` `1 16` is real in v1 and is the last of its modules with no
milestone.** v1 advertises it as `satellite.analyze("file.satl")` — one argument,
present in the registry, the evaluator and the programs — and M8.5 records the
defect that makes it worth naming: v1's own help answers for six modules and returns
the empty string for `analyze`, *"which is how both callers tell"* a name is not a
module, so **asking v1 about `analyze` answers as though it does not exist.** A help
that walks the trie (M8.5) cannot repeat that; a path that nothing builds still can.

**Its call shape has no number, and that is the milestone's first blocker.** §2.2
writes `satellite.analyze` `1 16` bare — one of only three parents in that table
written without the `(0)` marker — and v1 takes exactly one argument. So
`satellite.analyze(path)` needs a number and **only WORD_NUMBERS can assign it**;
the same question `1 8 2` and `1 18 1`–`1 18 3` raise at M18, and the answer should
be given to all of them at once.

**Second blocker: what an analysis says.** M4 already gives `satl --unparse` and M5
gives codes, spans, carets and *did you mean*; **what `analyze` adds over running
`satl` on the file is a decision nobody has taken.** v1's answer is not recoverable
as a specification — it is a module that existed and was never described. The honest
options are that it is the front end's diagnostics as a value a program can read,
which makes it the first piece of Satellite Orbit's shape (M24), or that it is
`--unparse` with a report, which makes it small. **It must not be a third help
system**; M8.5's whole argument is that a second document is the drift being removed.

**Done when** a program includes a spaceship that declares a capsule, calls it, and
prints its answer — with the spaceship's own `satellite.library` globals visible
under §7's rules and its user names numbered from the same counter the host file
used — and when `satellite.analyze` over a file with a deliberate error prints the
same code, span and caret M5 would print for it **without running a line of it**,
and over a clean file says so. **Two files, one run, one numbering** is the whole
claim.

**M22 — spacesuits.** *(New 2026-08-28. After M21.)* **Three numbered paths** —
`satellite.spacesuit` `1 10 (0)`, `satellite.protected` `1 11 (0)` and
`satellite.public` `1 12 (0)` — and the largest feature in this list by everything
except path count. DESIGN §13 has it under **Decided**: *"Classes are
`satellite.spacesuit`, with `satellite.protected` / `.public` blocks and a bare-name
type. Reference semantics."*

**Nothing here is speculative and that is unusual for a milestone this late.** §6's
grammar already writes the rule — `spacesuit_decl`, `suit_block`, `suit_section`,
and `type := IDENT` for a spacesuit named bare — **so M4 parses it**, as three of
DESIGN §6.1's eleven segment-1 words; §7 already says resolve runs every capsule
name first and then every spacesuit name, so **M6 resolves it**; and §7 already
records that a spacesuit is a reference type with a fresh slot that *"is not an
implementation detail."* What has never had a milestone is the part that runs.

**It sat in "Later, in no fixed order" with a grammar rule already written**, which
`SCRATCH.md/MILESTONE.md` §3 called out as its own row: *"a whole feature, with a
grammar rule already written, in the unordered pile."*

- **The type.** A bare `IDENT` in type position is a spacesuit (§6 grammar), which
  is the **second** place in the language where a bare identifier means something
  other than a user's own name — §7.7's six spellings of `arguments` is the first,
  and M8.5's topic pages would be the third. All three should be settled the same
  way: the language *recognises* a name rather than introducing one.
- **The two sections.** `satellite.protected` and `satellite.public` are blocks
  inside the suit, not modifiers on a member, which is why they are segment-1 words
  with numbers of their own rather than syntax.
- **Reference semantics, and DESIGN §12's refusals stay refused**: bare field access
  is accessor methods only, *"a spacesuit field is reachable from inside the
  spacesuit and nowhere else"*; user-defined operators and a spacesuit `to_string`
  the printer consults remain deferred, and this milestone must not quietly grant
  either by needing one for its own demonstration.
- **`satellite.returns(TYPE)` reaches its interesting case here.** DESIGN §13
  settles the return-type syntax and adds *"a constructor is the one member forbidden
  to declare one, since what a constructor produces is the object"* — a rule with
  nothing to apply to until spacesuits exist. M4 parses `returns` at `1 21`; this is
  where the refusal is enforced.
- **It is measured against a number v1 already has.** DESIGN §8.1 records the first
  satellite's spacesuit benchmark — a million constructions, a million prints and a
  million string transfers, at 2.79 s against the `double` version's 2.79 s — and
  §9 of this plan says a figure is re-measured on this machine rather than quoted.

**Blocker: two objects naming each other, and DESIGN §12 has already conceded the
argument.** Its garbage-collection entry says the original reasoning *"no longer
holds"* — refcounting sufficed because every value was immutable and cycles could
not be constructed, and **a spacesuit instance is mutable**, so *"two objects can
name each other and neither is ever freed. This is a real leak, it is the price of
reference semantics, and it is not fixable by being careful."* §12 names both
answers — a weak-reference field as the cheap partial one, a tracing collector over
the object table as the complete one — and **names neither as chosen.** This is the
milestone that creates the leak, so it is the milestone that has to say which, and
the choice is the author's.

**Done when** a program declares a spacesuit with a protected field and a public
accessor, constructs two of them, passes one into a capsule that mutates it, and
proves the caller sees the mutation — reference semantics, demonstrated rather than
asserted — and when reaching a protected field from outside is refused by name with
M5's caret rather than by silence.

**M23 — the network.** *(New 2026-08-28. After M22.)* **Nine numbered paths** —
`satellite.network` `1 20 (0)`, `.http(port)` `1 20 1`, `.https(host, port)`
`1 20 2`, `.new` `1 20 3`, `.open` `1 20 4`, `.receive` `1 20 5`,
`.http(host, port)` `1 20 6`, `.https(port, cert, key)` `1 20 7`, and the type
`satellite.variable.network` `1 6 12`.

**This is the one milestone in this list whose first job is a specification rather
than a build, and it says so instead of pretending otherwise.**
`SCRATCH.md/WORD_SURFACE.md` sources the whole namespace to `v1docs` and records the
position in one clause: *"v1 surface; nothing in this tree's documents promises it
yet."* There is **no v1 implementation** — five words in a first-satellite document,
swept into the numbering because §1's generating rule numbers what it finds — no
DESIGN section, and no sentence in this plan outside the ledger that counts it.
**That is a different and worse position than unscheduled**, and it is why this
milestone is late in build order and honest in shape: eight of its nine numbers
describe a protocol stack in five words.

**What the numbering already commits to, and it is more than it looks.** The two
`http` shapes are a **server** (`port`) and a **client** (`host, port`), and the two
`https` shapes are the same split with the server's `(port, cert, key)` — so §1.3's
*the arity is the identity* has already decided that one word serves both ends of a
connection and that the distinction is carried by argument count rather than by two
names. **That is a language decision made by a sweep**, and it is the first thing to
confirm or overturn, because §1.2 makes it permanent the moment anything is built
against it.

**M8.5 already refuses these paths and that is the shape of the guarantee.** Help
prints a node when `handlers[path_id]` is non-null, so
`satellite.help(satellite.network)` **refuses in plain words** rather than printing
seven shapes nobody has written — one of M8.5's two self-verifying checks. Until
this milestone lands, that refusal *is* the language's honest answer about the
network, and DESIGN §1.1 is why that beats a stub.

**Blockers, and none of them is this milestone's to take alone:**

- **What a `satellite.variable.network` `1 6 12` value is.** DESIGN §8's types table
  has no row for it. A socket is a reference type with the same two-handles-one-fd
  problem M18's file has, and DESIGN §8's reference semantics plus M18's
  `std::atomic` fd is the precedent to follow or to depart from deliberately.
- **What `receive` `1 20 5` blocks on, and on whose thread.** DESIGN §10.1's rule is
  that the program's own thread never blocks on the terminal; a socket is the same
  argument with a different fd, and M17 has already built the machinery — a reader
  thread and a queue — for the terminal case. **Whether that generalises is the
  design question this namespace exists to ask**, and answering it in a network
  milestone without saying so would be building a second reader.
- **Whether `https` implies a dependency.** Certificates and a TLS stack are the
  first thing in this language that cannot be written from libc, and §4's `ldd`
  discipline — six shared objects for `satl`, measured, not quoted — has been a
  stated property since M11.A. **A milestone that silently takes satl from six to
  a dozen would be changing a promise nobody wrote down as a promise**, and M16's
  vendored-PCG row in LAYOUT.md is the precedent for how a dependency arrives.
- **Nothing about it is QUAD's.** QUAD.md's table credits it with none of this, so
  unlike every other milestone in this list it has no acceptance program waiting for
  it. Its done-when has to be invented rather than found.

**Done when** a satellite program serves one HTTP request on a port and another
satellite program on the same machine fetches it and prints the body — two
processes, no C++ in the middle — **and when DESIGN has a section for the namespace
that this milestone wrote before it built anything.** The order matters: this is the
one milestone in the list where demonstrating first would *be* the specification,
which inverts what every other section of this plan does.

**M24 — Satellite Orbit and the wire format.** *(New 2026-08-28. Last in build
order.)* **One numbered path, `satellite.container.result` `1 4 4`** — §2.2's own
gloss for it is *"Satellite Orbit's answer"* — and one rule that three documents
already impose on a thing that does not exist.

**It is last because everything it distributes has to exist first**, and it is a
milestone rather than a line in a pile for one reason: **§8.1's constraint has no
enforcer until this exists.** *"Anything that persists a PathId must record the name
instead."* A language word's `PathId` is frozen forever and quotable; **a user's
capsule or spacesuit takes the next number free under its parent, allocated when the
name is first met, and is valid inside one run only** (WORD_NUMBERS §3, DESIGN §4.3,
§8.1's table). Orbit and the wire format are the two things that will want to write
one down, and SATC.md §3 imposes the same rule on the `.satc` — where M4.5 already
obeys it. **Nothing checks the rule for Orbit, and nothing can until Orbit is a
milestone**, which is precisely what `SCRATCH.md/MILESTONE.md` §3's row said: *"fine
where it is, but WORD_NUMBERS §3 and SATC.md §3 both impose a constraint on it, and
nothing will check that until it exists."*

**`1 4 4` is the whole of its numbered surface today, and that is a warning rather
than a size.** A result type under `container` says that Orbit's answer is a value a
program holds and asks about — the shape M21's `satellite.analyze` also gestures at
— and nothing else about the feature is numbered at all. **Every other path this
milestone needs would be new**, which under §1.2 means permanent, and under §1.1
means numbered in the order they are first written down. **Doing that badly is the
one mistake in this list that cannot be undone**, and it is the argument for this
milestone being last rather than for it being small.

**Blockers, and the first is not about the network:**

- **What Orbit is.** It has no section in DESIGN, no paragraph in this plan, and one
  gloss in the numbering. Every other milestone here could name the document that
  specifies it; this one cannot.
- **What a wire format persists.** The rule says never a user `PathId`; the name
  instead. Then a receiver has to allocate its own number for that name **under the
  same parent**, and §8.1's *"valid inside one run only"* becomes *valid inside one
  process only* — which is the same sentence and a much harder one, because two
  processes met the same name at different times.
- **Its relationship to M23.** Orbit over a socket is the obvious reading and the
  numbering does not say so; `satellite.container.result` is under `container`, not
  under `network`.

**Done when** two `satl` processes exchange a value and the receiver's answer is
identical to a local computation of it — and when a user name that both processes
know is proved to have **different numbers in each**, and to work anyway. That
second clause is the whole milestone: it is §8.1's rule turned into a test, and it
is the only test in this list that can fail for a reason no single process can see.

**"Later, in no fixed order" is empty, and this is where it used to be.** It held
eight entries — `satellite.variable.file`, `.time`, `.date`; `satellite.random.*`;
`satellite.variable.variant`; spacesuits; `satellite.include` of other files;
Satellite Orbit and the wire format — and every one of them now has a milestone that
names it: M18, M16, M16, M16, M15, M22, M21 and M24 in that order.
*(`satellite.variable.float` left it on 2026-08-27 and is M9.5, not M9, which this
sentence said until 2026-08-28.)*

**The list is not a milestone and things hid in it**, which is the only thing that
was ever wrong with it. `SCRATCH.md/MILESTONE.md` is the audit that proved it: run
per path rather than per namespace on **2026-08-27**, it found **122 of 218 numbered
paths reached by no milestone at all** and another 40 reached only by a sentence
about their parent. The three namespace sweeps before it reported 50, then 41, then
29 — none of them could see a path hiding under a parent §8 happens to name, which is
where most of them were. `satellite.random` was sixteen of them, sitting in this
list, with working and tested v1 code behind nine of its rows.

### 8.2 Every numbered path is named, and here is exactly what that claims

*(2026-08-28, at the end of the pass that added M14–M24.)* WORD_NUMBERS.md §2.2
holds **222 rows and 219 distinct numbers** — the three duplicates are §2.3's
`.range` aliases and nothing else. **All 222 rows are named by exactly one milestone
above**, counted mechanically against §2.2 rather than read off the prose:

| | paths | | | paths |
|---|---:|---|---|---:|
| M3 | 2 | | M13 | 3 |
| M4 | 3 | | M14 | 5 |
| M6 | 1 | | M15 | 2 |
| M6.5 | 11 | | M16 | 23 |
| M8.A | 7 | | M17 | 8 |
| M8.B | 4 | | M18 | 20 |
| M8.5 | 3 | | M19 | 37 |
| M9 | 26 | | M21 | 2 |
| M9.5 | 5 | | M22 | 3 |
| M10 | 39 | | M23 | 9 |
| M11.A | 2 | | M24 | 1 |
| M12 | 6 | | **total** | **222** |

**M1, M2, M4.5, M5, M7, M11.B and M20 hold none, and that is right rather than a
gap.** M2 registers all 222 and owns no behaviour; M5 and M7 build the machinery
every other row dispatches through; M20's whole content is a program. **The table
counts the milestone that makes a path answer, not the one that parses it** — M4
parses DESIGN §6.1's eleven segment-1 words and appears here with three, and M9's
`satellite.statement.*` rows say their parse rules land at M4.

**Four things this table does not claim, said plainly so nobody reads it as
finished:**

- **Named is not built, and it is not even specified.** M23's nine paths are five
  words in a first-satellite document, and its own entry says the first job is
  writing the DESIGN section that does not exist. M24 has one numbered path and no
  specification anywhere.
- **Eleven numbers are reserved and unbuilt on purpose**, listed by the milestone
  that owns them rather than left to a later audit: `1 7 1`, `1 7 2`, `1 7 3`,
  `1 7 6`, `1 7 9`, `1 7 12`, `1 9 2`, `1 6 7`, `1 6 8` (M16); `1 6 9` (M15);
  `1 22 2` (M19, the one `satellite.system` path with no v1 code behind it). A
  milestone that quietly leaves numbers behind it is how this document came to have
  a 121-path ledger.
- **Some numbers do not exist yet and are owed.** The failure contract's `.ok()`,
  `.path()` and `.error()` (M18); `.swap.used(unit)`, `.this.used(unit)` and
  `.environment(name)` (M19); a call shape for `satellite.analyze` `1 16` (M21) and
  for `1 8 2`, `1 8 3` and `1 18 1`–`1 18 3` (M18); a fourth `satellite.random`
  shape that takes a seed, for which `1 7 13` is free (M16, declined; M20, needed).
  **Only WORD_NUMBERS can assign them**, and when it does they arrive as new rows
  under nodes this table calls finished — which is exactly the shape of the failure
  that produced the 122 in the first place, and the reason they are enumerated here.
- **Ten milestones state something only the author can clear**, six of them under a
  heading that says *Blocker* and four inside an open list that stands in front of a
  demonstration: M9.5 (the rounding rule), M14 (three, all §4.5's), M15 (whether
  "nothing" is a state or a value), M16 (the clock, and what `1 7 1`–`1 7 3` name),
  M18 (the mode-word fold, and the failed-open contract's missing numbers), M19
  (DESIGN §7.7's live-code mapping, and the 33), M20 (a seeded draw with no number),
  M22 (cycles), M23 (four, starting with what a `satellite.variable.network` is),
  M24 (what Orbit is). **A milestone with a blocker in its done-when is scheduled; it
  is not startable**, and the two are worth telling apart.

**The cheapest ten came from a clause, not from a milestone.** Ten of the 121
uncovered paths were closed by adding a sentence to a milestone that already owned
the work and had never said so: **`satellite.bool`'s three to M9** — which had said
outright that they were not its — **`satellite.thread` and its `new` plus
`satellite.variable.thread`'s `start()` and `join()`, four, to M12**,
**`satellite.variable.capsule` `1 6 16` to M12**, and **`satellite.window` `1 24 (0)`
and `.new` `1 24 1` to M13**. Two more moved from *implied* to *named* in the same
clauses — `satellite.variable.window` `1 6 15`, which M13 now names beside the `.so`,
and `satellite.capsule` `1 2 (0)`, which M6 now names beside its frames. That is the
M3/M4 and M10 fix for the fourth and fifth time. **The lesson has not changed since
the first time it was written down here: the dangerous half is not the unscheduled
namespace, it is the path sitting under a parent some milestone happens to name.**

---

## 9. Measurement discipline

- **Measure on this machine, do not quote.** The first satellite's source says gtk4
  and vte pull "119 shared objects"; the real number here is 78. The shape held and
  the count did not, so the count is not repeated anywhere.
- **A number goes beside the decision it justifies**, in the file that makes the
  decision — not in a commit message, where nobody looks for it again.
- **Say what was measured and when.** Every figure in this document carries the
  machine or the date or both.
- **Startup is re-measured every milestone** against §4.3's floor.
- **Verify through the real code path.** If a check passes and the thing is still
  broken, the check is wrong. Running the *installed* binary is what proves an
  install, not comparing bytes.

---

*Companions: [DESIGN.md](DESIGN.md) — the generating rule, the syntax, the
numbering, scope, types, and what the language refuses. [LAYOUT.md](LAYOUT.md) —
every file in the tree and what it is for.*
