# CPU-PLAN — one satl, a number kernel for every processor it runs on

The author, 2026-09-23, after M37's races showed `satl.haswell` no faster than the
ordinary satl:

> *"we need to work this in a way that we don't have to constantly update all of these
> different kernels, we write satellite_number kernel once, and never have to refer back
> to it again"*
>
> *"we will build 64-bit, 128-bit, 256-bit and 512-bit numbers ... and we can build all
> of the kernels, the four kernels, into... the single satellite executable and get rid
> of satl-cpu-level, and we'll build satl-cpu-level into the single satl executable"*
>
> *"we will display what bit is being used into the startup display"*

The processors and their names are in [ARCHITECTURES.md](ARCHITECTURES.md).

## The plan

**One satl.** Inside it are four builds of the inner loops of `satellite_number`, called
the *kernels*: the loops that multiply, add, subtract and divide runs of limbs. At start-up
satl asks the processor what it can run (`satl-cpu-level`'s job, moved inside), picks
the best kernel the processor allows, and says which:

    THE SATELLITE PROGRAMMING LANGUAGE
    VERSION 004 REVISION 08 BUILD 0019
    CLANG++ 24 ALMALINUX 10.2
    SATL 64-BIT (2006)
    --bit FOR INFO

`satl --bit` explains the choice: which kernel, why, and what the processor lacks for the
next one. The last two lines are the author's; where they sit under the title is a
proposal.

**Why only numbers.** Of everything satl does, the big-number loops are the one place
where a newer processor's instructions do real work. The walker is branches and
pointers, and no instruction set changes that. Bulk memory copies already use AVX2 in
the ordinary build, because glibc's `memcpy` picks its AVX2 version at run time. See
"Strings" below for the measurement.

**The machines.** Three of the four kernels have a machine to test on:

| Kernel | Runs on | Tested on |
|---|---|---|
| 64-bit (2006) | every 64-bit x86 | the author's 2006 computer, and everywhere else |
| 256-bit (2013) | Haswell and later | this Xeon E5-2670 v3 |
| the 13700K's | Raptor Lake | the i7-13700K |
| 512-bit (2018) | Palm Cove, Willow Cove, AMD Zen 4/5 | **none owned** |

## Measured, 2026-09-23

These numbers come from four agents, and nothing else ran while the race was timed:

- **callgrind** on reduced copies of the author's `race_program.satl` and
  `experiments/cpu_race/everything.satl`;
- **seven builds** of satl, all made from the same source;
- **a quiet race**: 5 rotated rounds per build, with the output checked against the
  shipped build;
- **stand-alone kernel benchmarks.**

The files are in that session's scratchpad; the numbers are here.

**What each thing is worth:**

| What | Measured | Over what |
|---|---|---|
| `-march=haswell`, whole build | within noise: −2.6% to +0.7% | both races |
| `-O3` | within noise | both races |
| **PGO** | **10.5–13.0% faster** (race), **7.7–9.6%** (everything) | every PGO run beat every non-PGO run |
| PGO + ThinLTO + BOLT | not separable from PGO alone, apart from about 2–3% on the race | both races |
| today's multiply loop at `-march=haswell` (clang already emits MULX) | 1.07× on the loop, 1.05× on `c = a * b` | 11 × 15 limbs, the race's numbers |
| hand-written MULX with intrinsics | **0.83× and 0.74×, slower** | same |
| GMP's Haswell assembly (`mul_basecase_coreihwl`) | **1.50×** | same |
| string `==` through `memcmp` instead of today's `char16_t` loop | **8.5×** (hand AVX2: 7.9×) | 29,696 characters |
| a name found ahead of time (a slot) vs today's rebuild-and-hash | **1.6 ns vs 38–46 ns** (82 ns for names over 15 characters) | one variable read |
| UTF-8 → 16-bit conversion at `-march=haswell` | 1.00× | 1 MiB |

**Where a turn's instructions go** (ordinary build, callgrind):

| | race_program | everything |
|---|---|---|
| big-number loops | **11.5%** (multiply 8.8%) | 0.7% |
| 16-bit string compare | 0 | **21.7%** |
| glibc `memcpy`/`memcmp` (already AVX2 in the ordinary build) | 1.9% | **28.6%**, mostly copying whole strings on every read |
| malloc/free | 10.2% | 2.9% |
| names: rebuilding them, the word-code search, the hash | **28.6%** | 12.0% |
| the rest of the walker | 45% | 32% |
| the 62 word libraries | 0.00% | 0.00% |

**What that says about the milestones.** It does not change their order, which is
the author's to set, but here is what each can give on the author's race:

- **CPU-2 to CPU-5, the kernels**, touch the 11.5%. Even GMP's assembly (1.5×) takes about
  **4%** off.
- **CPU-7, PGO**, gave **10–13%**, measured.
- **CPU-6, names ahead of time**, touches the 28.6%.

The kernels matter for programs that live in big numbers. This race lives in the walker.

## What the plan has to meet

These are the things the first draft did not account for. The first two change which
kernels are worth building.

### 1. The 13700K has no AVX-512

Asked of clang 24 (ARCHITECTURES.md has the whole table): `-march=raptorlake` defines
AVX2, BMI2 and ADX, and **no AVX-512**. Intel switched AVX-512 off on every Alder Lake
and Raptor Lake chip. Lion Cove does not have it either. So `satl.raptor` cannot be the
512-bit build. The only processors in the list that could run a 512-bit kernel are Palm
Cove and Willow Cove, and no machine here has either.

There are two ways to test a 512-bit kernel: Intel's SDE (a free emulator, not
installed; slow, but exact) or one machine that has AVX-512. A used Tiger Lake laptop or
a Zen 4 desktop would do.

