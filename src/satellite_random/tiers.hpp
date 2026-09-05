#pragma once

// satellite.random's twelve numbered shapes, as arithmetic -- DESIGN §11.
//
// TWO LAYERS, AND THE SEAM BETWEEN THEM IS THE TESTABLE ONE. The draw_*
// functions are the shapes pure: one answer each, from whatever Bits32 is
// handed in, no tier and no spin -- tests/eval_test drives them with the same
// splitmix32 stub tests/number_test/draw.cpp already uses, so a hundred-run
// distribution check costs microseconds rather than minutes of spinning. The
// tier_* functions are the only callers inside satl: the same three draws
// behind DESIGN §11's throwaway window, against one generator seeded once
// from the kernel.
//
// THE THROWAWAY IS WHOLE ANSWERS OF THE SHAPE BEING ASKED FOR. A request for
// a 40-digit number spins by discarding 40-digit numbers; a step request
// discards step-indexed draws. random.hpp's spin() note says why -- discarded
// work that is not the answer's work would make the window a lie -- and the
// do/while there is the "at least one, always" floor.
//
// EVERY ARGUMENT ARRIVES CHECKED. handlers.cpp refuses a fractional digit
// count, a backwards range, a step that misses -- before any of these run,
// which is random.hpp's lazy rule: a call that is going to fail must not
// spend three seconds spinning first. What is left to fail here is one thing,
// a bound wider than MAX_RANDOM_DIGITS inside the sampler, and the bool is
// that answer.

#include "satellite_number/bignum.hpp"
#include "satellite_random/random.hpp"

namespace satellite {

// Uniform over [0, 10^digits) -- `<tier>(digits)`, `1 7 4`/`1 7 7`/`1 7 10`.
// About one draw in ten of a 40-digit request renders with fewer than 40
// digits, because a leading zero is not printed and a uniform draw has one a
// tenth of the time. That is what uniform means and it is not a defect, but
// it is not what the name promises either, so DESIGN §11 says it out loud --
// here, and in the test that asserts the short answers HAPPEN.
bool draw_digits(Bits32 &bits, long long digits, Number &out);

// Uniform over [low, high], inclusive at BOTH ends -- `<tier>(min, max)` and
// its `.range` spelling, `1 7 5`/`1 7 8`/`1 7 11`. The +1 inside is the whole
// of what makes `high` reachable; without it .range(1, 100) would answer 100
// never, which is DESIGN §11's one promise broken silently.
bool draw_range(Bits32 &bits, const Number &low, const Number &high,
                Number &out);

// Uniform over {low, low + step, ..., high} -- `<tier>(min, max, step)`,
// `1 7 6`/`1 7 9`/`1 7 12`, specified 2026-09-04. The caller has already
// refused a step that does not divide high - low exactly, so `high` is a
// member of the set and "inclusive at both ends" stays true of every call
// that answers.
bool draw_step(Bits32 &bits, const Number &low, const Number &high,
               const Number &step, Number &out);

// The same three, behind the tier's window. One spin, then one answer, from
// one process-wide generator constructed on first use -- Source's note says
// what a construction costs and why it happens once. `discarded`, when asked
// for, is how many whole answers the spin threw away: PLAN §9 says measure on
// this machine rather than quote, and this count is the measurement.
bool tier_digits(Tier tier, long long digits, Number &out,
                 long long *discarded = nullptr);
bool tier_range(Tier tier, const Number &low, const Number &high, Number &out,
                long long *discarded = nullptr);
bool tier_step(Tier tier, const Number &low, const Number &high,
               const Number &step, Number &out,
               long long *discarded = nullptr);

} // namespace satellite
