// The harness and the entry point for the satellite.random test binary.
//
// Part of satellite_system/tests/random_test/, split from a 375-line
// random_test.cpp. See random_test.hpp for what the pieces share and why the
// seams fall where they do; the sections themselves are in
// random_test_sampler.cpp, random_test_tiers.cpp and random_test_surface.cpp.
//
// What lives here is everything that must exist exactly once: the failure
// counter, the three check functions that write to it, and the main() that
// turns it into an exit code.

// Quoted and unqualified, unlike every other include in this folder: -Isrc
// makes the module headers root-relative, and this header is not under src/ —
// it is the sibling file the compiler finds next to the one including it.
#include "random_test.hpp"

#include "interpreter/interp.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// --- the module surface -----------------------------------------------------

static int ns_counter = 0;

static std::string fresh_ns()
{
    return "r" + std::to_string(++ns_counter);
}

void check_output(const std::string &source, const std::string &want,
                  const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(), want.c_str(),
               result.output.c_str());
        failures++;
    }
}

void check_error(const std::string &source, const std::string &fragment,
                 const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.ok || result.output.find(fragment) == std::string::npos) {
        printf("FAIL: %s\n  want error containing: %s\n  got: %s\n",
               what.c_str(), fragment.c_str(), result.output.c_str());
        failures++;
    }
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
