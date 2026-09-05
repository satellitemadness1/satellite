// DESIGN §8.6's representation: the three invariants, normalize, places, and
// what a float prints as. The section every other section stands on, because
// an invariant that can be violated makes the arithmetic assertions about
// nothing in particular.

#include "float_test.hpp"

namespace float_test {

using satellite::Float;
using satellite::Number;

void section_representation()
{
    // §8.6's own four examples, verbatim.
    check(text_of(of("3.14")) == "3.14", "3.14 is (true, 3, 0.14)");
    check(text_of(of("-3.14")) == "-3.14", "-3.14 is (false, 3, 0.14)");
    check(text_of(of("-0.14")) == "-0.14", "-0.14 is (false, 0, 0.14)");
    check(text_of(of("0")) == "0.0", "zero is (true, 0, 0)");

    // A WHOLE FLOAT PRINTS ITS POINT -- "4.0" and never "4" -- so a float
    // never impersonates a number on the console.
    check(text_of(of("4")) == "4.0", "a whole float prints one fractional digit");

    // INVARIANT 3: THERE IS NO NEGATIVE ZERO, however one is asked for.
    check(of("0").positive(), "zero is positive");
    check(Float::assemble(false, Number(), Number()).positive(),
          "assembling a negative zero answers the positive one");
    check(text_of(of("0").negated()) == "0.0",
          "negating zero leaves it positive -- §8.6's one negate exception");
    check(Float::compare(of("0"), Float::assemble(false, Number(), Number())) == 0,
          "zero equals the zero a negative assembly answers");

    // NEGATE AND ABS ARE ONE BOOL.
    check(text_of(of("3.14").negated()) == "-3.14", "negate flips the sign");
    check(text_of(of("-3.14").abs()) == "3.14", "abs sets the one bool");

    // NORMALIZE'S CARRY: a fraction at or over 1 moves into the left half,
    // exactly, and cannot round.
    check(text_of(Float::assemble(true, Number(1), number_of("1.75"))) == "2.75",
          "assemble carries trunc(R) into L");
    check(text_of(Float::assemble(true, Number(), Number(3))) == "3.0",
          "a whole fraction carries entirely");

    // INVARIANT 1 BY CONSTRUCTION: signed halves cannot smuggle a second
    // sign in -- they are taken as magnitudes.
    check(text_of(Float::assemble(true, number_of("-2"), number_of("-0.5"))) ==
              "2.5",
          "assemble takes the halves as magnitudes");

    // PLACES IS THE PRECISION AND IT TRAVELS WITH THE VALUE. Trailing zeros
    // do not survive Number's canonical form, so 0.50 reads one place.
    check(of("3.14").places() == 2, "3.14 has two places");
    check(of("4").places() == 0, "a whole float has none");
    check(of("0.001").places() == 3, "leading fractional zeros count");
    check(of("2.50").places() == 1, "a trailing zero is not information");

    // RENDERING KEEPS THE ZEROS BETWEEN THE POINT AND THE DIGITS -- they are
    // information there, not padding -- and survives Number's own switch to
    // scientific notation on either half.
    check(text_of(of("0.00001")) == "0.00001",
          "small fractions render fixed inside a float");
    check(text_of(of("1e20")) == "100000000000000000000.0",
          "a large left half renders its zeros rather than an exponent");

    // COMPARISON IS TOTAL AND EXACT: sign first, then the halves, reversed
    // when both are negative -- through the one compare M8 wrote.
    check(Float::compare(of("2.5"), of("2.4")) > 0, "2.5 is above 2.4");
    check(Float::compare(of("-3.14"), of("-0.14")) < 0,
          "the ordering of negatives reverses");
    check(Float::compare(of("-0.5"), of("0.5")) < 0, "a true sorts above a false");
    check(Float::compare(of("1.5"), of("1.5")) == 0, "equal is equal");

    // THE EXACT JOIN AND ITS ROUND TRIP -- to_number is the operations' road
    // into M8's arithmetic and loses nothing either way.
    check(of("3.14").to_number().to_string() == "3.14", "the join is exact");
    check(of("-0.001").to_number().to_string() == "-0.001",
          "the join carries the sign once");
}

} // namespace float_test