### 2. A wider register is not a wider number

- **"128-bit" is what satl already does.** Every x86-64 multiply takes two 64-bit limbs
  and gives a 128-bit answer. That is the `unsigned __int128` in today's loop
  ([satellite_number.cpp:243](satellite/satellite_variable_number/satellite_number.cpp#L243)).
  The 128-bit SSE registers cannot multiply 64-bit limbs at all.
- **AVX2's 256-bit registers cannot multiply 64-bit limbs either.** Their widest
  whole-number multiply is 32 × 32 → 64, four at a time.

**What made big numbers faster since 2006 is three instructions, and only the last is
wide:**

| Instruction | Arrived | What it does for a big multiply | In the list |
|---|---|---|---|
| MUL + ADC | x86-64 (2003) | one limb × one limb, one carry chain | everything |
| **MULX** (BMI2) | Haswell (2013) | multiplies without touching the carry flag, so the loop can overlap | haswell and later |
| **ADCX / ADOX** (ADX) | Broadwell (2014) | two carry chains at once: the fast big-number loop | skylake, raptor, lion, palm, willow |
| **IFMA** (AVX-512) | Palm Cove (2018) | eight 52-bit multiply-adds per instruction | palm, willow |

So the four kernels that would each be faster than the one before are **64-bit (MUL),
MULX, MULX + ADX, and 512-bit IFMA**. Here are the author's four widths next to what
exists at each:

| Author's kernel | What exists at that width for numbers | Recommendation |
|---|---|---|
| 64-BIT (2006) | MUL + ADC: today's loop | keep |
| 128-BIT (2008) | nothing new: it *is* the 64-bit kernel | **replace with MULX + ADX**, the 13700K's kernel |
| 256-BIT (2013) | MULX (BMI2, same processors as AVX2) | keep, as the MULX kernel |
| 512-BIT (2018) | IFMA | keep, untestable here |

**DECISION OPEN (the author's):** the four kernels. The recommendation keeps his display
words for the widths that are real, and gives the 13700K its own kernel:

    SATL 64-BIT (2006)          intel, nehalem, sandy
    SATL 256-BIT (2013)         haswell                     MULX
    SATL 256-BIT ADX (2014)     skylake, raptor, lion       MULX + ADCX/ADOX
    SATL 512-BIT (2018)         palm, willow                IFMA

### 3. Kernels only help numbers above 18,446,744,073,709,551,615

A number that fits one limb never reaches a kernel. Its `+ - *` are inline in
[satellite_number.hpp](satellite/satellite_variable_number/satellite_number.hpp) and were
measured at 0.90 ns on 2026-09-16. `counter = counter + 1` gets nothing from any kernel.
The author's race multiplies a 200-digit number by a 280-digit one, so it does reach one.
How much of a turn that multiply is decides what a faster kernel can give.

**Measured 2026-09-23:** the big-number loops are 11.5% of the race's instructions. Each
multiply is 11 limbs × 15 limbs = 165 limb products and takes 262 ns through satl's
class. 20 ns of that is allocating the answer's vector, which no kernel touches.

### 4. Written once means the interface is fixed first

The kernels see only runs of limbs, through a small interface decided before the first
kernel is written:

    multiply(out, left, left_count, right, right_count)
    add / subtract (in place, with carry out)
    divide_in_place by one limb          -- the inner loop of turning a big number into text
    compare

Everything else stays in `satellite_number`, shared by all four: signs, the one-limb fast
path, allocation, and text. A kernel is then a few hundred lines that nobody has a reason
to open again. **Changing the interface is the one thing that makes you touch all four.**

What proves a kernel correct forever is a **differential test in check.sh**. It runs every
kernel the machine can run against the 64-bit kernel on edge cases: zero limbs, one limb,
all-ones limbs that carry the whole way, lengths 1 to 200, and every pairing of lengths.
Each kernel is green on the machine that runs it, and then left alone.

### 5. Two traps that only a 2006 machine would show

- **Shared code compiled for a newer processor.** If a kernel's `.cpp` is compiled with
  `-mbmi2` or `-mavx512ifma` and includes `<vector>` or any shared inline function, the
  linker keeps *one* copy of each such function. It may keep the one compiled with new
  instructions, and then the 2006 machine dies of an illegal instruction far from any
  kernel. The fix is that a kernel file includes nothing but `<cstdint>` and
  `<immintrin.h>`. Or each kernel function carries `__attribute__((target("...")))` in a
  file compiled for plain x86-64.
- **A floor check.** check.sh should fail when satl contains any instruction a Core 2
  lacks outside the kernels. M37's instruction census already has the pieces. The
  ordinary build passes today: clang's default target is plain `x86-64`, and the
  vendored GTK stack sets no `-march` (both checked 2026-09-23).

### 6. The 2006 computer needs a current Linux, and AlmaLinux 10 is not one it can run

satl needs `GLIBC_2.38` and `GLIBCXX_3.4.32`. AlmaLinux 10 needs a processor from
Haswell on for its main build, or x86-64-v2 (Nehalem on) for its v2 build. A Core 2 has
neither. The 2006 machine needs a distribution that still supports plain x86-64 and has
glibc 2.38 or newer, such as Debian 13 or Ubuntu 24.04. Its graphics chip will not do the
OpenGL 3.3 that GTK 4 draws with; GTK should fall back to drawing in software, but that
needs testing there.

### 7. The chooser needs two things M37's did not

- **An override.** For example `satl --bit 64 program.satl` forces a lower kernel. It
  makes a race one binary on one machine (`--bit 64` against the default). It is also the
  way out if a kernel is ever wrong on someone's machine. It could be a `config.ini` row
  as well.
- **The operating system's permission, not only the processor's.** AVX and AVX-512 can
  be present in the processor and switched off by the kernel or a hypervisor.
  `__builtin_cpu_supports` asks both, through XGETBV. A hand-written CPUID check must too.

Asking costs well under a microsecond, against a 14.7 ms start-up.

## Strings

**Measured 2026-09-23: strings need a one-line change, not a kernel.**

- **Comparing.** `satellite_string::compare` ([satellite_string.cpp:234](satellite/satellite_variable_string/satellite_string.cpp#L234))
  is libstdc++'s `char_traits<char16_t>::compare`: one character per trip round a loop
  that can stop early. That is why clang never vectorises it, under any `-march`. It is
  **21.7%** of `everything.satl`'s instructions. Testing equality with `memcmp` is
  **8.5× faster** on a 29,696-character string. That speed comes in the *ordinary*
  build, because glibc picks its AVX2 `memcmp` at run time, and it beats a hand-written
  AVX2 loop (7.9×). No processor-specific code is needed.
- **Copying.** Every read of a variable copies the whole value, so the 29,696-character
  strings are copied again and again: `memcpy` is 27.6% of `everything.satl`. That is
  the walker's problem (CPU-6's family), not a processor's.

Where SIMD strings would pay is **searching** big text: find, replace, split. That is
QUAD's work, reading corpora. satl has not built `.find`/`.replace` yet. When it does,
they should sit behind a kernel interface from the start, the same shape as numbers.

## The milestones

The author's order; CPU-6 and CPU-7 are his additions.

### CPU-1 — the processor test, inside satl

`satellite/cpu_level/cpu_level.cpp` (363 lines) becomes part of satl, run once at
start-up. It answers which kernel to use, the override (§7), and `--bit`. The title gets
its two lines. `arguments.cpu.*` gets a row saying which kernel is running; whether that
is `arguments.cpu.architecture` itself is the open question carried from M37.
`build/satl-cpu-level` goes.

**Choose by what the processor can do, never by its model.** Clang's
`target_clones("arch=haswell")` resolver tests the processor *model*, so a 13700K gets
the plain clone. `arch=x86-64-v3` tests features, and so does M37's chooser. Measured by
the kernel agent, 2026-09-23.

**DECISION OPEN:** what happens to `make cpus`, `build/cpu/` (53 builds, 13 minutes,
4.1 GB) and the nine `satl.<name>` executables. M37's race put `-march=haswell` within
noise of the ordinary build (25.25 s against 25.57 s, then 0.5% the other way). The
recommendation is one satl, with `make CPU=<clang name>` kept only if a race ever shows
it matters.

### CPU-2 — the 64-bit kernel (2006)

Today's loops, moved behind §4's interface unchanged, with nothing faster yet. This is
the milestone that fixes the interface, so it is the one to get right. It also brings the
differential test and the floor check (§5), and one real run on the 2006 computer.

### CPU-3 — the 128-bit kernel (the author's slot)

As written it would be CPU-2 again (§2). **Recommended instead: the MULX + ADX kernel**,
the 13700K's, tested there. It builds on CPU-4's MULX loop, so if taken, CPU-3 and CPU-4
swap.

**It has to be assembly.** Clang 24 never emits ADCX/ADOX: not from today's loop at
`-march=raptorlake`, and not even from `_addcarryx_u64`, which it turns back into
`adc`. Only inline assembly produced them, and GMP's Broadwell kernel is written that way.
The system libgmp here does not recognise the 13700K and falls back to generic code
there (0.98×).

### CPU-4 — the 256-bit kernel (2013)

MULX (BMI2), tested on this Xeon.

**The compiler already does the easy part** (measured 2026-09-23). Under
`-march=haswell`, clang turns today's loop into MULX: 1.07× on the loop, 1.05× on `c = a
* b`. A hand-written MULX loop with intrinsics was *slower* (0.83×). What beat it was
GMP's hand-written assembly: 1.50×, at the same 11 × 15 limbs. So this kernel means
writing assembly, or vendoring GMP's `mpn` layer (LGPL; its `--enable-fat` build already
picks per-processor code at run time), not writing intrinsics.

### CPU-5 — the 512-bit kernel (2018)

AVX-512 IFMA, the only kernel with no machine here. It is tested under Intel SDE, or on a
machine bought for it. Its speed can only be known on real hardware. Ask whether it is
worth building before there is a machine to race it on.

### CPU-6 — names looked up ahead of time

The biggest gain on every processor, independent of kernels. On 2026-09-16 each variable
access rebuilt a name from its 16-bit codes and hashed it (to be re-confirmed against
today's walker). `program_check.cpp` already sees every name in a capsule before the
walker runs, so it can give each one a slot number. The walker then reads slot *n* of an
array, which is what CPython does.

**Measured 2026-09-23:**

- A variable read today costs **38–46 ns**, and **82 ns** for a name over 15
  characters, which no longer fits in `std::string`'s own buffer and so allocates on
  every read. A slot found ahead of time costs **1.6 ns**.
- In the author's race:
  - `text_at` rebuilding the name is 13.8% of all instructions;
  - `word::code_of` repeats a binary search for a constant on every operand
    (`expression.cpp:1096`, `:1233`): 11.7%;
  - the hash lookup is 3.1%.

  Together that is **28.6%**, the largest single share. The gap measured that day, `i = i + 1` at 458 ns
against CPython's 87 ns, is mostly this. Names that appear only at run time (arguments
rows, `interpret`, spacesuit fields reached by name) keep a slow path.

### CPU-7 — PGO, LTO and BOLT — **BUILT 2026-09-23**, `c5b37f0`, in the plain `make`

The author: *"let's build LTO, PGO and BOLT into the regular make"*.
[make_support/045-optimise.mk](make_support/045-optimise.mk) does all three on every
`make`:

1. It builds satl a second time, instrumented, into `build/pgo-train/`.
2. It runs the four programs in [make_support/training/](make_support/training/)
   (numbers, strings, lists, capsules with spacesuits). None of them is a race program.
3. It compiles satl's objects with the merged profile and `-flto=thin`, and links them
   with lld.
4. BOLT instruments the result, trains it again, and rewrites it.

**Where each step stands:**

- **On this machine:** check.sh 786 passed, 0 failed, against the result.
- **What is left plain:** the 62 word libraries (0.00% of the time).
- **Development builds:** `make OPTIMISE=no` is the old plain `-O2`, for quick rebuilds.
- **When a tool is missing:** each step turns itself off when its tool is not beside the
  compiler, and the link line says which. g++ builds plain. AlmaLinux's clang 21 gets
  PGO and ThinLTO with `llvm` and `lld` installed, and no BOLT, because AlmaLinux
  ships none.
- **One processor's build** (`CPU=`) is never optimised this way.

**Raced in the plain `make`, 2026-09-23.** The optimised build was raced against the
installed plain BUILD 0021: same source, 5 alternating rounds, the load below 1.5, and
identical output.

| | optimised (median) | plain (median) | |
|---|---|---|---|
| the author's race, 1,000,000 turns | 2.226 s | 2.508 s | **11.2% faster** |
| everything.satl, 40,000 turns | 4.529 s | 5.218 s | **13.2% faster** |

Every optimised run beat every plain one: the slowest optimised runs were 2.236 and
4.560 s, and the fastest plain runs 2.497 and 5.195 s.

**What the fresh reader found in c5b37f0, all fixed:**

- **The serious one.** After any plain `make`, `make CPU=<x>` and `make cpus` were
  refused. The processor build described itself as "plain" against a stamp saying "PGO
  ThinLTO BOLT". Every build made from BUILD N now carries BUILD N's word.
- A wrapper `CXX` (`ccache clang++`) made the tool paths two words each. The tools now
  come from clang's own `InstalledDir`.
- `PGO`, `LTO` or `BOLT` exported in a shell turned steps on in a plain build.
- The training programs named the wrong fragment.
- The training build's refusal talked about processors.
- **A failed BOLT now keeps the PGO + ThinLTO satl and says why**, instead of stopping
  every `make`. This was forced in a fresh copy with a failing merge: exit 0, and the
  satl runs.

**The comma trap.** The first real build failed at BOLT: "instrumentation runtime
libraries require relocations". `$(if $(BOLT),-Wl,--emit-relocs)` had split at its
comma and passed a bare `-Wl`. Link flags are named in variables now.

#### What was planned

**These are not three flags.** One is a flag, one is a build procedure, and one is a
separate tool:

| | What it is | Turned on by |
|---|---|---|
| **LTO** | the linker optimises across every file at once | a flag: `-flto=thin`, on compile and link, with lld (this machine already links with lld) |
| **PGO** | the compiler lays out code by what a real run did | **two builds and a run between them**: build with `-fprofile-instr-generate`, run a training set of programs, merge the profiles with `llvm-profdata`, build again with `-fprofile-instr-use` |
| **BOLT** | rearranges the *finished* binary by a real run | **a separate tool, after the link**: link with `--emit-relocs`, record a run (`perf`, or BOLT's own instrumentation, since this machine has no `perf`), then `llvm-bolt` writes a new satl |

Three things specific to satl:

- **LTO cannot cross a `dlopen`.** Each of the 62 words is its own `.so`, so LTO
  optimises inside satl and inside each word, never between them.
- **PGO would starve the kernels this machine does not run.** Trained here, the ADX and
  512-bit kernels have no profile and are laid out as cold code. Either leave the kernel
  files out of PGO or train each kernel through `--bit`.
- **The training set is part of the build.** It is committed, it is never the race
  programs (or the race measures the training), and the profile is regenerated every
  build. A stale profile is quietly ignored.

`make` would carry it as `make pgo` or a `PGO=yes` knob, adding a few minutes per build.

**Measured 2026-09-23.** "10–30%" was the range other interpreters report, not a satl
number. On satl, PGO gave **10.5–13.0%** on the author's race and **7.7–9.6%** on
`everything.satl`, and all 20 PGO runs beat all 25 others. ThinLTO and BOLT on top could
not be told apart from PGO alone, except about 2–3% on the race.

The whole pipeline was built on this machine:

- **Recipe:** `-fprofile-instr-generate`, then 7 training programs, then
  `-fprofile-instr-use`, then `-flto=thin`, then `llvm-bolt -instrument`, then
  `llvm-bolt -reorder-blocks=ext-tsp -reorder-functions=cdsort -split-functions`.
- **Checked:** every build's output matched, and check.sh ran 784 of 785 against the
  BOLT build. The one failure is a terminal-width row that fails the same way on the
  plain build.
- **Cost:** a full build went from 19 s to about 80 s.

Three traps the build agent hit and solved:

1. The word libraries share a module signature, so their profiles overwrote each other.
   The fix is to link with `-Wl,--build-id` and name the files `%p-%b`.
2. The training set had no capsules or spacesuits, which `everything.satl` uses
   heavily. A committed training set should.
3. BOLT warns about 39 relocations it cannot analyse and leaves two GLib functions
   alone. It still runs correctly.
