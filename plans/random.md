satellite.random — the plan
===========================

STATUS: **built**, 2026-08-19. `random.hpp`, `random.cpp`, `bignum/random.cpp`,
five words and six paths in `format.def`, `random_test`, and DESIGN.md §18 —
which is now where this lives, because a plan describes what was going to happen
and §18 describes what does. Read that first; this file is kept for the
measurement session it records.

Two things landed differently from what is written below, both recorded in §18:

* **The arbitrary-precision sampler is limb-aligned, not bit-aligned.** The
  bit_length form under IMPLEMENTATION NOTES assumes a binary bignum, and §8.1
  deliberately made satellite's base 10^9 — so a bit length is not something the
  representation knows and computing one costs more than the sampling. Drawing
  the top limb over `[0, top]` and the rest over the whole base gives the same
  uniformity and the same "never rejects more than half the time", because
  `trim()` guarantees `top >= 1`.
* **A ceiling was needed and none was planned.** `Number::MAX_RANDOM_DIGITS` is
  100000, and it is a refusal rather than a clamp. Without it
  `.range(0, 1e2000000000)` is a two-billion-digit allocation attempt from
  twelve characters of source.

Everything else is as specified, including the two open recommendations that
were deliberately NOT taken: the fold is still `(acc + x) / 2` and the reseed is
still there.

Written 2026-08-19, after a measurement session against pcg-cpp-0.98 in the
tree. Every number below was measured on this machine (AlmaLinux 10.2, the
Xeon DESIGN §17 quotes) rather than reasoned about, and the measurement is
named wherever a decision rests on it. Where something was not verified, it
says so.

DESIGN.md does not yet have a §18 for this. Writing one is part of the work.
(It does now — see STATUS above. The rest of this file is left as written.)


WHAT LANDS
----------

`satellite.random`, in three tiers that differ in how long they run:

| tier | throwaway window |
|---|---|
| `satellite.random.fast` | 50–100 ms |
| `satellite.random.normal` | 250–300 ms |
| `satellite.random.ultra` | 2000–3000 ms |

Two forms on each tier:

    satellite.variable.number n = satellite.random.ultra(40)
    satellite.variable.number m = satellite.random.ultra.range(1, 100)

`ultra(40)` is **uniform over [0, 10^40)** — 0 through forty nines, leading
zeros allowed. Confirmed with the author 2026-08-19. One consequence to write
into the docs: about 10% of "40-digit" draws render as 39 digits or fewer,
because a leading zero is not printed. That is correct for a uniform draw and
is not what the name promises, so it needs saying out loud.

`.range(min, max)` is inclusive at both ends.


THE MECHANISM
-------------

Per call, in order:

1. Seed `pcg32_k16384` from `pcg_extras::seed_seq_from<std::random_device>`.
2. Draw a random spin length in the tier's millisecond range.
3. Throw draws away until the deadline, folding each one into an accumulator.
4. Seed a second `pcg32_k16384` from the accumulator.
5. Draw the answer from the second generator.

Step 3's fold is `acc = (acc + x) / 2` — add two, divide, each step. This is
the author's decision and it is recorded here with what it measures, not
argued with.


WHAT THE FOLD COSTS, MEASURED
-----------------------------

Six runs of a 2000 ms throwaway (~255,000,000 draws each), comparing three
ways of folding the discarded values into a seed. The figure is the spread of
the resulting seed across runs, which is what an attacker searches:

| fold | spread |
|---|---|
| `sum / n` — the full mean | **~17.7 bits** |
| `acc = (acc + x) / 2` — pairwise, **chosen** | **~30.4 bits** |
| `sum` — no division at all | **~48.5 bits** |

The full mean collapses because the Law of Large Numbers concentrates it: the
mean of N uniform draws converges on 2^31 with spread shrinking as 1/sqrt(N),
and the measured spread matched the predicted `(2^32/sqrt(12))/sqrt(N)` to
under a percent. More draws make it worse, not better.

