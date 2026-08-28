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
WORD_NUMBERS.md, taking §2.2 from 144 entries to 215, and M2 is the milestone that
transcribes them. The rest lands on M9 and M10, and QUAD.md §4 proposes a milestone
that does not exist yet: **one mechanism out of `mind.hpp`, running.**

## 1. Where things stand

**Milestone 1 landed 2026-08-26.** There is a `satl` that says what it is, says how
a file will be run, and refuses to pretend about the parts that do not exist. There
is no interpreter behind it yet.

What exists: the `Makefile` as an index over eight fragments under `make_support/`,
**ten C++ files totalling 1,153 lines**, and `satellite_enterprise/`, the Enterprise
Linux installer and the artwork. [LAYOUT.md](LAYOUT.md) lists all of it.

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

**Next: milestone 2**, the namespace trie and the path interner (§8). Its
`static_assert` was blocked from 2026-08-27 until 2026-08-28 on a "no duplicates"
clause the language's three deliberate aliases falsify; §8's M2 now carries the
four-property form that survives them, so the transcription can start.

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

**No C++ file exceeds 300 lines.**

This is tighter than the first satellite's 325 and it is a hard default rather than
a suggestion. The first satellite arrived at its ceiling late, by splitting a
2208-line `eval.cpp` and two 700-line functions *after* they had been written — and
the splits are visible in the result, with headers named `eval_internal.hpp`
existing to hold what an anonymous namespace used to, and comments explaining that
"the bodies below are UNCHANGED; they moved."

**Splitting a file after the fact preserves its shape. Writing to a ceiling changes
the shape.** So the ceiling applies from the first commit of every file, not from
the commit where somebody notices.

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
costs roughly 0.5–1.5 ms — fifty to a hundred and fifty times satl's entire current
startup — and the fixed word table is ~150 words, which a single thread walks in
microseconds. **Threading the table at startup is a guaranteed loss.**

Threading the walk over a *user's source* is a different question, because that
work scales with the program and the table does not. The number that decides it is
the **crossover**: how many satellite-rooted source lines a program needs before 24
threads beat 1, counting thread creation. **That is not yet measured** — §9's rule
is measure on this machine, do not quote, and this has not been.

So the shape to build toward: a pool that is **created lazily, on first real
threaded work**, sized from `THREAD_COUNT`, and shared by everything that needs
threads — the console's printer thread (DESIGN §10.1), parse-time interning, and
`satellite.variable.thread` at M12. One pool with three tenants amortises a cost
that none of them could justify alone, and a program that never threads never pays.

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

**M1 — `satl` exists and says how to use it.** *Landed 2026-08-26.* §1.

**M2 — the namespace trie and the path interner.** ← next
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
  least as a counter, years before the milestone that builds it. Decide whether M2
  owns a real allocator or a stub, and say which in the code.

**M3 — the lexer.** Tokens, spans, the reservation rule. Known words carry their
node identity out of the lexer; user-owned bare words carry their text. DESIGN §5.

**It also owns `satellite.variable.binary` `1 6 5` and `.hex` `1 6 11`**, and that was
unsaid until 2026-08-27. DESIGN §8.5 makes them real types **with literals** —
`x00FF` and `b1010`, where **the width is part of the value**, so `x0009` is not `x9`.
A literal is lexed, so the lexer decides them whether or not a milestone says so.
`hexadecimal` is the language's one alias for `hex` (WORD_NUMBERS §2.3), which the
lexer's spelling table has to know.

**M4 — the arena AST and the parser.** `uint32_t` node indices into a contiguous
arena, no `shared_ptr` anywhere in the tree. `satl --unparse file.satl` round-trips,
which is how we know the parser is right before anything can run.

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

**M6 — resolve.** Names to integer frame slots. Capsules, frames, the `SLOT_*`
sentinels. Resolved data in a side table indexed by arena node id, not `mutable` on
the node. DESIGN §7.

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

**M7 — the value model and closure compilation.** `Value` (40 bytes, the
static_assert comes too) and `Str`. `Number` arrives at M6.5 and this milestone is its
first consumer. The arena AST compiles to a closure tree.
Module calls dispatch through `handlers[path_id]`, and the **inline caches of §2.4
land here too** — third of the three adoptions §2.6 orders, and the milestone that
owns them. Recursion depth is bounded here.

**M8 — hello world.** DESIGN §3 runs. Console with its printer thread,
`satellite.main`, `satellite.return`. **Startup measured again against M1's number.**

**`satellite.main` declares `satellite.container.list<satellite.variable.string>
arguments`, and M8 must say what it hands over.** *(Restored 2026-08-28, reversing
the 2026-08-27 removal.)* For one day this milestone read "`satellite.main` takes no
arguments at this milestone, and that is what makes the milestone reachable",
resting on WORD_NUMBERS.md §2.2 writing `satellite.main` as `1 3 (0)` with `(0)`
read as *zero arguments*. WORD_NUMBERS §1.3 defines that marker as **a node reached
both bare and as a parent** — a fact about reaching `satellite.main`, not about
declaring it — so the argument was reading the authority backwards. DESIGN §3 has
the full reversal.

**What that costs this milestone is one empty list, and nothing else.** Hello world
never reads `arguments`, so no `satellite.variable.string` value is ever
constructed — **M9 is not a dependency** — and none of M10's twenty-five list
methods is reached. What remains is a single empty `satellite.container.list` bound
to the slot. **If M8 may own that empty list, this milestone does not move. If it is
M10's, M8 moves after M10, and the ordering is the decision to make here.** Say
which, in this paragraph, before M8 is built.

