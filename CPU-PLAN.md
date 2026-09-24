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
How much of a turn that multiply is decides what a faster kernel can give. **Being
measured (2026-09-23); the numbers go here.**

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

**Probably not worth kernels now; being measured.** Long copies and joins already go
through glibc's `memcpy`, which runs its AVX2 version in the ordinary build. Two things
are not yet known: whether comparing two long 16-bit strings reaches `memcmp` or a plain
loop, and what share of a real program strings take. Both are being measured on
2026-09-23; the numbers go here.

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

### CPU-4 — the 256-bit kernel (2013)

MULX (BMI2), tested on this Xeon. Clang may already emit MULX from today's loop under
`-march=haswell`; the census being run 2026-09-23 says whether, and a hand-written MULX
loop is timed against it.

### CPU-5 — the 512-bit kernel (2018)

AVX-512 IFMA, the only kernel with no machine here. It is tested under Intel SDE, or on a
machine bought for it. Its speed can only be known on real hardware. Ask whether it is
worth building before there is a machine to race it on.

### CPU-6 — names looked up ahead of time

The biggest gain on every processor, independent of kernels. On 2026-09-16 each variable
access rebuilt a name from its 16-bit codes and hashed it (to be re-confirmed against
today's walker). `program_check.cpp` already sees every name in a capsule before the
walker runs, so it can give each one a slot number. The walker then reads slot *n* of an
array, which is what CPython does. The gap measured that day, `i = i + 1` at 458 ns
against CPython's 87 ns, is mostly this. Names that appear only at run time (arguments
rows, `interpret`, spacesuit fields reached by name) keep a slow path.

### CPU-7 — PGO, LTO and BOLT

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

**The gain is not known yet.** "10–30%" was the range other interpreters report, not a
satl number. The build is being raced on 2026-09-23 (ordinary, haswell, `-O3`, PGO, PGO +
LTO, PGO + LTO + BOLT), and the measured number goes here.
