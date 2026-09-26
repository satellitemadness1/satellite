// Prototype for satellite.random -- the three tiers of DESIGN's random work.
//
// Each tier seeds pcg32_k16384 from real kernel entropy, throws draws away for
// a randomised span of milliseconds, and then draws its answer from the SAME
// generator. There is no reseeding step: the seed_seq_from<random_device>
// below fills all 16384 extension words, and reseeding from a single 32-bit
// draw would funnel every one of them through 32 bits.

#include "../pcg-cpp-0.98/include/pcg_extras.hpp"
#include "../pcg-cpp-0.98/include/pcg_random.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>

// steady_clock, NOT high_resolution_clock: the latter is an alias for
// system_clock here, which NTP can step backwards underneath a running
// deadline. Both measure to 21ns, so there is nothing to trade away.
using clock_type = std::chrono::steady_clock;

// A random duration in [minimum_ms, maximum_ms], inclusive at both ends.
// The +1 is what makes maximum_ms reachable.
static long long random_ms(pcg32_k16384 &rng, long long minimum_ms, long long maximum_ms)
{
    return minimum_ms
         + rng(static_cast<uint32_t>(maximum_ms - minimum_ms + 1));
}

// Throw draws away until `ms` have passed. Returns how many went in the bin.
static long long spin_for(pcg32_k16384 &rng, long long ms)
{
    const auto deadline = clock_type::now() + std::chrono::milliseconds(ms);

    long long thrown = 0;
    while (clock_type::now() < deadline) {
        // Batched: one clock read costs more than a draw, so checking the
        // time every iteration would measure the clock instead of the RNG.
        for (int i = 0; i < 1000; ++i) rng();
        thrown += 1000;
    }
    return thrown;
}

// One tier. `minimum_ms`/`maximum_ms` are what separate fast, normal and ultra.
static uint32_t tier(long long minimum_ms, long long maximum_ms,
                     const char *name)
{
    pcg_extras::seed_seq_from<std::random_device> seed_source;
    pcg32_k16384 rng(seed_source);

    const long long ms = random_ms(rng, minimum_ms, maximum_ms);

    const auto started = clock_type::now();
    const long long thrown = spin_for(rng, ms);
    const long long took = std::chrono::duration_cast<std::chrono::milliseconds>(
                               clock_type::now() - started).count();

    const uint32_t answer = rng();

    printf("  %-7s picked %4lld ms in [%lld, %lld], spun %4lld ms, "
           "threw away %-12lld -> %u\n",
           name, ms, minimum_ms, maximum_ms, took, thrown, answer);
    return answer;
}

int main()
{
    printf("satellite.random tiers\n");
    tier(50, 100, "fast");
    tier(250, 300, "normal");
    tier(2000, 3000, "ultra");
    return 0;
}
