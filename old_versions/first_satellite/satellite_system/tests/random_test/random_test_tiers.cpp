// The tier half of the satellite.random tests: the three tier names, the three
// throwaway windows behind them, and the two call shapes checked end to end
// against a clock.
//
// Part of satellite_system/tests/random_test/, split from a 375-line
// random_test.cpp. See random_test.hpp for what the pieces share.
//
// Everything here runs on `fast`, whose window is 50-100 ms, and it draws a
// handful of times rather than in a loop. That is deliberate and is the reason
// the sampler is checked separately: a uniformity claim needs hundreds of
// thousands of draws, and hundreds of thousands of real tier draws would be
// hours. Do not raise a count in this file, and do not reach for a shorter tier
// to make room for one — there isn't a shorter tier, and the floor asserted
// below is the whole point.

#include "random_test.hpp"

#include "satellite_number/bignum.hpp"
#include "random_numbers/random.hpp"

#include <chrono>
#include <string>

using namespace satellite;

// --- the tiers --------------------------------------------------------------

void test_tiers()
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

void test_spin()
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
