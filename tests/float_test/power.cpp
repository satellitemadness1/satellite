// power and sqrt -- DESIGN §8.6's class 3, where "rounding is required for an
// answer to exist at all". The exact rows of power's table are asserted
// exact; the irrational rows are asserted against digits computed by hand
// (and checkable by hand: every expected string here is the mathematical
// value rounded half away from zero at the stated places).

#include "float_test.hpp"

namespace float_test {

using satellite::Float;
using satellite::PowerOutcome;

namespace {

std::string power_text(const std::string &base, const std::string &exponent,
                       unsigned digits)
{
    Float out;
    const PowerOutcome outcome =
        power_of(number_of(base), number_of(exponent), digits, out);
    if (outcome != PowerOutcome::Answered)
        return outcome == PowerOutcome::DividedByZero ? "(divided by zero)"
                                                      : "(no real answer)";
    return text_of(out);
}

std::string sqrt_text(const std::string &value, unsigned digits)
{
    Float out;
    const PowerOutcome outcome = sqrt_of(number_of(value), digits, out);
    if (outcome != PowerOutcome::Answered)
        return "(no real answer)";
    return text_of(out);
}

} // namespace

void section_power()
{
    // --- power's table, row one: integer >= 0 is EXACT ---------------------
    check(power_text("2", "3", 34) == "8.0", "2^3 is exactly 8, as a float");
    check(power_text("2.5", "2", 34) == "6.25", "a fractional base stays exact");
    check(power_text("-2", "3", 34) == "-8.0", "an odd power keeps the sign");
    check(power_text("-2", "2", 34) == "4.0", "an even power drops it");
    check(power_text("7", "0", 34) == "1.0", "x^0 is 1");
    check(power_text("0", "0", 34) == "1.0", "0^0 is 1 -- the empty product, "
                                             "decided at M15");
    check(power_text("0", "5", 34) == "0.0", "0 to a positive power is 0");
    check(power_text("2", "100", 34) == "1267650600228229401496703205376.0",
          "the growth is real and the left half carries it exactly");

    // --- row two: integer < 0 is the reciprocal and rounds correctly -------
    check(power_text("2", "-2", 34) == "0.25", "2^-2 is exactly 0.25");
    check(power_text("-2", "-3", 34) == "-0.125", "the sign survives the "
                                                  "reciprocal");
    check(power_text("3", "-1", 5) == "0.33333", "1/3 rounds at float_digits");
    check(power_text("0", "-1", 34) == "(divided by zero)",
          "0 to a negative power is 1 over 0 -- S0601's fact");

    // --- row three: fractional MUST round, and refuses a negative base ----
    check(power_text("4", "0.5", 5) == "2.0", "4^0.5 lands exactly on 2");
    check(power_text("2", "0.5", 5) == "1.41421", "2^0.5 at five places");
    check(power_text("10", "2.5", 4) == "316.2278",
          "an integer part and a fractional part compose");
    check(power_text("0.37", "4.65", 6) == "0.009821",
          "QUAD's rack exponent shape -- pow(urgency, exp) -- answers");
    check(power_text("2", "-0.5", 5) == "0.70711",
          "a negative fractional exponent is the reciprocal of the root");
    check(power_text("-8", "0.5", 34) == "(no real answer)",
          "a negative base with a fractional exponent is refused whole");
    check(power_text("-32", "0.2", 34) == "(no real answer)",
          "even the case with a real answer -- the refusal is the rule, not "
          "a gap; MILESTONES/M15.md section 2 records why");

    // --- sqrt: Newton, then the last place settled exactly -----------------
    check(sqrt_text("6.25", 34) == "2.5", "sqrt(6.25) is exactly 2.5");
    check(sqrt_text("2", 5) == "1.41421", "sqrt(2) at five places");
    check(sqrt_text("2", 34) == "1.4142135623730950488016887242096981",
          "and at the default -- the 34th place of sqrt(2), half away");
    check(sqrt_text("0", 34) == "0.0", "sqrt(0) is the positive zero");
    check(sqrt_text("0.0001", 34) == "0.01", "a small square is exact");
    check(sqrt_text("1e40", 34) == "100000000000000000000.0",
          "a large square is exact and renders its digits");
    check(sqrt_text("-1", 34) == "(no real answer)", "S0602's fact");

    // POWER AND SQRT AGREE WHERE THEY OVERLAP, which is §8.6's argument for
    // one rule: two roads to one value must not answer two values.
    check(power_text("2", "0.5", 34) == sqrt_text("2", 34),
          "x^0.5 and sqrt(x) answer the same digits");
}

} // namespace float_test
