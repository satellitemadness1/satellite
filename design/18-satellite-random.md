*satellite design docs, §18 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§17](17-the-abstract-machine.md), On: [§19](19-nine-additions.md).*

---

## 18. `satellite.random`

Status: **built**, 2026-08-19. `random.hpp`, `random.cpp`, `src/satellite_number/random.cpp`, six rows in
`format.def`, and `random_test`. The plan it was built from is `plans/random.md`, and every
number in this section was measured on the Xeon E5-2670 v3 of §17 rather than reasoned about.

Three tiers, which differ in nothing a program can see except how long they take:

| tier | throwaway window |
|---|---|
| `satellite.random.fast` | 50–100 ms |
| `satellite.random.normal` | 250–300 ms |
| `satellite.random.ultra` | 2000–3000 ms |

Two shapes on each:

```satellite
satellite.variable.number n = satellite.random.ultra(40)
satellite.variable.number m = satellite.random.ultra.range(1, 100)
```

`ultra(40)` is **uniform over [0, 10^40)** — zero through forty nines. `.range(low, high)` is
**inclusive at both ends**, which is what the `+ 1` in `high - low + 1` buys and what makes
`.range(1, 100)` able to answer 100.

**About one draw in ten of a 40-digit request prints 39 digits or fewer**, because a leading
zero is not printed and a uniform draw has one a tenth of the time. That is what uniform
means; a draw that always printed 40 digits would not be one. It is said out loud here, and
in `random.hpp`, and checked in `random_test`, because it is *not* what the name promises and
someone will otherwise file it as a bug.

### The tiers are a statistical character, not a security property

PCG makes no cryptographic claim and its state is recoverable from its output. **No tier here
is secure and none of them is described that way** — not in the header, not in
`satellite.help`, not in this section. A longer spin buys a wider spread of possible seeds
and nothing else.

That is a deliberate refusal rather than an omission. The word "secure" in a language's
documentation is load-bearing: someone will key something on it. If satellite ever wants a
tier that can carry the word, the mechanism is `getrandom(2)` — one syscall, no dependency,
256 bits of kernel entropy in about a microsecond, and `ldd satl` stays at six. That is
recorded as the available answer, not taken.

### The mechanism, and what each step is worth

Per call, in order:

1. Seed a `pcg32_k16384` from `pcg_extras::seed_seq_from<std::random_device>`.
2. Draw a spin length from the tier's millisecond range, and a watchdog cap.
3. Throw draws away until the deadline, folding each into an accumulator.
4. Seed a **second** `pcg32_k16384` from that accumulator.
5. Draw the answer from the second generator.

The fold is `accumulator = (accumulator + draw) / 2` — add two, divide, each step. Six runs
of a 2000 ms spin (~255,000,000 draws each), comparing three ways of folding the discarded
values, measured as the spread of the resulting seed across runs, which is what an attacker
searches:

| fold | spread |
|---|---|
| `sum / n` — the full mean | ~17.7 bits |
| `acc = (acc + x) / 2` — pairwise, **what is built** | ~30.4 bits |
| `sum` — no division at all | ~48.5 bits |

**The full mean collapses**, and it collapses harder the longer the spin runs. The Law of
Large Numbers concentrates it on 2^31 with spread shrinking as 1/√N; the measured figure
matched the predicted `(2^32/√12)/√N` to under a percent. More work makes it worse.

**The pairwise fold does not collapse**, because it is an exponential moving average and not
a mean: the newest draw contributes 1/2, the one before 1/4, and in integer arithmetic
anything about 32 steps back has been shifted out entirely. It keeps roughly one draw's worth
of entropy however long the spin runs — which is why the spin length is a *character* of the
tier rather than a quantity of entropy.

Two things are on the record and not acted on, in that order of value:

- **Answering straight from the first generator is stronger than any fold.**
  `seed_seq_from<std::random_device>` fills all 16384 extension words from kernel entropy —
  on the order of half a million bits — and steps 3 and 4 funnel every one of them through a
  32-bit accumulator. Deleting steps 3–5 keeps the tiers' timing behaviour and starts the
  answer from all of that entropy instead of ~30 bits of it.
- **Dropping the divide is a one-line change worth ~18 bits** (`acc += x`, `uint64_t`, no
  divide).

