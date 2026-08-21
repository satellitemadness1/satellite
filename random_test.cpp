// satellite.random tests — DESIGN §18.
//
// Split in two, because the two halves cost different amounts of time. The
// SAMPLER is driven by a stub generator that does not spin at all, so its
// distribution can be measured over hundreds of thousands of draws in
// milliseconds; that is where the correctness lives, and it is the half a
// uniformity claim has to be checked in. The TIERS are then exercised end to
// end a handful of times on `fast`, whose throwaway window is 50-100 ms, so
// this binary stays under a second.
//
// Nothing here asserts a particular VALUE, because there is no particular value
// to assert. What is asserted is what §18 actually promises: every draw lands
// inside its interval, both ends of the interval are reachable, the
// distribution is flat, and a call takes at least as long as its tier says.

#include "bignum.hpp"
#include "interp.hpp"
#include "random.hpp"

#include <chrono>
#include <cstdint>
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

// A generator that does NOT spin, so the sampler can be measured without paying
// a tier's throwaway window per draw. splitmix32: deterministic from its seed,
// so a failure here reproduces exactly rather than being a distribution that
// was unlucky once.
class Stub : public Bits32 {
public:
    explicit Stub(uint32_t seed) : state_(seed) {}

    unsigned next() override
    {
        state_ += 0x9e3779b9u;
        uint32_t z = state_;
        z = (z ^ (z >> 16)) * 0x21f0aaadu;
        z = (z ^ (z >> 15)) * 0x735a2d97u;
        return z ^ (z >> 15);
    }

private:
    uint32_t state_;
};

// A generator stuck on one value, for the paths where what matters is that the
// sampler asks for more bits rather than what it does with them.
class Fixed : public Bits32 {
public:
    explicit Fixed(unsigned value) : value_(value) {}
    unsigned next() override { return value_; }

private:
    unsigned value_;
};

static Number number(const char *text)
{
    Number out;
    if (!Number::parse(text, out))
        check(false, std::string("the test's own literal did not parse: ") + text);
    return out;
}

// --- the sampler ------------------------------------------------------------

static void test_bounds()
{
    Stub bits(1);

    // [0, 1) has exactly one member, and a draw over it is not an error — it is
    // the answer 0. satellite.random.fast(0) is the surface that reaches here.
    Number out;
    check(Number::random_below(number("1"), bits, out) && out.is_zero(),
          "a bound of 1 draws 0");

    // Every draw inside the interval, and BOTH ends reachable. The low end is
    // easy to get right by accident; the high end is the one an off-by-one
    // costs, so it is checked by looking for it.
    bool saw_low = false;
    bool saw_high = false;
    bool inside = true;
    for (int i = 0; i < 20000; i++) {
        Number drawn;
        if (!Number::random_below(number("100"), bits, drawn)) {
            inside = false;
            break;
        }
        long long value = 0;
        if (!drawn.to_integer(value) || value < 0 || value > 99)
            inside = false;
        if (value == 0)
            saw_low = true;
        if (value == 99)
            saw_high = true;
    }
    check(inside, "20000 draws below 100 all landed in [0, 100)");
    check(saw_low, "0 is reachable");
    check(saw_high, "99 is reachable — the bound is exclusive, not off by one");
}

static void test_uniform()
{
    // Three buckets. The interesting property is not that it is roughly flat
    // but that it STAYS flat: `%` on its own skews toward the low residues
    // whenever the bound does not divide the generator's range, and §18
    // measured that as 1398410 / 904307 / 697283 against a flat 1000000.
    Stub bits(7);
    const int rounds = 300000;
    long long bucket[3] = {0, 0, 0};
    for (int i = 0; i < rounds; i++) {
        Number drawn;
        Number::random_below(number("3"), bits, drawn);
        long long value = 0;
        drawn.to_integer(value);
        if (value >= 0 && value < 3)
            bucket[value]++;
    }
    const long long expected = rounds / 3;
    const long long tolerance = expected / 50;   // 2%
    for (int i = 0; i < 3; i++) {
        const long long off = bucket[i] - expected;
        check(off <= tolerance && -off <= tolerance,
              "bucket " + std::to_string(i) + " is within 2% of flat (got " +
                  std::to_string(bucket[i]) + ", want " +
                  std::to_string(expected) + ")");
    }

    // A bound that spans more than one limb, so the top-limb draw and the
    // rejection both do work. 2e9 is two limbs with a top limb of 2, which is
    // the shape with the most rejection the sampler can produce short of a top
    // limb of 1.
    Stub wide(11);
    long long half = 0;
    bool inside = true;
    const Number bound = number("2000000000");
    for (int i = 0; i < 100000; i++) {
        Number drawn;
        Number::random_below(bound, wide, drawn);
        long long value = 0;
        if (!drawn.to_integer(value) || value < 0 || value >= 2000000000LL)
            inside = false;
        if (value >= 1000000000LL)
            half++;
    }
    check(inside, "100000 two-limb draws all landed in [0, 2e9)");
    check(half > 49000 && half < 51000,
          "the top half of a two-limb bound is drawn about half the time (got " +
              std::to_string(half) + "/100000)");
}

