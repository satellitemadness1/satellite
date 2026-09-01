// DESIGN §8.1's sign, which is the whole of what M8 changed about the port.
//
// THE LARGEST SECTION HERE, AND THE ONLY ONE WHOSE SUBJECT IS NEW. The first
// satellite's arithmetic worked; what did not exist was an explicit `positive`
// bool with the magnitude carrying none. Three claims decompose out of that and
// each has its own block below: the flag defaults to true, there is no negative
// zero on ANY path that can produce a zero, and the ordering of negatives is
// right -- which is the one a wrong sign shows up in rather than hides in.

#include "number_test.hpp"

#include <climits>
#include <string>

namespace number_test {

using satellite::Number;

namespace {

// Every way this module can produce a zero. Enumerated rather than sampled,
// because "there is no negative zero" is a claim about paths and a test that
// checked three of them would be checking three of them.
//
// A LIST AND NOT A LOOP OVER OPERATIONS, so that a path added later is a line
// added here rather than a case nobody noticed was missing.
struct Zero {
    const char *how;
    Number value;
};

std::vector<Zero> every_zero()
{
    return {
        { "a default-constructed Number", Number() },
        { "the literal 0", of("0") },
        { "the literal -0", of("-0") },
        { "the literal -0.000", of("-0.000") },
        { "Number(0)", Number(0) },
        { "negate(0)", of("0").negated() },
        { "negate(-0)", of("-0").negated() },
        { "abs(0)", of("0").abs() },
        { "5 + -5", Number::add(of("5"), of("-5")) },
        { "-5 + 5", Number::add(of("-5"), of("5")) },
        { "5 - 5", Number::sub(of("5"), of("5")) },
        { "-5 - -5", Number::sub(of("-5"), of("-5")) },
        { "1e18 + -1e18 (the small form)",
          Number::add(of("1000000000000000000"), of("-1000000000000000000")) },
        { "a boxed magnitude plus its negation",
          Number::add(of("1234567890123456789012345678901234567890"),
                      of("-1234567890123456789012345678901234567890")) },
        { "-7 * 0", Number::mul(of("-7"), of("0")) },
        { "0 * -7", Number::mul(of("0"), of("-7")) },
        { "0 / -7", Number::divide(of("0"), of("-7"), 34) },
        { "ceil(-0.5)", of("-0.5").ceil() },
        { "round(-0.4)", of("-0.4").round() },
        { "floor(0.5)", of("0.5").floor() },
        { "-8 % 4", Number::modulo(of("-8"), of("4")) },
        { "from_small(false, 0, 3)", Number::from_small(false, 0, 3) },
    };
}

} // namespace

void section_sign()
{
    // --- the flag defaults to true ------------------------------------------
    //
    // DESIGN §8.1: "a number with no sign written is positive, and something
    // has to flip the flag for it to be otherwise". The default constructor is
    // where that is either true or a bug nothing else can see.
    check(Number().positive(), "a default-constructed Number is positive");
    check(of("7").positive(), "a literal with no sign written is positive");
    check(!of("-7").positive(), "a literal written with `-` is not positive");
    check(of("+7").positive(), "a literal written with `+` is positive");
    check(Number(42).positive(), "Number(42) is positive");
    check(!Number(-42).positive(), "Number(-42) is not positive");

    // --- there is no negative zero, on any path -----------------------------
    //
    // DESIGN §8.6 invariant 3, which §8.1 shares: "positive is true when L and
    // R are both zero -- there is no negative zero". §8.6 says why it is not
    // fussiness: -0 would compare unequal to 0 while PRINTING the same, and
    // QUAD's seven sort comparators are all `if (a != b) return a > b`.
    for (const Zero &zero : every_zero()) {
        check(zero.value.is_zero(),
              std::string(zero.how) + " is zero");
        check(zero.value.positive(),
              std::string(zero.how) + " is a POSITIVE zero -- DESIGN §8.6 "
              "invariant 3 forbids the other one");
        check(!zero.value.is_negative(),
              std::string(zero.how) + " does not answer is_negative()");
        check(text_of(zero.value) == "0",
              std::string(zero.how) + " prints as `0` and not as `-0`");
        check(Number::compare(zero.value, Number()) == 0,
              std::string(zero.how) + " compares equal to a plain zero");
    }

    // AND THE ONE THAT CANNOT BE READ OFF A PRINT. -0 and 0 printing the same
    // is exactly the failure §8.6 describes, so the assertion that matters is
    // the comparison rather than the text.
    check(of("-0") == of("0"), "-0 and 0 are the same value, not two");
    check(Number::compare(of("-0"), of("0")) == 0, "-0 does not sort against 0");

    // --- negation and abs ---------------------------------------------------
    check(text_of(of("7").negated()) == "-7", "negate(7) is -7");
    check(text_of(of("-7").negated()) == "7", "negate(-7) is 7");
    check(text_of(of("-7").abs()) == "7", "abs(-7) is 7");
    check(text_of(of("7").abs()) == "7", "abs(7) is 7");
    check(text_of(of("0").abs()) == "0", "abs(0) is 0");
    // A GENUINELY BOXED MAGNITUDE, and not 1e40 -- which normalises to a
    // significand of 1 and an exponent of 40 and never leaves the small form.
    // A check written against that one would pass whatever negation did.
    const Number boxed = of("-1234567890123456789012345678901234567890");
    check(boxed.payload_bytes() != 0, "the fixture is actually boxed");
    check(text_of(boxed.negated()) == "1234567890123456789012345678901234567890",
          "negating a boxed magnitude gives the magnitude back");
    check(boxed.negated().payload_id() == boxed.payload_id(),
          "negation SHARES the boxed magnitude rather than copying it -- one "
          "bool and one refcount, which is what sign-magnitude buys");
    check(boxed.abs().payload_id() == boxed.payload_id(),
          "abs shares it too, which DESIGN §8.6 predicted: `abs sets one bool`");

    // --- the ordering of negatives ------------------------------------------
    //
    // PLAN M8's done-when names this on its own, and it is the clause a wrong
    // sign shows up in: a compare() that forgot to reverse inside the negative
    // half gets every positive pair right and every negative pair backwards.
    check(Number::compare(of("-5"), of("-9")) > 0, "-5 is greater than -9");
    check(Number::compare(of("-9"), of("-5")) < 0, "-9 is less than -5");
    check(Number::compare(of("-1"), of("0")) < 0, "-1 is less than 0");
    check(Number::compare(of("0"), of("-1")) > 0, "0 is greater than -1");
    check(Number::compare(of("-1"), of("1")) < 0, "-1 is less than 1");
    check(Number::compare(of("-2.5"), of("-2.50")) == 0,
          "-2.5 and -2.50 are one value written two ways");
    check(Number::compare(of("-2.5"), of("-2.4")) < 0,
          "-2.5 is less than -2.4, which is the fractional half of the reversal");

    // THREE SITES REVERSE, AND EACH NEEDS ITS OWN FIXTURES -- which is what a
    // mutation run found rather than what the first draft of this block
    // assumed. compare() has a hot path (both small, one scale), a scaled small
    // path (both small, different scales, aligned in an unsigned long long) and
    // a general path (at least one magnitude boxed), and each ends in its own
    // `sa < 0 ? -c : c`. Deleting the reversal from the general path failed
    // exactly ONE assertion here, and from the scaled path exactly one -- so
    // two thirds of the negative ordering rested on a single fixture each.
    // MILESTONES/M7.md records the same shape of finding about resolve's cache
    // counts, and the fix is the same: assert per mechanism, not per claim.

    // The scaled small path: both operands small, different exponents, and the
    // alignment fits an unsigned long long.
    check(Number::compare(of("-1e-30"), of("-1e-40")) < 0,
          "-1e-30 is less than -1e-40, which the exponent alone would get wrong");
    check(Number::compare(of("-1e-40"), of("-1e-30")) > 0, "and the other way");
    check(Number::compare(of("-2.5"), of("-2.25")) < 0,
          "-2.5 is less than -2.25 across two scales");
    check(Number::compare(of("-2.25"), of("-2.5")) > 0, "and the other way");
    check(Number::compare(of("-100"), of("-99.5")) < 0,
          "-100 is less than -99.5, which is the scaled path with the shorter "
          "magnitude on the losing side");
    check(Number::compare(of("-0.001"), of("-0.0001")) < 0,
          "and again below one");

    // The general path: at least one magnitude boxed, so the comparison goes
    // through BigInt and the alignment cannot stay in a machine word.
    const std::string wide_a = "-1234567890123456789012345678901234567890";
    const std::string wide_b = "-1234567890123456789012345678901234567891";
    check(of(wide_a).payload_bytes() != 0, "the boxed fixtures are boxed");
    check(Number::compare(of(wide_a), of(wide_a)) == 0,
          "a boxed negative equals itself");
    check(Number::compare(of(wide_a), of(wide_b)) > 0,
          "the reversal holds once the magnitude is boxed");
    check(Number::compare(of(wide_b), of(wide_a)) < 0, "and the other way");
    check(Number::compare(of(wide_a), of("-1")) < 0,
          "a large negative is less than a small one");
    check(Number::compare(of("-1"), of(wide_a)) > 0, "and the other way");
    check(Number::compare(of(wide_a), of("1")) < 0,
          "a large negative is less than any positive");
    check(Number::compare(of(wide_a),
                          of("-1234567890123456789012345678901234567890.5")) > 0,
          "and the reversal holds when the boxed pair differs below the point");
    check(Number::compare(of("-1e-3000"), of("-1e-4000")) < 0,
          "and when the scaling is far past what a machine word holds");

    // max, min and clamp are the three M8 added, and they are compare() wearing
    // a name -- so what is asserted is that they agree with it on negatives,
    // which is where a hand-written comparison would be wrong.
    check(text_of(Number::max(of("-5"), of("-9"))) == "-5", "max(-5, -9) is -5");
    check(text_of(Number::min(of("-5"), of("-9"))) == "-9", "min(-5, -9) is -9");
    check(text_of(Number::max(of("-5"), of("-5"))) == "-5", "max of a tie is the value");
    check(text_of(Number::clamp(of("-12"), of("-10"), of("10"))) == "-10",
          "clamp(-12, -10, 10) is -10");
    check(text_of(Number::clamp(of("12"), of("-10"), of("10"))) == "10",
          "clamp(12, -10, 10) is 10");
    check(text_of(Number::clamp(of("3"), of("-10"), of("10"))) == "3",
          "clamp leaves a value inside the range alone");
    check(text_of(Number::clamp(of("3"), of("10"), of("-10"))) == "10",
          "clamp with low above high answers low -- number_query.cpp says so "
          "rather than leaving it undefined the way std::clamp does");

    // --- the sign of a product and a quotient -------------------------------
    //
    // DESIGN §8.6: the result is positive exactly when the operands' flags
    // agree, decided before any arithmetic runs.
    check(text_of(Number::mul(of("-3"), of("-4"))) == "12", "-3 * -4 is 12");
    check(text_of(Number::mul(of("-3"), of("4"))) == "-12", "-3 * 4 is -12");
    check(text_of(Number::mul(of("3"), of("-4"))) == "-12", "3 * -4 is -12");
    check(text_of(Number::divide(of("-12"), of("-4"), 34)) == "3", "-12 / -4 is 3");
    check(text_of(Number::divide(of("-12"), of("4"), 34)) == "-3", "-12 / 4 is -3");

    // A remainder takes the sign of the DIVIDEND, which is C's rule and not
    // Python's. DESIGN §8.6 chooses it because trunc is the free operation
    // under sign-magnitude.
    check(text_of(Number::modulo(of("-7"), of("2"))) == "-1", "-7 % 2 is -1");
    check(text_of(Number::modulo(of("7"), of("-2"))) == "1", "7 % -2 is 1");

    // --- what the unsigned magnitude fixed ----------------------------------
    //
    // v1's to_integer() came back FALSE for LLONG_MIN: BigInt::fits_ll() capped
    // the magnitude at LLONG_MAX, so the one value whose magnitude does not fit
    // a signed long long was refused -- by the same class whose constructor had
    // just accepted it and whose to_string() printed it correctly. Measured
    // against v1's own objects on 2026-08-31 before this was written down.
    long long back = 0;
    check(Number(LLONG_MIN).to_integer(back) && back == LLONG_MIN,
          "Number(LLONG_MIN) converts back to LLONG_MIN -- v1 answered `does "
          "not fit` to its own constructor's input");
    check(Number(LLONG_MAX).to_integer(back) && back == LLONG_MAX,
          "Number(LLONG_MAX) converts back to LLONG_MAX");
    check(text_of(Number(LLONG_MIN)) == "-9223372036854775808",
          "LLONG_MIN prints as itself");
    check(!of("-9223372036854775809").to_integer(back),
          "one past LLONG_MIN does not fit and says so");
    check(of("-9223372036854775808").to_integer(back) && back == LLONG_MIN,
          "LLONG_MIN reached through parse() converts too, not only through "
          "the constructor");

    // The small-form window round-trips the flag, which is what M9's inline
    // Value will hold a number as.
    bool positive = false;
    unsigned long long significand = 0;
    int exponent = 0;
    check(of("-2.5").small_parts(positive, significand, exponent),
          "-2.5 lives in the small form");
    check(!positive && significand == 25 && exponent == -1,
          "the small form carries the sign beside the magnitude, not inside it");
    check(text_of(Number::from_small(positive, significand, exponent)) == "-2.5",
          "from_small is small_parts' inverse, sign included");
    check(!boxed.small_parts(positive, significand, exponent),
          "a boxed magnitude has no small form and small_parts says so");
}

} // namespace number_test
