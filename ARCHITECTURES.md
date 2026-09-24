# ARCHITECTURES — the processors satellite supports, and their names

The author, 2026-09-23: *"Heres a list of all the architecutres that I want to build an
executable for"*, then *"those are the names that I want to use ... as we will be
supporting these with these names"*.

These nine names are the author's. The plan that uses them (one satl with its number
kernels inside, or one executable per name) is [CPU-PLAN.md](CPU-PLAN.md).

## The nine

| Name | Architecture (year) | clang's name | Chips it is in | Tested on |
|---|---|---|---|---|
| `satl.intel` | Intel Core (2006) | `core2` | Core 2 Duo E6600, T7200 | the author's 2006 computer |
| `satl.nehalem` | Nehalem (2008) | `nehalem` | Core i7-920, Xeon 5500 | none |
| `satl.sandy` | Sandy Bridge (2011) | `sandybridge` | Core i7-2600K | none |
| `satl.haswell` | Haswell (2013) | `haswell` | **Xeon E5-2670 v3 (this machine)**, Core i7-4770K | this machine |
| `satl.skylake` | Skylake (2015) | `skylake` | Core i7-6700K | none |
| `satl.palm` | Palm Cove (2018) | `cannonlake` | Core i3-8121U, about the only one sold | none |
| `satl.willow` | Willow Cove (2020) | `tigerlake` | Core i7-1165G7 (11th-gen laptops) | none |
| `satl.raptor` | Raptor Lake (2022) | `raptorlake` | **Core i7-13700K (the author's)** | the 13700K |
| `satl.lion` | Lion Cove (2024) | `arrowlake`, `lunarlake` | Core Ultra 9 285K, Core Ultra 7 258V | none |

**`satl.intel` means the Core *microarchitecture*: Core 2 Duo, Core 2 Quad, Xeon 5100.**
Intel's first chips with "Core" in the name, the Core Duo and Core Solo ("Yonah",
January 2006), were 32-bit only. They cannot run satl, and clang has no 64-bit name for
them (`-march=yonah` is refused). Check which one the 2006 computer has before testing:
`grep -c lm /proc/cpuinfo` answers 0 on a Yonah.

## What each can run that matters to satellite

What each `-march` makes clang define, **asked of clang 24 on 2026-09-23** (`clang++
-march=<name> -dM -E`), not recalled:

| Name | SSE4.2 | AVX (256-bit float) | AVX2 (256-bit integer) | BMI2 (MULX) | ADX | AVX-512 | AVX-512 IFMA | AVX-IFMA (256-bit) |
|---|---|---|---|---|---|---|---|---|
| `intel` | – | – | – | – | – | – | – | – |
| `nehalem` | yes | – | – | – | – | – | – | – |
| `sandy` | yes | yes | – | – | – | – | – | – |
| `haswell` | yes | yes | yes | yes | – | – | – | – |
| `skylake` | yes | yes | yes | yes | yes | – | – | – |
| `palm` | yes | yes | yes | yes | yes | **yes** | **yes** | – |
| `willow` | yes | yes | yes | yes | yes | **yes** | **yes** | – |
| `raptor` | yes | yes | yes | yes | yes | – | – | – |
| `lion` | yes | yes | yes | yes | yes | – | – | yes |

Four things in that table decide what satellite can do:

1. **Raptor Lake has no AVX-512, and neither does Lion Cove.** Intel switched it off on
   every Alder Lake and Raptor Lake chip, and left it out of Lion Cove. The 13700K is a
   256-bit processor. Only Palm Cove and Willow Cove, from this list, have 512-bit
   registers.
2. **Sandy Bridge's 256 bits are for floating point only.** Whole numbers wider than 128
   bits start with AVX2 on Haswell.
3. **ADX arrived with Broadwell (2014)**, between Haswell and Skylake. It is the one
   thing the 13700K has for numbers that this Xeon lacks. `/proc/cpuinfo` here shows
   `avx2` and `bmi2` on all 24 threads, and no `adx` or `avx512f`.
4. **Lion Cove has AVX-IFMA**: the 52-bit multiply-add that big-number code wants, in
   256-bit registers.

## What is not in the list

- **AMD.** Zen 4 and Zen 5 (Ryzen 7000 and 9000) have AVX-512 *with* IFMA. satl chooses
  by what a processor can run, never by its name (003's rule, M37's chooser). So an AMD
  machine gets whatever its instructions allow. A Zen 4 desktop, or a used Tiger Lake
  laptop, is the cheapest way to own a machine that runs a 512-bit kernel.
- **Broadwell, Ice Lake, Rocket Lake, Alder Lake and the server chips.** Each falls into
  a tier of a named neighbour by what it can run: Broadwell with Skylake, Ice Lake with
  Willow Cove, Alder Lake with Raptor Lake.

## How these relate to M37's 53 builds

M37 (`0d875d1`) builds satl for every processor clang names, into
`build/cpu/<clang name>/`, and `build/satl-cpu-level` picks one. These nine are the
author's names for nine of them; the second column of the first table is the mapping.
[CPU-PLAN.md](CPU-PLAN.md) proposes replacing the 53 builds and the separate chooser with
one satl that picks its own number kernel. Whether the nine names stay as separate
executables is a decision the plan leaves open.
