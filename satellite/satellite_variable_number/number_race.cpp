// satellite/satellite_variable_number/number_race.cpp -- the author's M4 race:
// i = i + 1, satellite_number against signed long long int.
//
//     build/number_race <satellite-first | cxx-first> <count> <increment>
//
// prints "<satellite ns> <c++ ns> <checked c++ ns> <satellite i> <c++ i>" on stderr. The count and
// the increment come from argv, so neither loop can be folded to a constant; the
// same text reaches both sides through the same strtoll. Each loop is in its own
// function that is never inlined into main, so neither side sees the other's
// state. number_race.sh runs it seven times in each order and keeps the fastest.
//
// Nothing else is done to either loop: the C++ loop is exactly `i = i + step`,
// whatever the compiler makes of it (number_race.sh prints what that is).

#include "satellite_number.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace satellite004;

namespace {

using clock_type = std::chrono::steady_clock;

long long nanoseconds_since(clock_type::time_point start)
{
    return (long long)std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now() - start).count();
}

// Both sides count in a local and hand the answer out through a reference at the
// end, so neither loop's i is the caller's memory.
[[gnu::noinline]] void race_satellite(unsigned long long int count, signed long long int increment, satellite_number &answer,
                                      long long &ns)
{
    const clock_type::time_point start = clock_type::now();
    satellite_number i;
    const satellite_number step = satellite_number::from_signed(increment);
    for (unsigned long long int k = 0; k < count; k++)
        i = i + step;
    answer = i;
    ns = nanoseconds_since(start);
}

[[gnu::noinline]] void race_cxx(unsigned long long int count, signed long long int increment, signed long long int &answer,
                                long long &ns)
{
    const clock_type::time_point start = clock_type::now();
    signed long long int i = 0;
    const signed long long int step = increment;
    for (unsigned long long int k = 0; k < count; k++)
        i = i + step;
    answer = i;
    ns = nanoseconds_since(start);
}

// Not the author's race: C++ that, like satellite_number, refuses to wrap. The
// plain loop above compiles to one multiplication (both compilers, objdump), so it
// measures no additions at all; this one runs every addition, and is reported
// beside it so the per-addition cost can be compared. It changes nothing above.
[[gnu::noinline]] void race_cxx_checked(unsigned long long int count, signed long long int increment,
                                        signed long long int &answer, long long &ns)
{
    const clock_type::time_point start = clock_type::now();
    signed long long int i = 0;
    const signed long long int step = increment;
    for (unsigned long long int k = 0; k < count; k++)
        if (__builtin_add_overflow(i, step, &i))
            break;
    answer = i;
    ns = nanoseconds_since(start);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 4) {
        std::fprintf(stderr, "usage: number_race <satellite-first|cxx-first> <count> <increment>\n");
        return 1;
    }
    const unsigned long long int count = std::strtoull(argv[2], nullptr, 10);
    const signed long long int increment = std::strtoll(argv[3], nullptr, 10);
    long long satellite_ns = 0, cxx_ns = 0, checked_ns = 0;
    satellite_number satellite_i;
    signed long long int cxx_i = 0, checked_i = 0;
    if (std::strcmp(argv[1], "cxx-first") == 0) {
        race_cxx(count, increment, cxx_i, cxx_ns);
        race_cxx_checked(count, increment, checked_i, checked_ns);
        race_satellite(count, increment, satellite_i, satellite_ns);
    } else {
        race_satellite(count, increment, satellite_i, satellite_ns);
        race_cxx(count, increment, cxx_i, cxx_ns);
        race_cxx_checked(count, increment, checked_i, checked_ns);
    }
    std::fprintf(stderr, "%lld %lld %lld %s %lld\n", satellite_ns, cxx_ns, checked_ns, satellite_i.to_text().c_str(), cxx_i);
    return satellite_i == satellite_number::from_signed(cxx_i) && cxx_i == checked_i ? 0 : 2;
}