**Whichever way that goes, M8 hands `satellite.main` a slot named `arguments` two
milestones before anything can make it real**, and §7.7 puts the recognition of the
name at resolve, which is M6. A program written between M8 and the milestone that
builds §7.7 will therefore ask for `arguments.username` and get an error that no
document predicts unless this milestone writes the handover down. That is the M3/M4
failure caught before it happens instead of after.

**`example/hello_world.satl` is the done-when**, rather than a paragraph describing
one. DESIGN §3 is a byte-for-byte copy of that file so the two cannot drift. The
`arguments` **object** — `arguments.username`, `arguments.machine.cores` and the
rest of §7.7 — belongs to the milestone that builds §7.7, and **that milestone does
not exist yet**; `SCRATCH.md/MILESTONE_DRAFTS.md` has the draft.

**M8.5 — `satellite.help`, and the trie answering for itself.** *(Its own milestone
as of 2026-08-28.)* Three paths — `satellite.help` `1 19`, `satellite.help()`
`1 19 0`, `satellite.help(x)` `1 19 1` — **moved here out of
`SCRATCH.md/MILESTONE.md` §0.1**, which had them under *"nothing"* and sized them as
the cheapest row in the ledger: *"3 paths and DESIGN §4.6 makes it a walk of the
trie — nearly free once M2 lands."*

**Everything it needs is behind it.** M2 gives the trie and the interner, M5 the
refusal text, M7 the `handlers[path_id]` table, M8 the console to print through.
Nothing later is required, which is the argument for putting it here rather than at
the end: **help that arrives last is help nobody had while the language was being
built.**

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
output names exactly the paths M1–M8 built and no others — so the same unedited
program run again at M13 prints a different and equally correct language. Two
checks make it self-verifying, which no earlier milestone is: the output is
comparable to the non-null entries of `handlers[]` by construction, and
`satellite.help(satellite.network)` **refuses in plain words** rather than printing
seven shapes nobody has written.


**M9 — scalars and control flow.** `satellite.statement.if` `1 13 1`, `.for` `1 13 2`,
`.while` `1 13 3` and `.else` `1 13 4` — **their parse rules land at M4** (above);
what lands here is running them. `satellite.variable.bool`, `.number`, `.string` and
their methods.

**`satellite.bool` `1 17` is a different node and is not this milestone.** The type is
`satellite.variable.bool` `1 6 6`; the module constants `satellite.bool.true` `1 17 2`
and `.false` `1 17 1` hang off a top-level namespace that no milestone claims
(DESIGN §6.1 cites them as the module-constant case). Reading M9 as covering both is
the mistake this paragraph exists to stop.

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

**Done when the four operations run and — the blocker — the rounding rule is chosen.**
Truncate, half-up, or half-even. No representation escapes it: `pow` at a fractional
exponent is irrational, so the fractional half must be rounded to exist. QUAD's
determinism invariant means a program's behaviour depends on the answer.

**M10 — containers and the search power.** `satellite.container.list`,
`satellite.container.map`, **and their methods** — the map's nine `1 4 1 1`–`1 4 1 9`
and the list's twenty-five `1 4 2 1`–`1 4 2 25` — plus the search power ported close
to unchanged.

**That clause is a fix, not an addition.** *(2026-08-27.)* M9 writes "`.bool`,
`.number`, `.string` **and their methods**" and this milestone did not, so twenty-nine
numbered paths sat under a type name that a milestone mentioned and were owned by
nothing that said so. Same failure as M3/M4 and DESIGN §6.1's eleven words, one
milestone later.

**The sort primitive is part of this milestone and is not the search power.**
`sort()` `1 4 2 3` through `sort_up(key)` `1 4 2 7` are §1.1's *one primitive rather
than comparators*; the search power is v1's comparator ladder, ported. Two different
things that happen to land together, and saying so is what stops the next reader
assuming "the search power" covered sorting.

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
Ctrl-C, the exit words. DESIGN §10.2 is why Ctrl-C is two different things.

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

**M13 — windows.** `libsatellite_window.so`, `dlopen`ed on first use. Marshalling to
the UI thread is satellite's job, never the user's (DESIGN §10.3).

**Later, in no fixed order.** `satellite.variable.file`, `.time`, `.date`;
`satellite.random.*`; `satellite.variable.variant`; spacesuits;
`satellite.include` of other files; Satellite Orbit and the wire format.

*(`satellite.variable.float` was here until 2026-08-27 and is now M9.)*
**This list is not a milestone and things hide in it.**
[SCRATCH.md/MILESTONE.md](SCRATCH.md/MILESTONE.md) is the audit, and on
**2026-08-27** it was finally run per path rather than per namespace:
**124 of WORD_NUMBERS.md's 222 numbered paths are reached by no milestone at all**,
and another 35 only by a sentence about their parent. **56% of the language is not in
this list.** *(The sweep was run against 218 rows; four were added on 2026-08-28 —
`satellite.window.console` and its `new`, which M11.A names, and
`satellite.variable.thread`'s `start` and `join`, which M12 does not.)* The three namespace sweeps before it reported 50, then 41, then 29 —
none of them could see a path hiding under a parent §8 happens to name, which is where
most of them were.

`satellite.system` alone is 30 of the 122 and is named nowhere in this document except
here. `satellite.random` is 16 more, in "Later" above, which is not a milestone.

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