static void test_leading_zeros()
{
    // §18 says this out loud rather than rounding it away: `ultra(40)` is
    // uniform over [0, 10^40), so about one draw in ten has a leading zero and
    // therefore RENDERS as 39 digits or fewer. It is what uniform means, and a
    // draw that always printed 40 digits would not be one.
    Stub bits(3);
    const Number bound = number("1e40");
    int short_ones = 0;
    const int rounds = 20000;
    for (int i = 0; i < rounds; i++) {
        Number drawn;
        Number::random_below(bound, bits, drawn);
        if (drawn.to_string().size() < 40)
            short_ones++;
    }
    check(short_ones > rounds / 15 && short_ones < rounds / 6,
          "about a tenth of 40-digit draws render shorter (got " +
              std::to_string(short_ones) + "/" + std::to_string(rounds) + ")");
}

static void test_refusals()
{
    Fixed bits(0);
    Number out;

    check(!Number::random_below(number("0"), bits, out),
          "a bound of 0 is refused — [0, 0) has nothing in it");
    check(!Number::random_below(number("-5"), bits, out),
          "a negative bound is refused");
    check(!Number::random_below(number("2.5"), bits, out),
          "a fractional bound is refused");

    // The ceiling is on the DRAW, so the widest legal bound is 10^MAX, whose
    // values are all at most MAX digits. One past it is refused, and refused
    // WITHOUT expanding: `1e2000000000` is twelve characters of source and two
    // billion digits of value.
    Stub real(5);
    const std::string at = "1e" + std::to_string(Number::MAX_RANDOM_DIGITS);
    const std::string past = "1e" + std::to_string(Number::MAX_RANDOM_DIGITS + 1);
    check(Number::random_below(number(at.c_str()), real, out),
          "a bound of 10^MAX_RANDOM_DIGITS is accepted");
    check(out.to_string().size() <=
              static_cast<size_t>(Number::MAX_RANDOM_DIGITS),
          "and the draw it produces is no wider than the ceiling");
    check(!Number::random_below(number(past.c_str()), real, out),
          "one digit past the ceiling is refused");
    check(!Number::random_below(number("1e2000000000"), real, out),
          "a two-billion-digit bound is refused rather than attempted");
}

// --- the tiers --------------------------------------------------------------

static void test_tiers()
{
    RandomTier tier = RandomTier::Ultra;
    check(random_tier("fast", tier) && tier == RandomTier::Fast, "fast names a tier");
    check(random_tier("normal", tier) && tier == RandomTier::Normal, "normal names a tier");
    check(random_tier("ultra", tier) && tier == RandomTier::Ultra, "ultra names a tier");
    check(!random_tier("quick", tier), "quick is not a tier");
    check(!random_tier("range", tier), "range is not a tier either");

    // The three windows, from the code rather than from this comment. §18 is
    // where the numbers are argued; this is where they are checked.
    long long low = 0;
    long long high = 0;
    random_window(RandomTier::Fast, low, high);
    check(low == 50 && high == 100, "fast throws away for 50-100 ms");
    random_window(RandomTier::Normal, low, high);
    check(low == 250 && high == 300, "normal throws away for 250-300 ms");
    random_window(RandomTier::Ultra, low, high);
    check(low == 2000 && high == 3000, "ultra throws away for 2000-3000 ms");
}