Neither is taken, because the mechanism above is the specified one and the divide is part of
the specification. They are written down here so that the choice stays a choice.

`uint64_t` for the accumulator and not `uint32_t`: the sum of two 32-bit values overflows one,
and the divide brings it back under 2^32 afterwards, so the intermediate is the only place
the width matters.

### The watchdog is a failsafe, and the clock choice is why it is only that

The spin also stops if the count reaches a random value in [500,000,000, 600,000,000]. Its
purpose is that **if the clock moved under a running spin, the deadline might never arrive**,
and a language builtin that can hang is not one.

It never fires in ordinary operation, which is the intended behaviour for a watchdog rather
than a defect:

```
fast    cap=568142855   drew=8609000     cap_fired=no
normal  cap=569337947   drew=31058000    cap_fired=no
ultra   cap=571482761   drew=249047000   cap_fired=no
```

Throughput is ~153,600 draws/ms bare and ~104,000/ms when each draw is also stored. Reaching
500M takes 3.3–4.8 s; ultra's ceiling is 3000 ms ≈ 310M draws. The cap therefore sits about
1.6–1.9× above ultra's worst case, which is a sane margin.

**The hang it guards against cannot happen with `steady_clock`**, which is the clock the
implementation uses. That clock is monotonic by the standard (`is_steady` is true, verified
here) and is `CLOCK_MONOTONIC` under libstdc++, so NTP cannot step it backwards; NTP can only
slew its rate by a few hundred ppm, which moves a 2500 ms deadline by about a millisecond.
Linux does not advance `CLOCK_MONOTONIC` across suspend either, so a machine suspended
mid-spin resumes and finishes rather than hanging. The failure mode is real for `system_clock`
and for `high_resolution_clock` — which is *precisely why* the clock is specified — and is
prevented by construction once that choice is made.

So the cap is belt and braces on top of a clock that already cannot hang. It costs one
comparison per 1000 draws and it bounds worst-case runtime whatever the clock does, which is
cheap enough to keep.

One refinement is available and **not taken**: a single global threshold is loose for the
short tiers. If the clock did freeze during a `fast` call, 500M draws is 3.3–4.8 seconds
before the watchdog trips — survivable, but a long time for a tier whose whole promise is
50–100 ms. Scaling the cap per tier (~50M for fast, ~150M for normal, the specified figure for
ultra) keeps the same protection proportionate. The single cap is correct and safe as written.

Worth knowing either way: the cap is drawn from the same generator, so it is a deterministic
function of the seed and contributes no unpredictability of its own. Its value is as a bound.

### Arbitrary precision: where the draw actually happens

A 32-bit generator's own bounded draw caps at a 2^32 span. `ultra(40)` needs 10^40 — about
133 bits — and a `.range` between two numbers a program wrote down can need any number at all.
So the draw happens at the `Number` level, in `src/satellite_number/random.cpp`, which knows nothing about
PCG and asks a `Bits32` for 32 bits at a time.

The plan specified the textbook binary form: take `bit_length(bound)` bits, reject if the
result is not below the bound, which rejects at most half the time because 2^k < 2·bound.
**That algorithm assumes a binary bignum, and §8.1 deliberately made this one base 10^9** —
decimal in, decimal out, and scaling by 10^k as a limb shift. A bit length is not something
that representation knows, and computing one costs more than the sampling does.

What is built instead is limb-aligned, and it has the *same* guarantee for the same reason:

```
count = number of limbs in bound          (base 10^9, top limb non-zero after trim)
top   = the top limb
loop:
    every limb but the top  <- uniform over [0, 10^9)
    the top limb            <- uniform over [0, top]
    if the result < bound: return it
```

The values this produces are exactly `[0, (top+1) · BASE^(count-1))`, each once, so the draw
is uniform on that interval, and `bound` lies inside it. Rejection happens with probability
`1 − bound/((top+1)·BASE^(count-1))`, which is at most `1/(top+1)` and so at most half —
because `trim()` guarantees `top ≥ 1`. Same bound as the bit version, no bit length needed.

Each limb comes from a rejection draw and never from `%` alone. Modulo skews toward the low
residues whenever the bound does not divide 2^32: measured over three buckets, `%` gave
1398410 / 904307 / 697283 against 998574 / 1001152 / 1000274 for the unbiased path. That is a
2:1 skew, and it is invisible to any test that only checks the range.

