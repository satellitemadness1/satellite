// floor, ceil and round -- `1 6 4 7`, `1 6 4 8` and `1 6 4 9`.
//
// THREE MODES SHARING ONE BODY, which is why they are checked together. They
// differ only in whether what was dropped bumps the magnitude, and under
// sign-magnitude that decision reads the flag -- so a mode that consulted the
// wrong side of it gets every positive input right and every negative input
// wrong, which is the shape of a bug a test with three positive fixtures never
// finds. Every claim below is made twice, once each side of zero.

#include "number_test.hpp"

#include <string>

namespace number_test {

void section_rounding()
{
    // --- floor: toward negative infinity ------------------------------------
    //
    // So it is the NEGATIVE side that grows, which is the asymmetry.
    check(text_of(of("2.5").floor()) == "2", "floor(2.5) is 2");
    check(text_of(of("-2.5").floor()) == "-3", "floor(-2.5) is -3, not -2");
    check(text_of(of("2.1").floor()) == "2", "floor(2.1) is 2");
    check(text_of(of("-2.1").floor()) == "-3", "floor(-2.1) is -3");
    check(text_of(of("2").floor()) == "2", "floor leaves an integer alone");
    check(text_of(of("-2").floor()) == "-2", "and a negative integer too");
    check(text_of(of("0.5").floor()) == "0", "floor(0.5) is 0");
    check(text_of(of("-0.5").floor()) == "-1", "floor(-0.5) is -1");

    // --- ceil: toward positive infinity -------------------------------------
    check(text_of(of("2.5").ceil()) == "3", "ceil(2.5) is 3");
    check(text_of(of("-2.5").ceil()) == "-2", "ceil(-2.5) is -2, not -3");
    check(text_of(of("2.1").ceil()) == "3", "ceil(2.1) is 3");
    check(text_of(of("-2.1").ceil()) == "-2", "ceil(-2.1) is -2");
    check(text_of(of("-0.5").ceil()) == "0",
          "ceil(-0.5) is 0 -- and a POSITIVE zero, which is the one operation "
          "here that can reach DESIGN §8.6 invariant 3");
    check(of("-0.5").ceil().positive(), "and section_sign asserts the flag");

    // --- round: half away from zero -----------------------------------------
    //
    // Away from zero and not half-even, so the two sides are mirror images.
    check(text_of(of("2.5").round()) == "3", "round(2.5) is 3");
    check(text_of(of("-2.5").round()) == "-3", "round(-2.5) is -3");
    check(text_of(of("2.4").round()) == "2", "round(2.4) is 2");
    check(text_of(of("-2.4").round()) == "-2", "round(-2.4) is -2");
    check(text_of(of("3.5").round()) == "4",
          "round(3.5) is 4 -- away from zero, not to even, which would be 4 "
          "here and 2 for 2.5");
    check(text_of(of("0.4").round()) == "0", "round(0.4) is 0");
    check(text_of(of("-0.4").round()) == "0",
          "round(-0.4) is 0, and positive -- the other operation that reaches "
          "the invariant");
    check(of("-0.4").round().positive(), "and its flag");

    // --- past the small form ------------------------------------------------
    //
    // Rounding is a decimal-string split, so the boxed case is the same code --
    // but the fixture is here because "same code" is a claim about the source
    // and not about the result.
    check(text_of(of("-1234567890123456789012345678901234567890.5").floor()) ==
              "-1234567890123456789012345678901234567891",
          "floor on a boxed negative still grows the magnitude");
    check(text_of(of("-1234567890123456789012345678901234567890.5").ceil()) ==
              "-1234567890123456789012345678901234567890",
          "and ceil still does not");

    // --- is_integer, which is what all three answer about their own result ---
    check(of("2").is_integer(), "2 is an integer");
    check(of("2.0").is_integer(), "2.0 is an integer -- the trailing zero is "
                                  "spelling, not a fractional part");
    check(of("2.50").is_integer() == false, "2.50 is not");
    check(of("0").is_integer(), "zero is");
    check(of("-2").is_integer(), "so is a negative one");
    check(of("1e40").is_integer(), "and so is a value stored as an exponent");
    check(of("2.5").floor().is_integer(), "floor's answer always is");
    check(of("-2.5").ceil().is_integer(), "ceil's answer always is");
    check(of("-2.5").round().is_integer(), "round's answer always is");
}

} // namespace number_test
