# pcg-cpp — vendored, and cut down to four files

**This is not our code.** It is Melissa O'Neill's PCG C++ implementation, and the
only third-party dependency in this tree.

| | |
| --- | --- |
| upstream | https://github.com/imneme/pcg-cpp |
| version | **0.98** — the number the directory used to carry, kept here instead |
| licence | **Apache License 2.0**, in `LICENSE.txt`, which must travel with these files |
| brought in | 2026-08-27 |

## Why the version is written down rather than left in the directory name

The directory upstream ships is `pcg-cpp-0.98/` and the first satellite kept that
name, so its build said `-isystem pcg-cpp-0.98/include` and the version was visible
in every compile line. This tree shortened the directory to `pcg/`, which is easier
to read and **throws away the one fact that says which upstream this is** — so the
version is recorded here on purpose. It is not decoration: pcg-cpp relicensed after
0.98, so the version and the licence are the same fact asked two ways.

## What was removed, and what must not be

Upstream is 103 files. **Three headers and a licence are kept**; the samples, the
test suites, their expected output, the Makefiles and the `.gitignore` are gone —
none is reachable from a satellite build, and a vendored test suite is a second
build system nobody runs.

    include/pcg_random.hpp      the generators
    include/pcg_extras.hpp      included by pcg_random.hpp
    include/pcg_uint128.hpp     included by pcg_extras.hpp
    LICENSE.txt                 Apache 2.0 -- a redistribution requirement, not a courtesy

**The three headers must stay in one directory.** They include each other by bare
name — `pcg_random.hpp:102` includes `"pcg_extras.hpp"`, which at line 87 includes
`"pcg_uint128.hpp"` — so splitting them breaks the include and the failure is a
missing file, not a linker error.

## `-isystem`, never `-I`

The first satellite found this and wrote it down: `pcg_extras.hpp:223` warns under
`-Wall -Wextra`, which this build turns on for everything. `-isystem` suppresses
warnings from a header the project does not own and cannot fix without forking.
Its build spelled it `PCGFLAGS = -isystem pcg-cpp-0.98/include`; the equivalent
here is `-isystem pcg/include`.

**No flag is in this build yet, deliberately.** Nothing includes these headers —
`satellite.random` has no milestone, and PLAN M2's rule is that a thing gets a
consumer in the milestone that adds it. The flag arrives with the first file that
includes a PCG header, and not before.

## What it is for

`satellite.random` — DESIGN §11's three tiers. DESIGN §11.1 is the constraint worth
carrying over: **PCG makes no cryptographic claim and no tier is described as
secure.** Its state is recoverable from its output.

## The one type this project uses

`pcg32_k16384` — `pcg_random.hpp:1748`, `ext_setseq_xsh_rr_64_32<14,16,true>`.

The first satellite consumed this whole library through **one type, one seed
adaptor, four operations and ten call sites, in a single translation unit**
(`src/random_numbers/random.cpp`). Nothing else in that tree named a PCG entity in
code. The contract, in full: construction from a seed sequence, construction from
one `uint64_t`, an unbounded 32-bit draw, a bounded `[0, bound)` draw, and being
in-place constructible inside a `std::optional`. Not `min()`, `max()`, `discard()`,
`seed()`, stream selection, equality or streaming — no `std::uniform_int_distribution`
and no `std::shuffle` ever touched it, so **`UniformRandomBitGenerator` is not a
requirement.**

**The real seam is two lines wide.** `Bits32` — a virtual destructor and
`virtual unsigned next() = 0`, *"uniform over the whole 32-bit range."* The
arbitrary-precision sampler consumes only that, and v1's own test suite drove it
with a home-grown splitmix32 stub, which proves the seam is generator-agnostic.

**DESIGN §11's three tiers are not three generators.** They are this one generator
spun for different lengths — v1 picks a millisecond window per tier and folds draws
until the deadline. That is what makes §11's claim true: the tiers *"differ in
nothing a program can see except how long they take."*

Measured by v1 on this machine, 2026-08-24: `sizeof` 65552, the seed sequence
requests 16392 words (524,544 bits), entropy-seeded construction ~1.6 ms,
value-seeded ~0.026 ms.

