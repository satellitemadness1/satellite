#pragma once

// What `satellite_random`'s two handler files share -- the checks every draw
// makes before it answers, and the seeded tier's install half.
//
// NOT A DOOR ONTO THIS MODULE; satellite_random/handlers.hpp is. The same file
// satellite_scalars/methods_internal.hpp is over its five method files, for the
// same reason and with the same contents: the three questions both files ask,
// given external linkage because an anonymous namespace cannot be shared.
//
// PLAN §3 NAMES THIS FILE'S SHAPE BEFORE IT EXISTED. Its line rule warns that
// the first satellite's splits are "visible in the result, with headers named
// `eval_internal.hpp` existing to hold what an anonymous namespace used to" --
// and then says the thing that matters: **split by SUBJECT when you split at
// all, because a seam chosen to satisfy an arithmetic is a seam in the wrong
// place.** The seam here is a subject: `seeded` has no throwaway window, holds
// a stream the PROGRAM seeds rather than the kernel, and is the one door in
// the module that accepts a fractional bound. That is a different thing from
// the three tiers whose only difference from each other is how long they take,
// and `satellite_scalars/hex_methods.cpp` is the precedent one directory over
// -- split from `bits_methods.cpp` on the day the second radix arrived.

#include "evaluator/machine.hpp"
#include "satellite_number/bignum.hpp"

#include <string>

namespace satellite::random {

// What the refusal sentences call the callee: the spelling op_dispatch
// compiled, so `fast.range(1, 100)` is quoted as the program wrote it rather
// than as the number it folded to.
std::string callee(eval::Machine &m);

// Inclusive at both ends, so low == high is a range of one and is legal;
// low > high is empty, and there is nothing to answer with.
bool ordered(eval::Machine &m, const Number &low, const Number &high);

// The width check, TAKEN AT THE DOOR rather than left to the sampler: a span
// wider than MAX_RANDOM_DIGITS is refused before the spin it would otherwise
// pay for.
bool narrow_enough(eval::Machine &m, const Number &span);

// The seeded tier's four rows -- `1 7 13`-`1 7 16`, seeded.cpp. Called by
// install_handlers() so this module still has exactly one install door.
void install_seeded_handlers();

} // namespace satellite::random
