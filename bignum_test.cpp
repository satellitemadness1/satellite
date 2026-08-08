// Number tests: §8.1's exact arbitrary-precision decimal.
//
// The cases that matter are the ones a double got WRONG, because those are the
// whole reason for the migration: 0.1 + 0.2, a trillion printed whole, 10^20 + 1
// keeping its 1, and a factorial that runs past 64 bits. The rendering cases are
// §8.1.1's rule — print the value — checked at both notation boundaries.

#include "bignum.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

static Number num(const std::string &text)
{
    Number out;
    if (!Number::parse(text, out)) {
        printf("FAIL: cannot parse %s\n", text.c_str());
        failures++;
    }
    return out;
}

// `a <op> b` renders as `want`.
static void check_op(const std::string &a, const char *op, const std::string &b,
                     const std::string &want)
{
    const Number x = num(a);
    const Number y = num(b);
    Number got;
    switch (*op) {
    case '+': got = Number::add(x, y); break;
    case '-': got = Number::sub(x, y); break;
    case '*': got = Number::mul(x, y); break;
    case '/': got = Number::divide(x, y, Number::DEFAULT_DIVISION_DIGITS); break;
    case '%': got = Number::modulo(x, y); break;
    default: check(false, "bad op"); return;
    }
    if (got.to_string() != want) {
        printf("FAIL: %s %s %s\n  want: %s\n  got:  %s\n", a.c_str(), op,
               b.c_str(), want.c_str(), got.to_string().c_str());
        failures++;
    }
}

static void check_render(const std::string &text, const std::string &want)
{
    const std::string got = num(text).to_string();
    if (got != want) {
        printf("FAIL: %s renders as %s, want %s\n", text.c_str(), got.c_str(),
               want.c_str());
        failures++;
    }
}

