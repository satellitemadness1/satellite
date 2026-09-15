// parse and to_string -- `1 6 4 6`, and the two halves of "print the VALUE".
//
// THE NOTATION BOUNDARY IS THE SUBJECT AND IT IS NOT THE ONE THE FIRST
// SATELLITE WROTE DOWN. render.cpp restates it: switch to scientific when the
// PADDING ZEROS would outnumber the information, never when the information
// itself is long. The old phrasing was a fixed range, [1e-6, 1e21), and it was
// written when a number was a double -- where a 33-digit integer had no 33
// informative digits. An exact type that renders 30! as
// 2.6525285981219105863630848e+32 is hiding the answer it went to the trouble
// of computing. Both thresholds below are the old boundaries restated, so every
// value that printed fixed before still does.

#include "number_test.hpp"

#include <string>

namespace number_test {

using satellite::Number;

void section_text()
{
    // --- a round trip through both halves -----------------------------------
    const char *const round_trips[] = {
        "0",      "1",      "-1",     "2.5",    "-2.5",   "0.1",
        "1000",   "-1000",  "0.001",  "-0.001", "123456789012345678901234567890",
        "-123456789012345678901234567890",
    };
    for (const char *const text : round_trips)
        check(text_of(of(text)) == text,
              std::string("`") + text + "` parses and prints back as itself");

    // --- what parse refuses -------------------------------------------------
    //
    // The grammar is digits, optionally a `.` with more digits, optionally an
    // `e` and an exponent -- and nothing else. S0610's sentence says exactly
    // that, so a shape parse accepted and the sentence did not describe would
    // be the message lying.
    Number ignored;
    const char *const refused[] = {
        "",     " ",    "1 ",   " 1",   "1.2.3", "1e",   "1e+",  "e5",
        "abc",  "1a",   "--1",  "+-1",  "0x10",  ".",    "1,000", "1_000",
    };
    for (const char *const text : refused)
        check(!Number::parse(text, ignored),
              std::string("`") + text + "` is refused rather than half-read");

    // `.5` and `1.` are the two nearly-legal ones, and they are on opposite
    // sides: a bare fraction has digits and a trailing point does too, so both
    // parse. Asserted rather than left implicit, because "what parse takes" is
    // S0610's sentence and the sentence has to be true.
    check(Number::parse(".5", ignored) && text_of(ignored) == "0.5",
          "`.5` parses -- it has digits, which is what the sentence asks for");
    check(Number::parse("1.", ignored) && text_of(ignored) == "1",
          "`1.` parses too, and prints without the point");

    // --- trailing zeros are spelling ----------------------------------------
    check(text_of(of("2.50")) == "2.5", "2.50 prints as 2.5");
    check(text_of(of("5.0")) == "5", "5.0 prints as 5");
    check(text_of(of("0.000")) == "0", "0.000 prints as 0");
    check(text_of(of("-0.0")) == "0", "-0.0 prints as 0, sign and all");
    check(of("2.50") == of("2.5"), "and they are one value, not two");

    // --- the boundary, both sides -------------------------------------------
    //
    // Twenty trailing zeros is 1e20 and five leading zeros is 1e-6, which are
    // the old fixed boundaries restated as counts of padding.
    check(text_of(of("1e20")) == "100000000000000000000",
          "1e20 is twenty zeros and prints whole");
    check(text_of(of("1e21")) == "1e+21", "1e21 is one too many and goes scientific");
    check(text_of(of("1e-6")) == "0.000001", "1e-6 prints whole");
    check(text_of(of("1e-7")) == "1e-7", "1e-7 goes scientific");
    check(text_of(of("1e308")) == "1e+308",
          "1e308 is one significant digit and 308 zeros, so it stays "
          "scientific -- the padding is what decides");

    // AND THE CASE THE RESTATEMENT EXISTS FOR. 30! is 26 significant digits and
    // 7 trailing zeros: long, but almost all of it information.
    Number factorial = Number(1);
    for (int i = 2; i <= 30; i++)
        factorial = Number::mul(factorial, Number(i));
    check(text_of(factorial) == "265252859812191058636308480000000",
          "30! prints whole -- 26 significant digits and 7 zeros, which the "
          "old fixed [1e-6, 1e21) would have rendered as 2.65...e+32 and hidden");
    check(factorial.digit_count() == 33, "and it is 33 digits written out");

    // --- the sign in the rendering ------------------------------------------
    check(text_of(of("-1e21")) == "-1e+21", "a negative in scientific notation");
    check(text_of(of("-1e-7")) == "-1e-7", "and with a negative exponent");
    check(text_of(of("-0.000001")) == "-0.000001", "and in fixed notation");
}

} // namespace number_test
