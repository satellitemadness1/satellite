# satellite-004 — SATELLITE_INFINITY

`satellite.variable.float` and `satellite.variable.infinity`: what they hold, how
they are stored, the one rule every operation follows, and what `satl --configure`
measures to set `arguments.infinity`.

Written 2026-09-18 from four messages the author sent in a row while working the
design out aloud. They are kept whole in Part 1, **in order**, because the design
changed between the first and the last and the last one is the decision.

**Nothing here is built.** 004 has no float and no infinity: `satellite_float` is
listed as a future arm in `satellite_object.hpp`, and `4 / 3` still answers *"not
a whole number, and there is no satellite_float yet"*.

---

# Part 1 — the brief, as the author wrote it

**First:**

> write the float type satellite.variable.float is just two
> satellite.variable.numbers, and a satellite.variable.infinity we work with it as
> a number that is 0.9999999999999999999999999999999999999999... and we use
> however many digits the machine can process in 0.005 seconds, and we test the
> machine in --configure (change --config to have the alias --config and
> --configure that runs a test of how big of a number the machine can add which is
> at arguments.infinity it's a number of NS, the smallest amount of time the
> machine can measure, so we measure by default what the machine can add together
> in... (however many digits) the test is adding 4096 (all 9's) digits to 4096
> digits, so for arguments.infinity it will be a satellite.variable.number of 1024
> but the infinity floating point number must never reach 1, so it's always
> 0.99999999999999 and if you add two together, you get something.99999999 which
> if your adding one, the new number is
> 1.99999999999999999999999999....99999999999999999999999998 that is the new
> number, then if you added another one, it would be....
> 0.99999999999999999...999999999 + 1.99999999999999...99999999999998 the
> precision (amount of digits, the width, which is saved,) is always at 4096 if
> arguments.infinity is = 4096. So then if you do infinity * 0.5 (50 percent) it
> becomes 0.499999999999999999999999999999999999999999999999999999
>
> Do you need more to build the infinity? The amount of precision must be kept at
> arguments.infinity and the number has to only keep that many digits, build this
> prompt into SATELLITE_INFINITY.md

**Second:**

> so we use this 9999999 but you can write
> infinity_object.resize(amount_of_digits_of_precision_here_as_a_satellite.variable.number)
> to resize ONLY that infinity, and adjusts the infinity WAIT! we can adjust the
> infinity at every single operation, we HAVE to adjust the infinity every time we
> use it, so it starts off as TWO separate numbers, the whole number, and the
> number of nines, and when you add them, you only use the whole number, when you
> subtract, you only use the whole number UNLESS -- UNLESS you can't, when you
> cannot perform the operation, you sometimes use the 9's and adjust to 2.0 for
> infinity + infinity, it's like these 9's are used for something for the infinity
> and I do not know how to do it,

**Third:**

> how do you program an infinity? Store it as a floating point let's do the
> 0.9999999 precision idea, let's try that... and adjust the number for the width
> every time,

**Fourth — the decision:**

> that's it ! you adjust the width every single time, and keep it at
> whole_number.99999999999 you adjust the width every time, that is how you do it

---

# Part 2 — the earlier ruling this must agree with

**MILESTONES M11, the author, 2026-09-16** (and `satellite_config.hpp`, where the
two rows already exist):

- `infinity + infinity` displays **`infinityx2`**; `infinity - 50%` is
  **`infinityx0.5`**. *"What satellite does not know the answer to is an ERROR,
  never a guess."*
- An infinity *"carries a single satellite float for going up or down"*.
- `arguments.infinity` — a digit count, **default 4096**, *"held as a
  `satellite_number` so the user can enter anything"*.
- `arguments.infinity_display` — **default 32**, *"displayed as a rounded thing...
  we round to 32 digits"*, *"both digits configurable"*.
- **2026-09-17:** an infinity keeps a sign, as a bool.

**They are not two designs. They are one, described twice.** `infinityx2` is the
same number as `infinity + infinity` — checked exactly, not by eye:

    2 x infinity         1.9999999999999999999999999999999999999998
    infinity + infinity  1.9999999999999999999999999999999999999998    equal: True

The `x2` is the float the infinity carries (Part 2's *"single satellite float"*),
and it is the second message's *"whole number"*. Part 4 builds on that.

**ONE PIECE OF IT MUST CHANGE, and the author's own new rule is what changes it.**
09-16 says the display is *rounded* to 32 digits. Rounding an infinity to 32
digits prints this:

    infinity rounded to 32    1.00000000000000000000000000000000
    infinity truncated to 32  0.99999999999999999999999999999999

— which is an infinity **reaching 1 on the screen**, the one thing the first
message says it must never do. So the display is **cut, never rounded**. It is
recorded as I9 below and flagged for the author, because it amends his wording.

---

# Part 3 — what "adjust the width" has to mean

"Adjust the width every time" has three readings, and they give different
answers. All three were computed, at 30 digits so the ends are readable (the rule
is the same at 4096):

|       | `inf + inf` | `inf * 0.5` | |
|---|---|---|---|
| **(A)** | `1.999999999999999999999999999998` | `0.499999999999999999999999999999` | work it out exactly, then cut to the width |
| **(B)** | `1.999999999999999999999999999999` | `0.999999999999999999999999999999` | snap every answer to `<whole>.999` |
| **(C)** | `1.999999999999999999999999999999` | `0.499999999999999999999999999999` | **one unit below where it is going** |

**(B) is wrong and is ruled out by its second column**: half an infinity comes out
at nearly 1.

**(A) is what the first message computed** — its `1.999…98` is exactly (A)'s. But
under (A) **the nines wear away**:

    after adding infinity to itself 1000 times:
      (A) 999.999999999999999999999999999000
      (C) 999.999999999999999999999999999999

**(C) IS THE DECISION.** It is the fourth message — *"keep it at
whole_number.99999999999"* — and the second — *"adjust to 2.0 for infinity +
infinity"* — and it still gives the first message's `infinity * 0.5`. It does
change one thing the first message wrote: `infinity + infinity` is `1.999…99`, not
`1.999…98`. That is the cost of the nines never wearing away, and it is the reason
(C) is chosen. **Q1 asks the author to confirm it.**

---

# Part 4 — how an infinity is programmed

The author asked this directly (third message). The answer is already in the
second message: *"it starts off as TWO separate numbers, the whole number, and the
number of nines."*

## An infinity is a TARGET and a WIDTH

    target   where it is going -- a satellite.variable.float   (1 for a new infinity)
    width    how many nines     -- a satellite.variable.number  (arguments.infinity)

    its value is always   target - 10^-width

    satellite.infinity.new()       target 1,   width 4096   ->  0.999...9     (4096 nines)
    infinity + infinity            target 2                 ->  1.999...9     "adjust to 2.0"
    infinity * 0.5                 target 0.5               ->  0.4999...9
    infinity + 5                   target 6                 ->  5.999...9
    infinity.resize(10)            width 10                 ->  0.9999999999

**THE NINES ARE A COUNT, NEVER DIGITS.** This is what the second message sensed —
*"it's like these 9's are used for something"*. They are the width: how close
below the target the infinity sits. Nothing ever stores 4096 nines and nothing
ever adds them. `infinity + infinity` adds two targets (1 + 1) and keeps the count.

**THAT IS ALSO WHY IT IS FAST**, and it needs saying because the first message's
calibration assumed the opposite. Measured on this machine, quiet (load 0.64), with
the `satellite_number` that already exists:

| width | one add | one multiply |
|---|---|---|
| 1024 | 157 ns | 3.8 µs |
| 4096 | 469 ns | 59 µs |
| 16384 | 1.5 µs | 906 µs |
| 65536 | 7.0 µs | **14.8 ms** |

Stored as (C), none of those costs is paid per operation — the targets are small.
The width is only spent when the infinity is **shown** or turned into a plain
number, and display is cut to `arguments.infinity_display` (32) anyway.

**"Adjust the width every time"** is therefore one line after every operation:
the value is recomputed as `target - 10^-width`. It cannot drift, because it is
never carried — it is always derived.

## A float is TWO satellite_numbers — a mantissa and a width

The first message: *"satellite.variable.float is just two satellite.variable.numbers"*.
There are two ways to choose the two, and one of them loses data:

    as (whole, fraction)        12.05  ->  (12, 5)
                                12.5   ->  (12, 5)      <- the same pair: 12.05 IS LOST
    as (mantissa, width)        12.05  ->  (1205, 2)
                                12.5   ->  (125, 1)     <- different, as they must be
                                -0.5   ->  (-5, 1)      <- the sign lives in the mantissa

**(mantissa, width) is the one to build** — value = mantissa × 10^-width. It is
still exactly two `satellite_number`s. A fraction's leading zero survives because
the width knows it is there, which is the same reason a binary keeps `b0101` and
a plain number does not keep `021`. **Q6 asks the author to confirm.**

A float's own arithmetic also "adjusts the width every time": `1 / 3` is cut at
`arguments.infinity` digits, by truncation, never rounding — the same rule, so a
float and an infinity agree about what a digit past the width means.

---

# Part 5 — `satl --configure` and `arguments.infinity`

**What the brief asks for:**

- `--configure` is a new spelling of `--config`, and it also runs the infinity test.
- The test adds two 4096-digit numbers of all nines and times it in nanoseconds —
  *"the smallest amount of time the machine can measure"*.
- The budget is **0.005 seconds** — *"however many digits the machine can process
  in 0.005 seconds"*.

**Two things in it contradict each other, and the author needs to settle them (Q3):**

1. **Is `arguments.infinity` a number of nanoseconds or a number of digits?** The
   brief says *"arguments.infinity it's a number of NS"*, and also *"the precision…
   is always at 4096 if arguments.infinity is = 4096"*. The row that already exists
   (09-16) is a digit count, default 4096.
2. **Is the default 4096 or 1024?** *"for arguments.infinity it will be a
   satellite.variable.number of 1024"* against the existing row's 4096.

**AND ONE MEASUREMENT CHANGES THE TEST ITSELF (Q4):** calibrating on an ADD sets a
width no MULTIPLY can survive. Adding is linear, so 5 ms of it buys a width of
**tens of millions of digits** (4096 digits in 469 ns). Multiplying is quadratic —
four times the width costs sixteen times the time — so at that width one
`infinity * infinity` would run for hours. Under (C) this matters less, because the
width is only paid for at display, but a float's `1 / 3` DOES pay it on every
division. **The test should time a multiply.** At 5 ms, that lands between 16384
(0.9 ms) and 65536 (14.8 ms) digits on this machine.

---

# Part 6 — what blocks building, and what does not

**The author's answers are needed for these.** Each has what will be built if he
says nothing, so none of them stops the work.

| | question | built unless told otherwise |
|---|---|---|
| **Q1** | (C) makes `infinity + infinity` = `1.999…99`, where the first message wrote `1.999…98`. Confirm (C)? | **(C)** — the fourth message |
| **Q2** | `infinity - infinity` and `infinity * 0` have target 0, so the value is *just below zero*: `-0.000…1`. Is that a value, or is it *"an ERROR, never a guess"* (09-16)? | **an error**, per 09-16 |
| **Q3** | `arguments.infinity`: digits or nanoseconds? default 4096 or 1024? | **digits, 4096** — the existing row |
| **Q4** | time a multiply rather than an add in `--configure`? | **multiply** |
| **Q5** | two infinities of different widths: which width does the answer keep? | **the smaller** — no answer is more precise than what went in |
| **Q6** | float as (mantissa, width) rather than (whole, fraction)? | **(mantissa, width)** |
| **Q7** | display cut to 32 rather than rounded (it amends 09-16's "round")? | **cut** |

**Not questions — already decided:** the sign is a bool (09-17); `.resize(n)`
changes one infinity's width and nothing else; `12.34.reverse()` is `34.12` and
`.reverse().reverse()` is `21.43` (PLAN.md red note 9, which also says to read the
CHAIN rather than put a hidden flag on the number).

---

# Part 7 — milestones

**I1 — `satellite_float`, the arm.** Arm 12 of `satelliteObject` (the plan in
`satellite_object.hpp` already names it). Two `satellite_number`s: mantissa and
width. Word `satellite.variable.float` is `1 6 10`, already numbered.

**I2 — float literals and display.** `12.34` lexes to a float; it displays as
written. Today the lexer has no decimal token at all.

**I3 — float arithmetic, width adjusted every time.** `+ - * /` on two floats and
on a float and a number. Division truncates at `arguments.infinity` digits. **`4 /
3` stops being refused** — every "there is no satellite_float yet" in the tree
becomes an answer.

**I4 — `satellite.variable.infinity` and `satellite.infinity.new()`.** New words:
`satellite.variable.infinity` takes `1 6 17`; the `satellite.infinity` family takes
`1 26`. Its arm is the next free one after the float's. A target, a width and a
sign.

**I5 — infinity arithmetic on targets.** `+ - * /` against an infinity, a float, a
number and a percentage. The value is `target - 10^-width`, derived after every
operation, never carried.

**I6 — `.resize(n)`.** A new method token. One infinity's width, and nothing else.

**I7 — ordering.** `infinity < 1` is true; `infinity == infinity` is true;
`infinity < 2` is true. Falls out of the value, so it is a test more than a build.

**I8 — `arguments.infinity` read at last.** The row exists and *"is read by nothing
yet"* (its own comment). A new infinity takes its width from it.

**I9 — display, cut to `arguments.infinity_display`.** Never rounded (Part 2). That
row is read at last too.

**I10 — `--configure`.** An alias of `--config` that also times the infinity test
and writes `arguments.infinity`.

**I11 — a float's `.reverse()`.** `34.12` and `21.43`, read from the chain (red
note 9).

**Order:** I1 → I2 → I3 is the float, and is useful alone. I4 → I5 → I6 → I9 is
the infinity, and needs the float. I7, I8, I10 and I11 can go anywhere after.