int main()
{
    // --- the layout §8.1 promised -------------------------------------------
    // 32 bytes is what keeps sizeof(ValueBase) at 40 with SatString's 32 still
    // dominating, which is what made replacing the double free.
    check(sizeof(Number) == 32, "sizeof(Number) is 32");

    // --- what a double got wrong --------------------------------------------
    check_op("0.1", "+", "0.2", "0.3");
    check_op("2000000", "+", "1", "2000001");
    check_op("1000000", "*", "1000000", "1000000000000");
    check_render("123456789", "123456789");
    check_render("3.14", "3.14");
    check_render("0.1", "0.1");

    // 10^20 + 1. A double's 53-bit mantissa cannot hold this at all; the 1 is
    // simply gone. Alignment to the lower exponent is what keeps it.
    check_op("100000000000000000000", "+", "1", "100000000000000000001");

    // ...and the same in the other direction, across 40 orders of magnitude.
    check_op("1e20", "+", "1e-20",
             "100000000000000000000.00000000000000000001");

    // --- arbitrary precision means arbitrary --------------------------------
    {
        // 30!, which is 33 digits and runs past 64 bits at 21!.
        Number factorial = num("1");
        for (int i = 2; i <= 30; i++)
            factorial = Number::mul(factorial, Number(i));
        check(factorial.to_string() == "265252859812191058636308480000000",
              "30! is exact");
    }
    {
        // 2^200, checked against its decimal expansion.
        Number power = num("1");
        for (int i = 0; i < 200; i++)
            power = Number::add(power, power);
        check(power.to_string() ==
                  "1606938044258990275541962092341162602522202993782792835301376",
              "2^200 is exact");
    }

    // --- division -----------------------------------------------------------
    // Exact when it terminates, and every division by a power of ten does,
    // which is what makes nanoseconds-to-seconds exact.
    check_op("10", "/", "4", "2.5");
    check_op("2788516221", "/", "1000000000", "2.788516221");
    check_op("1", "/", "8", "0.125");
    check_op("-10", "/", "4", "-2.5");

    // Rounded when it does not, to division_digits significant figures.
    check_op("1", "/", "3", "0.3333333333333333333333333333333333");
    check_op("2", "/", "3", "0.6666666666666666666666666666666667");

    {
        // A different precision is a different number of digits, and nothing
        // else changes.
        const Number third = Number::divide(num("1"), num("3"), 5);
        check(third.to_string() == "0.33333", "division_digits is honoured");
    }

    // --- modulo keeps the dividend's sign, as fmod does ---------------------
    check_op("7", "%", "2", "1");
    check_op("-7", "%", "2", "-1");
    check_op("7.5", "%", "2", "1.5");

    // --- rounding -----------------------------------------------------------
    check(num("3.5").floor().to_string() == "3", "floor(3.5)");
    check(num("3.5").ceil().to_string() == "4", "ceil(3.5)");
    check(num("3.5").round().to_string() == "4", "round(3.5)");
    check(num("-2.5").floor().to_string() == "-3", "floor(-2.5) is -3");
    check(num("-2.5").ceil().to_string() == "-2", "ceil(-2.5) is -2");
    check(num("-2.5").round().to_string() == "-3", "round(-2.5) is half away");
    check(num("-4").abs().to_string() == "4", "abs(-4)");
    check(num("7").floor().to_string() == "7", "floor of a whole number");

    // --- how a number renders, §8.1.1 ---------------------------------------
    // Scientific only when the padding zeros outnumber the information, which
    // is the old [1e-6, 1e21) boundary restated for a type whose digits are
    // exact: 20 trailing zeros and 5 leading ones.
    check_render("0.000001", "0.000001");
    check_render("0.0000001", "1e-7");
    check_render("100000000000000000000", "100000000000000000000");   // 1e20
    check_render("1e21", "1e+21");
    check_render("1.5e22", "1.5e+22");
    // ...but a long EXACT integer prints whole, because its digits are
    // information rather than padding. Under a double it had none to print.
    check_render("1606938044258990275541962092341162602522202993782792835301376",
                 "1606938044258990275541962092341162602522202993782792835301376");
    check_render("-0", "0");
    check_render("0.0", "0");
    check_render("2.50", "2.5");
    check_render("00012", "12");

    // --- value equality, not representation equality ------------------------
    // 1 is `sig 1 exp 0` and 1.0 is `sig 10 exp -1`. A defaulted member-wise
    // operator== would call them different, and Value is a std::variant that
    // would have inherited the mistake.
    check(num("1") == num("1.0"), "1 == 1.0");
    check(num("1") == num("1.000000"), "1 == 1.000000");
    check(!(num("1") == num("1.0000001")), "1 != 1.0000001");
    check(Number::compare(num("2.5"), num("3")) < 0, "2.5 < 3");
    check(Number::compare(num("-1"), num("-2")) > 0, "-1 > -2");
    check(Number::compare(num("0"), num("-0")) == 0, "0 == -0");
    check(Number::compare(num("1e20"), num("1e-20")) > 0, "1e20 > 1e-20");

    // --- integers -----------------------------------------------------------
    {
        long long value = 0;
        check(num("42").to_integer(value) && value == 42, "42 is an integer");
        check(num("42.0").to_integer(value) && value == 42, "42.0 is 42");
        check(!num("42.5").to_integer(value), "42.5 is not an integer");
        check(num("-7").to_integer(value) && value == -7, "-7 is an integer");
        check(!num("1e40").to_integer(value), "1e40 does not fit a long long");
        check(num("1e18").to_integer(value) && value == 1000000000000000000LL,
              "1e18 fits");
    }
    check(num("42").is_integer(), "42 is_integer");
    check(!num("0.5").is_integer(), "0.5 is not is_integer");

    // --- parsing rejects what it should -------------------------------------
    {
        Number ignored;
        check(!Number::parse("", ignored), "empty is not a number");
        check(!Number::parse("abc", ignored), "abc is not a number");
        check(!Number::parse("1.2.3", ignored), "1.2.3 is not a number");
        check(!Number::parse("1e", ignored), "1e is not a number");
        check(!Number::parse("12 ", ignored), "trailing space is not a number");
        check(Number::parse("-0.5", ignored), "-0.5 is a number");
        check(Number::parse("1E+3", ignored), "1E+3 is a number");
    }

    // --- the integral constructor -------------------------------------------
    // §8.1's trap 2: the constrained template is what keeps Number(42) working
    // while `Value v = 3.14` fails to compile. The deleted float overloads are
    // not testable here for the obvious reason — they would not compile.
    check(Number(42).to_string() == "42", "Number(42)");
    check(Number(-42).to_string() == "-42", "Number(-42)");
    check(Number(0).to_string() == "0", "Number(0)");
    check(Number(LLONG_MIN).to_string() == "-9223372036854775808",
          "Number(LLONG_MIN) does not overflow on negation");
    check(Number(ULLONG_MAX).to_string() == "18446744073709551615",
          "Number(ULLONG_MAX) goes big rather than wrapping");
    check(Number::add(Number(LLONG_MAX), Number(1)).to_string() ==
              "9223372036854775808",
          "long long overflow promotes instead of wrapping");
    check(Number::mul(Number(LLONG_MAX), Number(LLONG_MAX)).to_string() ==
              "85070591730234615847396907784232501249",
          "so does multiplication");

    if (failures == 0)
        printf("PASS: bignum (exact decimal arithmetic, arbitrary precision, "
               "§8.1.1 rendering; sizeof(Number)=%zu)\n",
               sizeof(Number));
    else
        printf("bignum_test: FAIL (%d)\n", failures);
    return failures ? 1 : 0;
}
