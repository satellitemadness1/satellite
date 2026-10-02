# RANDOM — satellite.random.fast, normal and ultra, and the 512-bit generator

## 2026-10-02: HIS BRIEF, HIS RULING ON THE GRADES, AND WHAT IS BUILT

**Read this first.** The choices he may still overrule are under *My choices*, each with what
changes if he rules the other way; the questions that are his are under *Open*.

### His words (2026-10-02), in order

> *"this is the SATELLITE programming language, and we need to add satellite.random.fast() and
> satellite.random.normal and satellite.random.ultra()"*

> *"First we need to take the pcg that is in /vendor and use it to build a random number
> generator... the generator in /vendor has a 16386k variant that I wanted to use, it's 32_16386
> but I wanted to make it 512_16386 so it's a 512-bit x 16386 generator, can you do that?"*

Then, when the first build had made the grades three different generators:

> *"I never told you that I wanted for .fast() .normal() and .ultra() for ultra draw a random
> number between 2000 - 3000 and throw away random numbers of the size requested for that many
> ms, for normal, 1000 - 2000 and for fast do 500 - 1000ms"*

That is the design of his 001 (DESIGN §18) and 003 (`src/satellite_random/random.hpp`: "three
tiers that differ in NOTHING a program can see except how long they take"), with the windows
he set today. The first build is history (this file's first version, in git); what follows is
what stands. And on the shapes:

> *"I also never told you that I wanted it to be satellite.random.fast(digits) and
> satellite.random.fast(min, max) if the user enters two numbers, then it's a min and a max,
> and if they enter 1 number it's the number of digits and to accept this syntax:
> satellite.variable.number my_random_number = satellite.random.fast(digits or min, max)"*

So the COUNT of numbers decides, never a word: `fast(20)` is twenty digits, `fast(1, 6)` is a
min and a max, `fast(0, 100, 5)` is 003's step shape, and any of them may be a variable. The
word table's rows `fast(digits)` / `fast(min, max)` are how 003 numbered the shapes (a shape is
keyed by its word and its arity); the lexer picks the row by the call's own commas, so a
program never writes those names. The refusal sentences and the help page say the rule in his
words.

### What is built

**The generator he asked for, once, behind all three words.** `pcg32_k16384` is pcg-cpp's
`extended<14, 16, setseq_xsh_rr_64_32, oneseq_rxs_m_xs_32_32, true>`: a 64-bit LCG, 32 bits
out through XSH-RR, XORed with one of 2^14 = 16384 extension words, each word a 32-bit
generator of its own that steps every 2^16 outputs. `pcg512_k16384`
(`satellite/satellite_random/pcg_512.hpp`) is the same template with 512 written where 32
was: a 1024-bit LCG, 512 bits out through XSL-RR (the permutation pcg uses once a state spans
registers, as pcg64's does), XORed with one of 16384 512-bit words. **Nothing under `vendor/`
changed** (`vendor/edit_journal/pcg-cpp/EDITS.md` still says "No edits", and check.sh reads
that line). Three things upstream never wrote are supplied beside the template:

| supplied | where | why |
|---|---|---|
| `PCG_BITCOUNT_T = unsigned int` | `pcg_512.hpp`, before the include | pcg counts bits in a `uint8_t`; 512 does not fit one. Upstream names the knob for exactly this. check.sh proves pcg32 and pcg32_k16384 still answer upstream's own expected output |
| `wide_unsigned<8>` and `<16>` | `wide_unsigned.hpp` | a 512- and a 1024-bit unsigned integer with the operators the templates write, and three rules that make 2014 templates compile over it (its header) |
| five constants | `random_constants.hpp`, by `make_random_constants.py` | pcg looks up LCG multipliers by type; none exist past 128 bits. These are π's binary digits, 3584 of them, low bits set for full period. **No spectral test** — the header says so |

The table is **16384, not 16386**: pcg indexes it with the state's low 14 bits, a power of two
by construction. State: 128 bytes of LCG + 128 of stream + 1 MiB of table = 1,048,832 bytes.
Period, as pcg's own `period_pow2()` computes it: 2^(1024 + 16384·512) = 2^8,389,632. Seeded
from the kernel through getrandom(2), the WHOLE state — 1,049,088 bytes read — one generator
a thread, made the first time that thread draws (`random_source.hpp`).

**The grades are his windows** (`random_spin.hpp`). A call, in order: every check; then it
draws HOW LONG, uniform inside its window, both ends in, from the same stream the answer will
come from; then it throws away draws of exactly the shape it will answer — the same count,
base and step — until that long has passed, **at least once** (a do/while: a window is a
minimum, not a budget); then it draws the answer.

| grade | window |
|---|---|
| `fast` | 500 to 1000 ms |
| `normal` | 1000 to 2000 ms |
| `ultra` | 2000 to 3000 ms |

**The words are 003's, whole**: `1 7 1` to `1 7 16` in `words/words.tsv`, the author's table of
2026-09-04. Nine shapes draw, all exact whole numbers at any size:

| shape | answers | refused, before any spin |
|---|---|---|
| `fast(20)` | uniform over 0 .. 10^20 − 1 ("twenty digits") | a count below 0, or not a whole number (S431) |
| `fast(1, 6)` | uniform over 1 .. 6, both ends in | min above max (S432); not whole numbers (S431) |
| `fast(0, 100, 5)` | one of 0, 5, 10 … 100 | a step below 1 (S433); a step that misses max, naming the last value reached (S434) |
| `fast()` | **nothing** — refused before anything runs (S430) | 003's ruling, 2026-09-04: "not a question with an answer" |
| `satellite.random(...)` | refused: "pick a grade" (13) | |
| `seeded(...)` | not built (14) | his to shape — see *Open* |

and the same for `normal` and `ultra`. The sampler (`random_draw.cpp`) draws exactly as many
bits as the largest value it may be and draws again past the bound — rejection, never a
remainder, nothing rejected at all when the bound is a power of two. The shape (10^digits, a
span, a count of places) is worked out ONCE a call, so the spin never recomputes a power of
ten per throwaway. **No ceiling on the size**: 001 and 003 refused more than 100,000 digits;
004's rule is no limits of the language's own, so `ultra(1000000)` builds 10^1000000 and draws
under it.

### Files

| | |
|---|---|
| `satellite/satellite_random/wide_unsigned.hpp` | the fixed-width integer |
| `satellite/satellite_random/pcg_512.hpp` | the 512-bit instantiation, the constants' specialisations, the doubling `unxorshift` |
| `satellite/satellite_random/random_constants.hpp`, `make_random_constants.py` | π's digits; `--check` is a check.sh row |
| `satellite/satellite_random/random_source.hpp/.cpp` | the seam (64 bits a call), the generator, the kernel seed, one a thread |
| `satellite/satellite_random/random_draw.hpp/.cpp` | the uniform draw at any size, and the three shapes as one |
| `satellite/satellite_random/random_spin.hpp/.cpp` | his windows, and the throwaway |
| `satellite/satellite_random/random_cases.cpp` | the harness, `build/random_cases` |
| `satellite/bytecode/random_calls.hpp/.cpp` | the words: the checker's refusals, the shape, the spin, the call |
| `satellite/machine/machine_codes.hpp` 71–75, `s_codes.hpp` S430–S434 | the refusals |
| `satellite/satellite_variable_number/satellite_number.hpp` | `from_limbs`, the one addition to the number |
| `make_support/030, 040, 050, 060, 065` | `RANDOM_DIR`, `PCG_INCLUDE` (−isystem, the generator's folder only), the harness |
| `tests/random*.satl`, `utility/check_random_output.py`, check.sh's RANDOM rows | the proofs, one of them timing a spin through satl |
| `satellite.help/satellite.random/help_text.txt` | the page; its examples run under check.sh |
| `licenses/pcg-cpp/license.txt`, `licenses/README.md`, `THIRD-PARTY-NOTICES.txt` | Apache-2.0, carried in every build now |

### Measured (this machine, `build/random_cases | grep note`; he measures himself)

The first probe of `pcg512_k16384` cost **595 ns a 512-bit output**, nearly all of it the table:
pcg's `unxorshift` undoes `x ^= x >> s` by recursion, one level per `s` bits of width, which at
512 bits and a shift of 6 is 85 levels of 512-bit arithmetic for each of 16384 cells every
65536 draws. A doubling form (`x ^= x >> s, x ^= x >> 2s, …`, seven steps), supplied for
`wide_unsigned` and found by argument-dependent lookup from inside pcg_detail, brought it to
**about 315 ns**, 43 ns a limb through the seam. The harness prints the figures of the day,
and how many draws each grade's spin threw away.

### My choices, each reversible

1. **The bare shape refuses** (S430) — his 2026-09-04 ruling in 003, kept. If he would rather
   `ultra()` answer one raw output of the generator (a 512-bit number, 155 digits), it is one
   case in `random_word_refused` and one in `call_random_word`.
2. **The step must land on max** (S434) — his 2026-09-04 rule, kept, with the last value the
   step reaches named. The draw itself never passes max either way.
3. **No 100,000-digit ceiling** (001's and 003's `MAX_RANDOM_DIGITS`), under 004's no-limits
   rule. One comparison in `call_random_word` brings it back.
4. **How long a spin takes is drawn from the same stream the answer comes from**, as 003 did
   ("costs one word and needs no second generator"), by the same unbiased draw as everything
   else.
5. **A throwaway is a whole draw of the answer's shape** — count, base and step — which is
   what "random numbers of the size requested" reads as; the shape itself (10^digits) is worked
   out once a call, not once a throwaway.
6. **The 512-bit outputs are handed out as eight limbs**, least significant first, before
   drawing again: the generator's own stream, a limb at a time. Consuming a whole output per
   limb is one line.
7. **π's digits for the constants**, with the low bits set by the script, and no spectral test
   claimed: a 1024-bit multiplier nobody has tested is a true statement about every 1024-bit
   multiplier there is. What the generator rests on is the full-period theorem, XSL-RR, and the
   table — the same three things pcg32_k16384 rests on — and the harness's battery.
8. **One generator a thread, lazily**, never a shared locked one: a program's threads each
   seed their own; a thread that never draws never holds a table.
9. **The S-codes are S430–S434**, in the band with division by zero, as "arithmetic that has no
   answer". 003 numbered them S09xx, which is 004's "satl itself is in trouble" band.
10. **A binary or a hex given to a draw is refused**, with the hint that its `.number` is the
    whole number it holds, rather than taken as a number quietly.

### Open — his

- **seeded** (`1 7 13`–`1 7 16`): 003's help lines gave it a shape — `seeded(seed)` sets the
  stream and answers nothing, `seeded(min, max)` may answer a FRACTION, `seeded(min, max, step)`
  takes a fractional step, and it does not spin — which 004's number model does not fit as it
  stands (a satellite number is whole; a fraction is its own type). The generator side exists
  (`seeded_random_source`, the harness drives it); the words wait for his shape.
- Whether a grade should ever be able to carry the word "secure". None does, and the help page
  says so; getrandom(2) straight through is the only route there, as 001's DESIGN §18 recorded.
- Ctrl-C during a spin: the stop flag is read between statements, so a Ctrl-C lands after the
  call's window ends (at most three seconds), as it does for any long statement.
- What `satellite.random.fast` written **without** brackets should say: today the lexer reads
  `satellite.random` and then `.fast` as a name nobody declared (S201).

---

## The seam, for the next reader

`LimbSource::next_limb()` is the whole contract: 64 bits, uniform, one stream. Everything above
it (`random_draw`, `random_spin`, `random_calls`) knows nothing of PCG; everything below it
(`random_source.cpp`, `pcg_512.hpp`) is the only place the Apache-2.0 headers are seen — 003's
rule, kept, so the dependency has one file to be replaced in. check.sh's fingerprint row
never asks for the vendored headers because they are a system include; `040-sources.mk` names
them as build inputs instead, so the next pcg-cpp release is a new build number.
