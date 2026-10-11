#pragma once
// satellite/satellite_random/random_draw.hpp -- the uniform draw at any size, from 64-bit
// limbs, and the three shapes as one. Nothing here knows what a PCG is; it asks a LimbSource
// for a limb at a time.
//
// REJECTION, NEVER A REMAINDER. `limb % bound` is skewed whenever bound does not divide
// 2^64 -- the low residues get one extra preimage each -- and the first satellite measured
// the skew at 2:1 over three buckets (its DESIGN §18), invisible to any test that only
// checks the range. So a draw takes exactly as many bits as the largest value it may be,
// and draws again when it lands past the bound: at most half the time, by construction.
//
// LIMB-SIZED, BECAUSE 004'S NUMBERS ARE. 001 and 003 kept numbers in base 10^9 and drew a
// limb at a time over [0, 10^9); satellite_number is 64-bit limbs, so the textbook form
// comes back: k bits for a bound of k bits, the top limb masked, which rejects nothing at
// all when the bound is a power of two.
//
// ONE SHAPE FOR THE THREE WORDS. "twenty digits", "1 to 6" and "0 to 100 by 5" are each a
// count of values the draw may be, a base, and a step: the answer is base + step * (uniform
// below count). The shape is worked out ONCE a call -- 10^digits is a power the spin must
// not recompute on every throwaway -- and the spin throws away draws of exactly that shape
// (random_spin.hpp), which is what "random numbers of the size requested" means.
//
// NO CEILING ON THE SIZE. 001 and 003 refused more than 100,000 digits; 004's rule since
// 2026-09-25 is that the language has no limits of its own (satellite_number: "a bound is
// never the fix"), so ultra(1000000) builds 10^1000000 and draws under it, and the machine's
// memory is the only limit, reported as such.
//
// Written 2026-10-02.

#include "random_source.hpp"
#include "../satellite_variable_number/satellite_number.hpp"

namespace satellite004 {

// Uniform over 0 .. bound - 1. A bound of 1 answers 0; a bound of 0 or below answers 0 too,
// as the one thing it can say -- every caller has refused those before asking.
satellite_number random_below(LimbSource &limbs, const satellite_number &bound);

// What a call draws: base + step * (uniform below count).
struct DrawShape {
    satellite_number count;   // how many values the draw may be: 1 or more
    satellite_number base;    // the least of them
    satellite_number step;    // the distance between two of them
};

// "digits": 10^digits values from 0, by 1 -- so 20 digits is 0 through twenty nines, and
// about one draw in ten prints 19 digits or fewer, which is what uniform means. A count
// below 0 is a shape of one value, 0.
DrawShape digits_shape(const satellite_number &digits);

// "min, max": most - least + 1 values from least, by 1, both ends in. least above most is a
// shape of one value, least -- the caller has refused it before asking.
DrawShape range_shape(const satellite_number &least, const satellite_number &most);

// "min, max, step": (most - least) / step + 1 values from least, by step. The caller has
// checked that step is 1 or more and lands on most; when it does not, the draw still never
// passes most.
DrawShape stepped_shape(const satellite_number &least, const satellite_number &most, const satellite_number &step);

// One draw of a shape.
satellite_number draw(LimbSource &limbs, const DrawShape &shape);

} // namespace satellite004