The pairwise fold does **not** collapse, because it is an exponential moving
average rather than a mean: the newest draw contributes 1/2, the one before
1/4, and in integer arithmetic the values from ~32 steps back are shifted out
entirely. It therefore keeps roughly one draw's worth of entropy regardless of
how long the spin runs.

**Switching the fold to a plain sum is a one-line change worth ~18 more bits**
(`acc += x`, no divide, `uint64_t`). It is not taken here because the divide is
specified, but it is the cheapest available improvement if that changes.

For context on the whole pipeline: `seed_seq_from<std::random_device>` in step
1 fills all 16384 extension words from kernel entropy — on the order of half a
million bits. Steps 3–4 funnel that through the accumulator, so the second
generator starts from ~30 bits however good step 1 was. **Drawing the answer
straight from the first generator and deleting steps 3–5 is strictly stronger
than any fold**, and is the recommendation on the record. The tiers can keep
their timing behaviour without the reseed.


THE COUNT CAP IS A WATCHDOG, AND THE CLOCK CHOICE IS WHY IT MATTERS
-------------------------------------------------------------------

The cap stops the throwaway if the count reaches a random value in
[500,000,000, 600,000,000]. Its purpose is a failsafe: **if the clock moves
under a running spin, the deadline may never arrive and the loop never ends.**
That is a real failure mode and the cap is the correct shape of defence
against it.

It never fires in normal operation, and that is the intended behaviour for a
watchdog rather than a defect:

    fast    cap=568142855   drew=8609000     cap_fired=no
    normal  cap=569337947   drew=31058000    cap_fired=no
    ultra   cap=571482761   drew=249047000   cap_fired=no

Throughput is ~153,600 draws/ms bare, ~104,000/ms when each draw is also
stored. Reaching 500M takes ~3.3-4.8 s; ultra's ceiling is 3000 ms = ~310M. So
the cap sits about 1.6-1.9x above ultra's worst case, which is a sane watchdog
margin.

**The hang it guards against cannot happen with `steady_clock`.** That clock is
monotonic by the standard (`is_steady` is true, verified on this machine) and
is `CLOCK_MONOTONIC` under libstdc++, so NTP cannot step it backwards; NTP can
only slew its rate by a few hundred ppm, which moves a 2500 ms deadline by
about a millisecond. Linux also does not advance `CLOCK_MONOTONIC` across
suspend, so a machine suspended mid-spin resumes and finishes rather than
hanging. The failure mode is real for `system_clock` and
`high_resolution_clock` -- which is precisely why the plan specifies
`steady_clock` -- and is prevented by construction once that choice is made.

So the cap is belt-and-braces on top of a clock that already cannot hang. That
is cheap and defensible: it costs one comparison per 1000 draws and it bounds
worst-case runtime no matter what the clock does.

The one thing worth tuning is that a single global threshold is loose for the
short tiers. If the clock did freeze during a `fast` call, 500M draws is ~3.3-4.8
seconds before the watchdog trips -- survivable, but a long time for a tier
whose whole promise is 50-100 ms. Scaling the cap per tier, at some multiple of
each tier's expected draw count, keeps the same protection proportionate:

| tier | expected draws | a proportionate cap |
|---|---|---|
| fast | 7.7M-15.4M | ~50M |
| normal | 38M-46M | ~150M |
| ultra | 250M-310M | 500M-600M as specified |

Not decided. The specified single cap is correct and safe as written; this is
a refinement, not a fix.

Worth knowing either way: the cap value is drawn from the same generator, so it
is a deterministic function of the seed and contributes no unpredictability of
its own. Its value is as a bound, not as entropy.


IMPLEMENTATION NOTES
--------------------

**Clock.** `std::chrono::steady_clock`, never `high_resolution_clock`. Measured
on this machine: `high_resolution_clock` is the same type as `system_clock`,
`is_steady=false`, and both clocks tick at 1 ns with a 21 ns measured
granularity. So there is no precision to trade away, and `system_clock` can be
stepped backwards by NTP mid-spin — which would hang the loop or end it early,
unreproducibly.

**Batch the draws.** Check the deadline once per 1000 draws. One `now()` costs
more than one draw, so checking every iteration measures the clock instead of
the generator.

