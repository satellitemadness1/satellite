// Exact where DESIGN §8.1 promises exact, and rounded only where it says an
// answer does not otherwise exist.
//
// THE CLAUSE THIS SECTION IS FOR is PLAN M8's first: "exact arbitrary-precision
// arithmetic runs". §8.1 is precise about what that covers -- "addition,
// subtraction and multiplication are always exact, with no bound on the number
// of digits. So is division whose result terminates" -- and the two halves fail
// differently. The first is checked by writing down answers a double cannot
// produce; the second by dividing things whose expansion is longer than
// division_digits and asserting they come back whole anyway.

#include "number_test.hpp"

#include "machine_limits/limits.hpp"

#include <string>

namespace number_test {

using satellite::Number;

namespace {

// The digit budget every assertion here uses, so that a machine with a
// satellite_config.ini cannot change what this suite asserts. It is 34 because
// that is what limits::division_digits() answers when nothing has set the dial,
// and the two are checked against each other below rather than assumed equal.
constexpr int kDigits = 34;

} // namespace

void section_arithmetic()
{
    // --- the dial's meaning, which is M8's to give ---------------------------
    //
    // M6 built the storage and left it unset deliberately: "the milestone that
    // will read it decides what no answer means". This is that decision, and it
    // is asserted here rather than in limits_test because the number is only
    // meaningful next to the division it governs.
    check(satellite::limits::division_digits() == kDigits,
          "an unset division_digits means 34 -- decimal128's precision, which "
          "is M8's answer to the hole M6 left");
    check(satellite::limits::kDivisionDigitsDefault == kDigits,
          "and the constant beside the dial is the same 34");

    // --- what a double cannot do --------------------------------------------
    check(text_of(Number::add(of("0.1"), of("0.2"))) == "0.3",
          "0.1 + 0.2 is 0.3, not 0.30000000000000004");
    check(text_of(Number::add(of("1e20"), of("1"))) == "100000000000000000001",
          "1e20 + 1 keeps the 1, which a double loses entirely");
    check(text_of(Number::sub(of("1e20"), of("1"))) == "99999999999999999999",
          "and the subtraction too");
    check(text_of(Number::divide(of("1000000000"), of("1000000000"), kDigits)) ==
              "1",
          "a count of nanoseconds over a billion is an exact number of seconds");

    // Addition and multiplication have no width at which they stop being exact
    // -- DESIGN §7.5's rule that the language has no limits, one type down.
    check(text_of(Number::mul(of("123456789012345678901234567890"),
                              of("987654321098765432109876543210"))) ==
              "121932631137021795226185032733622923332237463801111263526900",
          "a sixty-digit exact product, with no rounding anywhere in it");
    check(text_of(Number::add(
              of("99999999999999999999999999999999999999999999999999"),
              of("1"))) == "1e+50",
          "a carry the whole width of the value");

    // --- exact where the expansion terminates -------------------------------
    //
    // number_arith.cpp's own recorded bug: a non-zero remainder means the
    // division did not terminate WITHIN THE SCALE CHOSEN, which is not the same
    // as not terminating. Both cases below were wrong in v1 before that fix and
    // both are invisible until a dividend is longer than division_digits.
    check(text_of(Number::divide(
              of("1"), of("1267650600228229401496703205376"), kDigits)) ==
              "7.888609052210118054117285652827862296732064351090230047702789"
              "306640625e-31",
          "1 / 2^100 terminates at 69 places and comes back whole, though "
          "that is twice division_digits");
    check(text_of(Number::divide(
              of("10000000000000000000000000000000000000001"), of("100"),
              kDigits)) == "100000000000000000000000000000000000000.01",
          "(1e40 + 1) / 100 terminates at two places and keeps all 41 digits, "
          "though the dividend alone is longer than division_digits");
    check(text_of(Number::divide(of("1"), of("8"), kDigits)) == "0.125",
          "an ordinary terminating division is exact and short");
    check(text_of(Number::divide(of("1"), of("10"), kDigits)) == "0.1",
          "division by a power of ten is exact");

    // --- rounded only where no exact answer exists --------------------------
    check(text_of(Number::divide(of("1"), of("3"), kDigits)) ==
              "0.3333333333333333333333333333333333",
          "1 / 3 rounds to division_digits, because there is nothing else "
          "anyone could do");
    check(of("0.3333333333333333333333333333333333").digit_count() == kDigits,
          "and it is exactly 34 significant digits, not 33 or 35");
    check(text_of(Number::divide(of("2"), of("3"), kDigits)) ==
              "0.6666666666666666666666666666666667",
          "2 / 3 rounds the last digit UP, which is the half-up rule the guard "
          "digit exists for");
    check(text_of(Number::divide(of("1"), of("3"), 5)) == "0.33333",
          "a narrower budget is honoured");
    check(text_of(Number::divide(of("1"), of("3"), 1)) == "0.3",
          "and the narrowest is one digit");

    // THERE IS NO CEILING, AND THIS ASSERTION IS THE REVERSE OF THE ONE IT
    // REPLACED. Until 2026-08-31 it read "a budget above kMaxDivisionDigits is
    // clamped to it" and passed, over a divide() that answered 10,000 digits to
    // a program asking for 11,000 and said nothing. DESIGN §7.5 does not allow
    // the constant and DESIGN §1.1 does not allow the silence; the constant is
    // gone, so what is checked now is that a wide count is a wide answer.
    // MILESTONES/M8.md §6 is where the change is written down.
    check(of(text_of(Number::divide(of("1"), of("3"), 11000))).digit_count() ==
              11000,
          "a budget past the deleted 10,000-digit ceiling is honoured in full");

    // AND ZERO IS THE ONE COUNT THAT IS NOT AN ANSWER, which used to be floored
    // to one digit here and is now the same non-answer a zero divisor is. The
    // value a person can actually type -- `division_digits=0` in a
    // satellite_config.ini -- never reaches this function at all: tests/limits_test
    // asserts it is refused with S0807 and a caret under the `0`.
    check(text_of(Number::divide(of("1"), of("3"), 0)) == "0",
          "a budget of zero keeps no digits and so has no answer, exactly as a "
          "zero divisor has none");

    // AND THE SAME OVER A DIVISION THAT TERMINATES, WHICH IS THE FIXTURE THAT
    // MAKES THE ONE ABOVE WORTH ANYTHING. Found by mutation on 2026-08-31:
    // deleting the `digits == 0` guard failed NOTHING, because 1 / 3 reaches
    // the rounding branch, resizes its digits to none and comes back as zero by
    // arithmetic rather than by rule. 1 / 2 does not reach that branch at all --
    // it is exact, so without the guard it answers 0.5 to a caller who asked
    // for no digits, and the two zero-budget divisions disagree.
    check(text_of(Number::divide(of("1"), of("2"), 0)) == "0",
          "including a division that TERMINATES, which is where a zero budget "
          "would otherwise slip past the rounding branch and answer 0.5");

    // --- the small form holds what it claims to -----------------------------
    //
    // DESIGN §8.2: "a loop counter, an index and every small literal live
    // entirely in a long long, and i = i + 1 is an add and an overflow check
    // with no allocation". The claim is about ALLOCATION, so it is checked by
    // asking what the result cost rather than what it is.
    Number counter = Number(0);
    for (int i = 0; i < 1000; i++)
        counter = Number::add(counter, Number(1));
    check(text_of(counter) == "1000", "a thousand increments reach 1000");
    check(counter.payload_bytes() == 0,
          "and not one of them allocated -- DESIGN §8.2's small case");

    // THE SMALL FORM GOT WIDER, WHICH IS THE FIRST OF TWO THINGS THE UNSIGNED
    // SIGNIFICAND PAID FOR AND THE ONE A FIXTURE FOUND. Writing this section
    // the first time, `9000000000000000000 + 9000000000000000000` was expected
    // to overflow the small form and box -- it does under a SIGNED significand,
    // where the ceiling is LLONG_MAX. It does not here: 1.8e19 is under
    // ULLONG_MAX, so the sum stays inline and allocates nothing. The old
    // fixture failed, which is the assertion arriving the right way round.
    const Number half_of_wide = of("9000000000000000000");
    check(Number::add(half_of_wide, half_of_wide).payload_bytes() == 0,
          "9e18 + 9e18 stays inline -- v1's signed significand would have "
          "boxed it, and the magnitude being unsigned is what moved the "
          "ceiling from LLONG_MAX to ULLONG_MAX");
    check(text_of(Number::add(half_of_wide, half_of_wide)) ==
              "18000000000000000000",
          "and the answer is right at the top of the range");

    // AND THE OPPOSITE-SIGN CASE CANNOT OVERFLOW AT ALL, which is the second
    // and is the half DESIGN §8.6 does not mention. number_arith.cpp records
    // it: subtracting the smaller unsigned magnitude from the larger always
    // fits, so that branch never falls through to the BigInt path -- where v1's
    // signed add fell through whenever the sum left the range.
    const Number wide = of("12345678901234567891");       // over ULLONG_MAX/2
    check(Number::add(wide, wide).payload_bytes() != 0,
          "same-sign addition CAN still overflow, past ULLONG_MAX, and boxes");
    check(Number::add(wide, wide.negated()).payload_bytes() == 0,
          "the SAME two magnitudes with opposite signs cannot overflow and "
          "stay inline");
    check(text_of(Number::add(wide, wide.negated())) == "0",
          "and give zero");
}

} // namespace number_test