## The licence problem, which is why this folder may not be permanent

**The top-level `LICENSE` says MIT (Expat) and says nothing about Apache-2.0.**
`README.md` and `LAYOUT.md` repeat the MIT claim flatly. Vendoring these three
headers makes all three statements incomplete — Apache-2.0 is permissive and
compatible, so this is a disclosure problem rather than a conflict, but it is a real
one and it is not fixed by this file.

**The first satellite tried to remove the dependency rather than disclose it.**
`old_versions/first_satellite/plans/pcg_k16384_spec.md`, written 2026-08-24, is a
derived specification for reimplementing `pcg32_k16384` in-tree — *"copyright covers
pcg-cpp expression, not the algorithm"* — so the MIT claim would become accurate.
Its own header records that **the agent meant to verify it bit-for-bit against the
real generator never finished, so the algorithm section is unverified.** Reusing that
spec means finishing that check first: 10,000 outputs, diffed.

So there are two honest routes and this folder commits to neither:

1. **Keep these headers** and amend `LICENSE`, `README.md` and `LAYOUT.md` to name
   the Apache-2.0 component. Cheap, immediate, and the tree stops being MIT-only.
2. **Reimplement from the spec** and delete this folder. Keeps the tree MIT, and
   costs the bit-exactness verification v1 never ran.

## A 512-bit variant was investigated on 2026-08-27 and is NOT a conversion

The ask was `pcg512_k16384`. It cannot be had by choosing a different typedef, and
the reason is worth writing down so it is not rediscovered.

**`k16384` exists only at 32-bit output** — `pcg32_k16384` and `pcg32_k16384_fast`,
and nothing else in the library carries that extension size. **The widest output
anywhere in pcg-cpp is 128 bits.**

**`uint_x4` looks like the way to widen and is not.** It is
`template <typename UInt, typename UIntX2>` with the full operator set, no
dependency on any builtin wide type, and it is what supplies `pcg128_t` on
compilers without `__int128` — so `uint_x4<uint64_t, pcg128_t>` for 256 and
`uint_x4<pcg128_t, u256>` for 512 is the obvious composition. **It produces wrong
answers.** `operator*` hard-codes the word width:

    pcg_uint128.hpp:531   r.w.v1 = UInt(a0b0 >> 32);
    pcg_uint128.hpp:534   r.w.v2 = UInt(a1b0 >> 32);
    pcg_uint128.hpp:543   r.w.v2 = addwithcarry(r.w.v2, UInt(a0b1 >> 32), ...);
    pcg_uint128.hpp:558   r.w.v3 = addwithcarry(r.w.v3, UInt(a1b1 >> 32), ...);

Nineteen bare `32`s in the header against three `sizeof(U)*CHAR_BIT` expressions.
With a 64-bit `UInt` those shifts are half what they must be, so the type
**compiles and multiplies incorrectly** — the worst failure available in a
generator, because nothing downstream can detect it. Two lesser breakages sit in
front of it and would have to be fixed first: the `Integral` constructor rejects
`__uint128_t`, and `flog2` is ambiguous. Neither is the real problem.

Building 512 therefore means, in order: forking this Apache-2.0 header and making
its arithmetic width-generic with every one of those literals audited; choosing
LCG constants at 512 or 1024 bits, which are not published and whose spectral
quality would be unproven; carrying **1 MiB of state per generator** for a
16384-entry table at 512 bits, against 64 KiB today; and seeding it, which at this
table size wants roughly 8 Mbit of kernel entropy per construction.

**And satellite does not need any of it.** The seam is `Bits32::next()` — 32 bits
at a time — and the arbitrary-precision sampler assembles a number of any width
from it, one base-10⁹ limb at a time, rejecting per limb. A 512-bit number is 16
calls; a 512-*digit* number is more calls. **Width is a property of the sampler,
not of the generator**, and the throwaway rule in DESIGN §11 — discard whole
numbers of the size being asked for — is expressible today, at any size, against
`pcg32_k16384` exactly as it stands.