**No 1 GB vector.** Storing every throwaway to fold it later costs ~1.0 GB at
ultra length and produces a result identical to a running accumulator. Fold as
you go.

**`-isystem`, not `-I`.** `pcg_extras.hpp:223` raises
`-Wunused-but-set-parameter` under the project's `-Wall -Wextra`. The Makefile's
from-scratch-rebuild-is-silent property (§9) depends on suppressing it. Note it
still surfaces through template instantiation in some builds; verify before
declaring the rebuild clean.

**`pcg-cpp`'s `discard()` is wrong for extended generators.** Measured:
`pcg32_k16384` matches `n` individual draws for n=10 and n=1000, and diverges
at n=100,000 and above. `pcg32` matches at every n tested. Do not use
`discard()` to skip ahead on a k-variant.

**Ranges.** `rng(bound)` gives [0, bound) unbiased via rejection sampling
(`pcg_extras.hpp:517`). `lo + rng(hi - lo + 1)` gives [lo, hi] inclusive; the
`+1` is what makes `hi` reachable. Never use `%` — measured skew over three
buckets was 1398410 / 904307 / 697283 against 998574 / 1001152 / 1000274 for
the unbiased path. `bound` is `result_type`, so this caps at a 2^32 span.

**Arbitrary precision.** The 32-bit bound above cannot express `ultra(40)`
(needs ~133 bits) or a bignum `.range`. Rejection-sample at the `Number` level:

    range = max - min + 1
    k     = bit_length(range)
    loop:
        r = k random bits            (ceil(k/32) draws, masked to k)
        if r < range: return min + r

Unbiased at any size, and never rejects more than half the time since
2^k < 2*range. This belongs next to bignum.hpp rather than in the RNG.


WHERE THE CODE GOES
-------------------

- **Module surface**: `eval/modules.cpp`. `call_module()` is at line 10 and
  `satellite.time.now` at line 22 is the closest existing model — a
  zero-argument module function with an arity check.
- **Format registry**: `format.def`'s `SAT_PATH` table needs rows for the new
  paths, and the words need ids in the registry.
- **A blocker for the two-form surface**: `SAT_PATH` records **one arity per
  path**, and `ultra(digits)` vs `ultra.range(min, max)` are different paths so
  they are fine — but `ultra(size)` and a proposed two-argument `ultra(min,
  max)` would be **one path with two arities**, which the format cannot encode.
  `format.def:307` documents this as a known gap that `satellite.help` already
  hit, with the §17.5 treatment recommended and undecided. Settle it once for
  both, or keep `.range` as a separate path and avoid it entirely. **Keeping
  `.range` is the recommendation** — it costs one word and sidesteps the gap.
- **Vendored library**: `pcg-cpp-0.98/` is in the tree, MIT/Apache-2.0, and the
  project's LICENSE is MIT (Expat), so there is no friction. It is currently
  untracked; decide whether it is vendored into git or listed as a build
  dependency. `ldd satl` lists six shared objects today and PCG is
  header-only, so linking it changes nothing there.


OPEN QUESTIONS
--------------

1. Whether the watchdog cap scales per tier or stays one global threshold
   (above). The specified single cap is safe either way.
2. Whether `ultra` stays PCG or moves to `getrandom(2)`. PCG makes no
   cryptographic claim and its state is recoverable from output, so the word
   "secure" cannot be used in the docs for a PCG-backed tier. `getrandom` is
   one syscall, needs no dependency, keeps `ldd` at six, and returns 256 bits
   of kernel entropy in ~1 us. Recorded as a recommendation; not decided.
3. Whether the fold stays `(acc + x) / 2` or becomes a plain sum (+18 bits).
4. Whether the timing tiers are documented as *statistical* character — which
   is what they are — rather than as a security property.

PROTOTYPE
---------

`pcg_test/pcg_test.cpp` in the tree is a working three-tier prototype with the
timer, the inclusive range, and the batching, built clean under
`-std=c++20 -Wall -Wextra -O2 -isystem ../pcg-cpp-0.98/include`. It does not
yet have the fold, the cap, or the bignum path.