**`Number::MAX_RANDOM_DIGITS` is 100000**, and it is a refusal rather than a clamp: a program
that asks for a million digits has made a mistake, and silently handing back a hundred
thousand would hide it. The check is on the *count* and happens before anything is built, so
`1e2000000000` — twelve characters of source, two billion digits of value — is refused rather
than attempted. The ceiling is on the draw, so the widest legal bound is 10^100000, which is
100001 digits wide and every value under it is at most 100000.

The generator is **lazy**: it spins the first time it is asked for a bit. A bound the sampler
refuses is refused before any bit is drawn, so a call that is going to fail does not spend
three seconds first — measured at 0.196 s for an over-wide `.range` against the 2–3 s the
spin would have cost.

### Why `.range` is a fourth segment and not an overload

When these rows were written, `format.def`'s `SAT_PATH` recorded **one arity per path**. A
single `satellite.random.ultra` taking either a digit count or a low-high pair was one path
with two arities, which the format could not encode — the same gap `satellite.help` was stuck
in. Spelling the second form as `satellite.random.ultra.range` made it a *different path*,
which the format already handled, at a cost of one word in the registry.

That constraint is **gone**: §17's `SAT_VARIADIC` closed the gap for `satellite.help` and
would carry an overloaded `ultra` just as well. The surface is kept anyway, and the reason is
no longer the format. `.range(low, high)` names the second question at the call site instead
of leaving a reader to work out which of two meanings is in play by counting arguments —
`ultra(1, 100)` and `ultra(40)` differ only in how many things are inside the parentheses, and
they mean entirely unrelated things. One word buys a name for that difference.

Five words (`random`, `fast`, `normal`, `ultra`, `range`, ids 80–84) buy six paths, and they
are the **first four-segment paths the language has** — the first rows that fill the fourth
word of the name code §17 has carried since it was written.

`range` is deliberately not a `SAT_SELECTOR`. There is no receiver it reads relative to:
`satellite.random.ultra` is not a value, and `.range` is the tail of a language path rather
than a method on one.

### Implementation notes worth not rediscovering

- **`steady_clock`, never `high_resolution_clock`.** Measured here: `high_resolution_clock` is
  the same type as `system_clock`, `is_steady` is false, and both tick at 1 ns with a 21 ns
  measured granularity. There is no precision to trade away, and the wrong choice is
  unreproducibly wrong.
- **Batch the deadline check.** One `now()` costs more than one draw, so checking every
  iteration measures the clock instead of the generator. 1000 draws per check, which also sets
  the watchdog's resolution.
- **Fold as you go.** Storing every throwaway to fold it afterwards costs ~1.0 GB at ultra
  length and produces a result identical to a running accumulator.
- **`-isystem`, not `-I`.** `pcg_extras.hpp:223` raises `-Wunused-but-set-parameter` under
  this project's `-Wall -Wextra`, and §9's silent-from-scratch-rebuild property depends on
  suppressing it. Verified after the fact: a from-scratch build is silent, and `ldd satl`
  still lists six shared objects, because pcg-cpp is header-only.
- **`pcg-cpp`'s `discard()` is wrong for extended generators.** Measured: `pcg32_k16384`
  matches *n* individual draws at n=10 and n=1000 and diverges at n=100,000 and above;
  `pcg32` matches at every n tested. Nothing here uses `discard()`, and nothing should on a
  k-variant.
- **One object sees the PCG headers.** `random.o` is compiled with `$(PCGFLAGS)` and nothing
  else is, for the same reason `window.o` is the only object that sees gtk (§9).

### What is not decided

1. Whether the watchdog cap scales per tier or stays one global threshold. Safe either way.
2. Whether a tier ever moves to `getrandom(2)`, which is the only route to a tier that could
   carry the word "secure".
3. Whether the fold stays `(acc + x) / 2`, becomes a plain sum (+18 bits), or is deleted
   along with the reseed (strictly stronger than any fold).
4. Whether `satellite.random.fast(0)` should stay legal. It draws from a one-member interval
   and answers 0, which is the mathematically right answer and one fewer special case; it is
   also almost certainly a mistake wherever it appears in real source.
