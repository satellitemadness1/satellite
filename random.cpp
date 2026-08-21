// satellite.random — the timer, the fold, and the watchdog. DESIGN §18.
//
// The vendored PCG headers are reached through -isystem (see the Makefile):
// pcg_extras.hpp:223 raises -Wunused-but-set-parameter under this project's
// -Wall -Wextra, and §9's from-scratch-rebuild-is-silent property depends on
// that warning not being ours to fix.

#include "random.hpp"

#include <pcg_extras.hpp>
#include <pcg_random.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <random>

namespace satellite {

namespace {

// steady_clock, NEVER high_resolution_clock. Measured on the machine §18 was
// written on: high_resolution_clock is the same type as system_clock,
// is_steady is false, and both tick at 1 ns with a 21 ns granularity — so
// there is no precision to trade away, and system_clock can be stepped
// backwards by NTP underneath a running deadline, which would end the spin
// early or not at all. steady_clock is CLOCK_MONOTONIC under libstdc++: NTP
// can slew its rate by a few hundred ppm, which moves a 2500 ms deadline by
// about a millisecond, and cannot step it at all.
using clock_type = std::chrono::steady_clock;

struct Window {
    long long minimum_ms;
    long long maximum_ms;
};

// The three tiers, indexed by RandomTier. The only thing that separates them.
constexpr Window kWindows[] = {
    {50, 100},
    {250, 300},
    {2000, 3000},
};

// The watchdog. The spin also stops if the count reaches a random value in
// [500000000, 600000000], and it is a FAILSAFE rather than part of the
// mechanism: if the clock moved under a running spin the deadline might never
// arrive, and a language builtin that can hang is not one.
//
// It never fires in ordinary operation, which is the intended behaviour for a
// watchdog. Measured: ultra draws ~249,000,000 at 2000 ms and ~310,000,000 at
// its 3000 ms ceiling, against a cap 1.6-1.9x above that; fast and normal are
// an order of magnitude further away.
//
// Belt and braces on top of a clock that already cannot hang — steady_clock is
// monotonic by the standard, and Linux does not advance CLOCK_MONOTONIC across
// suspend, so a machine suspended mid-spin resumes and finishes. The cost is
// one comparison per 1000 draws, which is why it stays.
constexpr long long CAP_LOW = 500000000LL;
constexpr long long CAP_HIGH = 600000000LL;

// Draws between deadline checks. One clock read costs more than one draw, so
// checking every iteration would measure the clock rather than the generator:
// throughput is ~153,600 draws/ms bare. It also sets the watchdog's resolution,
// which is why it is a round number rather than a tuned one.
constexpr int BATCH = 1000;

// Steps 1 to 3 of §18's mechanism: seed from kernel entropy, spin for the tier's
// window, and answer with the accumulator the spin folded its throwaways into.
//
// The SEED comes back rather than a generator, which keeps one 64 KB extension
// table alive at a time instead of moving a second one out of here.
//
// The fold is `accumulator = (accumulator + draw) / 2` — add two, divide,
// each step — and that is the author's decision, recorded with what it
// measures rather than argued with. Over six runs of a 2000 ms spin the seed
// it produces spread ~30.4 bits, against ~17.7 for the full mean and ~48.5 for
// a plain sum. The full mean COLLAPSES because the Law of Large Numbers
// concentrates it — more draws make it worse, not better — while this fold is
// an exponential moving average, so the newest draw contributes 1/2 and
// anything about 32 steps back has been shifted out entirely. It therefore
// keeps roughly one draw's worth of entropy however long the spin runs.
//
// Two things §18 records about that, on the record and not acted on here:
// dropping the divide is a one-line change worth ~18 bits, and answering
// straight from the first generator — which seed_seq_from<random_device> fills
// with about half a million bits of kernel entropy — is stronger than any fold,
// because steps 3 and 4 funnel all of it through this accumulator.
//
// uint64_t for the accumulator, not uint32_t: the sum of two 32-bit values
// overflows one, and the divide brings it back under 2^32 afterwards, so the
// intermediate is the only place the width matters.
uint64_t spun_seed(RandomTier tier)
{
    pcg_extras::seed_seq_from<std::random_device> seed_source;
    pcg32_k16384 rng(seed_source);

    const Window window = kWindows[static_cast<int>(tier)];

    // Inclusive at both ends; the +1 is what makes maximum_ms reachable.
    const long long ms =
        window.minimum_ms +
        rng(static_cast<uint32_t>(window.maximum_ms - window.minimum_ms + 1));
    const long long cap =
        CAP_LOW + rng(static_cast<uint32_t>(CAP_HIGH - CAP_LOW + 1));

    const auto deadline = clock_type::now() + std::chrono::milliseconds(ms);

    uint64_t accumulator = 0;
    long long drew = 0;
    while (drew < cap && clock_type::now() < deadline) {
        for (int i = 0; i < BATCH; i++)
            accumulator = (accumulator + rng()) / 2;
        drew += BATCH;
    }

    return accumulator;
}

// The bridge between §18's timing and bignum's sampler: 32 bits at a time, from
// a generator that spins the first time it is asked for one.
//
// LAZY on purpose. A bound the sampler refuses — a fractional one, or one wider
// than Number::MAX_RANDOM_DIGITS — is refused before any bit is drawn, and a
// call that is going to fail should not spend three seconds first.
class Draws : public Bits32 {
public:
    explicit Draws(RandomTier tier) : tier_(tier) {}

    unsigned next() override
    {
        // Step 4: the second generator, seeded from the accumulator. The
        // accumulator is a uint64_t but the divide keeps its value under 2^32,
        // so every one of the first generator's 16384 extension words has been
        // funnelled through 32 bits — which is the cost §18 records rather than
        // hides, and the reason the recommendation there is to answer from the
        // first generator instead.
        if (!rng_)
            rng_.emplace(spun_seed(tier_));
        return (*rng_)();
    }

private:
    RandomTier tier_;
    std::optional<pcg32_k16384> rng_;
};

} // namespace

bool random_tier(const std::string &word, RandomTier &out)
{
    if (word == "fast") {
        out = RandomTier::Fast;
        return true;
    }
    if (word == "normal") {
        out = RandomTier::Normal;
        return true;
    }
    if (word == "ultra") {
        out = RandomTier::Ultra;
        return true;
    }
    return false;
}

void random_window(RandomTier tier, long long &minimum_ms, long long &maximum_ms)
{
    const Window window = kWindows[static_cast<int>(tier)];
    minimum_ms = window.minimum_ms;
    maximum_ms = window.maximum_ms;
}

bool random_digits(RandomTier tier, int digits, Number &out)
{
    // 10^digits, exactly: from_small takes the internal representation, and a
    // significand of 1 at exponent `digits` IS that power of ten with no
    // multiplication and no rounding anywhere.
    Draws bits(tier);
    return Number::random_below(Number::from_small(1, digits), bits, out);
}

bool random_range(RandomTier tier, const Number &low, const Number &high,
                  Number &out)
{
    // Inclusive at both ends, and the +1 is the whole of what makes `high`
    // reachable. Without it, .range(1, 100) would answer 100 never.
    const Number span = Number::add(Number::sub(high, low), Number(1));

    Draws bits(tier);
    Number offset;
    if (!Number::random_below(span, bits, offset))
        return false;

    out = Number::add(low, offset);
    return true;
}

} // namespace satellite
