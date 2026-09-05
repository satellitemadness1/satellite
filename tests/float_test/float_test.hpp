#pragma once

// The harness, and the three sections. Three functions and a counter, no
// framework -- number_test's shape, which is the shape every suite here uses.
//
// WHAT IS BEING PROVED. PLAN M15's done-when is three clauses: the four
// operations run, the retune works, and the rounding rule is chosen. The
// retune is the evaluator's and tests/eval_test proves it; what this suite
// owns is the TYPE -- DESIGN §8.6's invariants, the operations' three
// classes, and THE RULE ITSELF: round half away from zero, ratified at M15
// (MILESTONES/M15.md §2). section_rounding is the largest section for M8's
// reason inverted -- the rounding is the one genuinely new code, everything
// else being composition over the Number M8 proved.
//
// EVERY ASSERTION COMPARES TEXT, number_test's rule for number_test's
// reason: Float::compare is one of the things under test, so a suite that
// asserted through it would be checking compare() against itself.

#include "satellite_float/satellite_float.hpp"

#include <string>

namespace float_test {

extern int failures;

void check(bool ok, const std::string &what);

// A float from text, through the two conversions under test: Number::parse,
// then from_number. Fails the suite rather than answering zero quietly.
satellite::Float of(const std::string &text);

// A number from text, for the class-3 receivers.
satellite::Number number_of(const std::string &text);

std::string text_of(const satellite::Float &value);

void section_representation(); // the invariants, normalize, places, rendering
void section_arithmetic();     // the four operations and the precision rule
void section_power();          // power and sqrt -- the class-3 answers

} // namespace float_test
