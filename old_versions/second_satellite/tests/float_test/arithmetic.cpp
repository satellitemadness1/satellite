// The four operations, DESIGN §8.6's three classes, and THE RULE -- round
// half away from zero, chosen at M15. The rounding assertions here are the
// written form of the decision: change the rule and this file names every
// place the language's behaviour moves, which is the visible-edit property
// tests/number_test/arithmetic.cpp already gives Number::divide.

#include "float_test.hpp"

namespace float_test {

using satellite::Float;

void section_arithmetic()
{
    // --- rounded_to: the rule itself, half away from zero ------------------
    check(text_of(of("2.345").rounded_to(2)) == "2.35",
          "a tie rounds away from zero");
    check(text_of(of("-2.345").rounded_to(2)) == "-2.35",
          "and away from zero on the negative side -- the sign is decided "
          "before the rounding and never consulted by it");
    check(text_of(of("2.3449").rounded_to(2)) == "2.34", "below half is toward");
    check(text_of(of("2.3451").rounded_to(2)) == "2.35", "above half is away");
    check(text_of(of("0.996").rounded_to(2)) == "1.0",
          "a rounded fraction can carry into the left half");
    check(text_of(of("0.25").rounded_to(1)) == "0.3", "0.25 to one place is 0.3");
    check(text_of(of("2.34").rounded_to(6)) == "2.34",
          "rounding never manufactures digits");

    // --- addition and subtraction: EXACT, never round ----------------------
    check(text_of(Float::add(of("0.1"), of("0.2"))) == "0.3",
          "0.1 + 0.2 is 0.3 -- the property `double` does not have");
    check(text_of(Float::add(of("3.14"), of("2.86"))) == "6.0",
          "a carry lands in the unbounded half");
    check(text_of(Float::add(of("2.5"), of("-3.75"))) == "-1.25",
          "opposite signs: the result takes the larger magnitude's sign");
    check(text_of(Float::sub(of("3.14"), of("3.14"))) == "0.0",
          "a difference of equals is the positive zero");
    check(text_of(Float::sub(of("-1.5"), of("-2.75"))) == "1.25",
          "subtraction is addition of the negation");

    // --- multiplication: exact product, one rounding at the result's
    // precision -- max of the operands', floored at float_digits ------------
    check(text_of(Float::mul(of("3.14"), of("2"), 34)) == "6.28",
          "a product inside the precision is exact");
    check(text_of(Float::mul(of("-3.14"), of("2"), 34)) == "-6.28",
          "the sign is the flags agreeing, decided before the arithmetic");
    check(text_of(Float::mul(of("0.5"), of("0.5"), 1)) == "0.3",
          "the bound on R bites: 0.25 rounds to one place, away from zero");
    check(text_of(Float::mul(of("0.5"), of("0.5"), 4)) == "0.25",
          "the floor at float_digits raises the target instead of cutting");

    // --- division: always rounds, correctly, and a tie is a proved tie -----
    check(text_of(Float::divide(of("1"), of("3"), 5)) == "0.33333",
          "1/3 rounds at the target places");
    check(text_of(Float::divide(of("2"), of("3"), 5)) == "0.66667",
          "2/3 rounds up -- the same guard-digit bump Number::divide takes");
    check(text_of(Float::divide(of("1"), of("8"), 2)) == "0.13",
          "0.125 at two places is a TRUE tie, proved by cross-multiplication, "
          "and the rule sends it away from zero");
    check(text_of(Float::divide(of("1"), of("8"), 3)) == "0.125",
          "the same division inside the precision is exact and untouched");
    check(text_of(Float::divide(of("2.5"), of("0.5"), 34)) == "5.0",
          "an exact quotient comes back exact");
    check(text_of(Float::divide(of("-1"), of("4"), 34)) == "-0.25",
          "division's sign is the flags disagreeing");
    check(text_of(Float::divide(of("0"), of("7"), 34)) == "0.0",
          "zero divided is the positive zero");

    // THE PRECISION RULE ACROSS AN EXPRESSION: precision never silently
    // shrinks -- the wider operand carries.
    check(Float::divide(of("1"), of("3"), 5).places() == 5,
          "a quotient's places are the floored target");
    check(text_of(Float::mul(Float::divide(of("1"), of("3"), 5), of("3"), 5)) ==
              "0.99999",
          "and the wider operand's precision carries through a product");
}

} // namespace float_test
