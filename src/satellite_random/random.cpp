// satellite.random -- the generator, the seam, and the spin.
//
// THE ONLY TRANSLATION UNIT THAT NAMES A PCG ENTITY. The first satellite held to
// the same rule and it is what made the vendored dependency replaceable: the
// contract is one type, one seed adaptor and four operations, so a reimplemented
// generator has one file to satisfy. pcg/README.md enumerates it.

#include "satellite_random/random.hpp"

#include <pcg_extras.hpp>
#include <pcg_random.hpp>

#include <chrono>
#include <random>

namespace satellite {

// Out of line so the vtable has one home rather than one per translation unit.
Bits32::~Bits32() = default;

namespace {

using clock_type = std::chrono::steady_clock;

// STEADY, not system_clock. A wall clock can be stepped by ntp or by a user
// while a spin is running, and a deadline computed against one that moves
// backwards does not expire. The first satellite carried a draw-count failsafe
// for exactly that; a monotonic clock removes the need for one.
static_assert(clock_type::is_steady, "the spin needs a clock that cannot go backwards");

// DESIGN §11's table. The only thing that separates the three tiers.
constexpr Window kWindows[] = {
    { 50, 300 },      // fast
    { 500, 600 },     // normal
    { 2000, 3000 },   // ultra
};

} // namespace

Window window_for(Tier tier)
{
    return kWindows[static_cast<int>(tier)];
}

// The generator, hidden behind a pointer so no header above this file has to
// include an Apache-2.0 one.
struct Source::State {
    pcg32_k16384 rng;

    // seed_seq_from fills the WHOLE 16384-word extension table -- about half a
    // million bits of kernel entropy, and about 1.6 ms. That cost is why a
    // Source is constructed once and kept, never made per draw.
    State() : rng(*seed_source()) {}
    explicit State(std::uint64_t seed) : rng(seed) {}

private:
    // A function rather than a member, because seed_seq_from is consumed by the
    // constructor and keeping one alive afterwards would hold a random_device
    // open for the life of the generator.
    static pcg_extras::seed_seq_from<std::random_device> *seed_source()
    {
        static thread_local pcg_extras::seed_seq_from<std::random_device> source;
        return &source;
    }
};

Source::Source() : state_(std::make_unique<State>()) {}
Source::Source(std::uint64_t seed) : state_(std::make_unique<State>(seed)) {}
Source::~Source() = default;

unsigned Source::next()
{
    return state_->rng();
}

long long spin(Tier tier, Bits32 &bits, void (*draw_and_discard)(Bits32 &, void *),
               void *context)
{
    const Window window = window_for(tier);

    // The duration is itself random, drawn inside the window, so the length of a
    // spin is not a constant an observer can rely on. DESIGN §11.
    //
    // Inclusive at both ends, and the +1 is what makes maximum_ms reachable.
    // Drawn from the same stream the answer will come from, which costs one word
    // and needs no second generator -- the first satellite used a second one here
    // and its own source says not to.
    const std::uint64_t span =
        static_cast<std::uint64_t>(window.maximum_ms - window.minimum_ms + 1);
    const long long milliseconds =
        window.minimum_ms + static_cast<long long>(bits.next() % span);

    const auto deadline = clock_type::now() + std::chrono::milliseconds(milliseconds);

    // DO/WHILE, and that is the whole of the "at least one" rule. A number whose
    // single draw outlasts the window still costs exactly one; a window already
    // expired on entry still costs one. A plain while() would throw nothing away
    // for a large enough request and the tier would be a lie.
    long long discarded = 0;
    do {
        draw_and_discard(bits, context);
        discarded++;
    } while (clock_type::now() < deadline);

    return discarded;
}

} // namespace satellite
