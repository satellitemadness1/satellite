// satellite/satellite_random/random_spin.cpp -- the throwaway. random_spin.hpp says whose
// design it is and why a do/while.

#include "random_spin.hpp"

#include "random_draw.hpp"

#include <chrono>

namespace satellite004 {

namespace {

using clock_type = std::chrono::steady_clock;
static_assert(clock_type::is_steady, "the spin needs a clock that cannot go backwards");

} // namespace

long long int spin(RandomTier tier, LimbSource &limbs, void (*discard)(void *), void *context)
{
    const SpinWindow window = spin_window(tier);

    // HOW LONG: uniform over the window, both ends in -- the + 1 is what makes most_ms
    // reachable -- by the same unbiased draw every answer takes (random_draw.hpp).
    const satellite_number span(static_cast<unsigned long long int>(window.most_ms - window.least_ms + 1));
    const long long int milliseconds = window.least_ms + static_cast<long long int>(random_below(limbs, span).limb(0));
    const clock_type::time_point deadline = clock_type::now() + std::chrono::milliseconds(milliseconds);

    long long int thrown = 0;
    do {
        discard(context);
        ++thrown;
    } while (clock_type::now() < deadline);
    return thrown;
}

} // namespace satellite004