static void test_spin()
{
    // The tier is a promise about TIME, so it is checked against a clock. Only
    // the floor is asserted: the ceiling is a machine's to miss under load, and
    // a test that fails when the build box is busy is a test nobody trusts.
    //
    // steady_clock here for the same reason random.cpp uses it to measure the
    // spin: this is a duration, and system_clock's epoch can move under one.
    const auto started = std::chrono::steady_clock::now();
    Number drawn;
    check(random_digits(RandomTier::Fast, 12, drawn), "a fast draw succeeds");
    const auto took = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - started).count();
    check(took >= 50, "a fast draw spends at least its 50 ms floor (took " +
                          std::to_string(took) + " ms)");

    long long value = 0;
    check(drawn.to_integer(value) && value >= 0 && value < 1000000000000LL,
          "a 12-digit draw lands in [0, 10^12)");

    // Inclusive at both ends, so a range of one is legal and has one answer.
    Number one;
    check(random_range(RandomTier::Fast, Number(7), Number(7), one) &&
              Number::compare(one, Number(7)) == 0,
          ".range(7, 7) is the number 7");

    // Negative bounds are ordinary. The draw is min + offset, and nothing in
    // that needs the interval to be positive.
    Number below;
    check(random_range(RandomTier::Fast, Number(-100), Number(-90), below),
          "a negative range draws");
    long long got = 0;
    check(below.to_integer(got) && got >= -100 && got <= -90,
          "and lands inside it (got " + std::to_string(got) + ")");
}

// --- the module surface -----------------------------------------------------

static int ns_counter = 0;

static std::string fresh_ns()
{
    return "r" + std::to_string(++ns_counter);
}

static void check_output(const std::string &source, const std::string &want,
                         const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(), want.c_str(),
               result.output.c_str());
        failures++;
    }
}

static void check_error(const std::string &source, const std::string &fragment,
                        const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.ok || result.output.find(fragment) == std::string::npos) {
        printf("FAIL: %s\n  want error containing: %s\n  got: %s\n",
               what.c_str(), fragment.c_str(), result.output.c_str());
        failures++;
    }
}

static void test_surface()
{
    // The two cases with exactly one right answer, which are the only two a
    // random module can be checked against by value.
    check_output("satellite.console.display(satellite.random.fast(0))\n", "0\n",
                 "fast(0) draws from a one-member interval");
    check_output("satellite.console.display(satellite.random.fast.range(5, 5))\n",
                 "5\n", "range(5, 5) is 5");

    // Arity is per PATH, and `ultra` and `ultra.range` are different paths with
    // different arities — which is the whole reason §18 spells the second form
    // as a fourth segment (format.def).
    check_error("satellite.random.fast()\n", "takes 1 argument, got 0",
                "the digit form takes one argument");
    check_error("satellite.random.fast(1, 2)\n", "takes 1 argument, got 2",
                "and not two");
    check_error("satellite.random.fast.range(1)\n",
                "satellite.random.fast.range takes 2 arguments, got 1",
                "the range form takes two");

    check_error("satellite.random.fast(\"x\")\n", "wants a whole number of digits",
                "a digit count is a number");
    check_error("satellite.random.fast(1.5)\n", "wants a whole number of digits",
                "and a whole one");
    check_error("satellite.random.fast(0 - 1)\n", "draws between 0 and",
                "a negative digit count is refused");
    check_error("satellite.random.fast(100001)\n", "draws between 0 and",
                "and so is one past the ceiling");

    check_error("satellite.random.fast.range(1.5, 2)\n", "wants whole numbers",
                "a range wants whole bounds");
    check_error("satellite.random.fast.range(100, 1)\n", "is empty",
                "a backwards range is empty, and says so");

    // A tier nobody wrote is not a tier, and the error is the ordinary one for
    // a path the module surface does not know.
    check_error("satellite.random.quick(4)\n",
                "no such module function: satellite.random.quick",
                "there are three tiers and quick is not one");
    check_error("satellite.random.fast.middle(1, 2)\n", "no such module function",
                "and one tail segment, which is range");

    // Without the call parentheses it is a path, not a value — the same answer
    // satellite.console.display gives, and deliberately NOT the module-constant
    // treatment satellite.bool.true gets.
    check_error("satellite.variable.number x = satellite.random.ultra\n",
                "is a module path, not a value",
                "a tier is not a value on its own");
}

int main()
{
    test_bounds();
    test_uniform();
    test_leading_zeros();
    test_refusals();
    test_tiers();
    test_spin();
    test_surface();

    if (failures) {
        printf("FAILURES: %d\n", failures);
        return 1;
    }
    printf("PASS: random (the sampler unbiased over one and two limbs and flat "
           "to 2%% in 300000 draws; both ends of an interval reachable; a "
           "fractional, negative or over-wide bound refused without drawing; "
           "three tiers, two shapes each, and the 50 ms floor met on the "
           "clock)\n");
    return 0;
}
