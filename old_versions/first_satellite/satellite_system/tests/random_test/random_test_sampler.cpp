// The sampler half of the satellite.random tests: every draw inside its
// interval, both ends reachable, the distribution flat, and a bound that cannot
// be drawn from refused before anything is drawn.
//
// Part of satellite_system/tests/random_test/, split from a 375-line
// random_test.cpp. See random_test.hpp for what the pieces share.
//
// This is the half that runs on a stub generator rather than on a tier, so the
// counts here can be in the hundreds of thousands and still cost milliseconds.
// The two generators below are why, and they stay file-local: no other section
// draws bits, and a test generator visible to the whole binary would be an
// invitation to check a TIER against one, which would measure nothing.

#include "random_test.hpp"

#include "satellite_number/bignum.hpp"

#include <cstdint>
#include <string>

using namespace satellite;

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

void test_bounds()
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

void test_uniform()
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

void test_leading_zeros()
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

void test_refusals()
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
