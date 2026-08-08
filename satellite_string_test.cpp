// Code-table, round-trip, and system-char tests for satellite_string.

#include "satellite_string.hpp"
#include "system.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

int main()
{
    // Table order: 0 void, a-z, A-Z, 0-9, punctuation ending in `~.
    check(encode("a") == SatString{1}, "a is 1");
    check(encode("z") == SatString{26}, "z is 26");
    check(encode("A") == SatString{27}, "A is 27");
    check(encode("Z") == SatString{52}, "Z is 52");
    check(encode("0") == SatString{53}, "0 is 53");
    check(encode("9") == SatString{62}, "9 is 62");
    check(encode("!") == SatString{63}, "! is 63");
    check(encode("~") == SatString{94}, "~ is 94");
    check(encode("hello!") == (SatString{8, 5, 12, 12, 15, 63}), "hello!");

    // Full round trip, including a raw-area char (space).
    const std::string text = "Hello, World 42! [a-z]/`~";
    check(decode(encode(text)) == text, "round trip");

    // Void decodes to nothing.
    check(decode(encode("\\void")).empty(), "void is nothing");

    // System chars expand to live machine facts.
    check(decode(encode("\\user")) == username(), "\\user");
    check(decode(encode("\\home")) == home_dir(), "\\home");
    check(decode(encode("\\cwd")) == cwd(), "\\cwd");
    check(decode(encode("\\threads")) == std::to_string(hardware_threads()),
          "\\threads");
    check(!decode(encode("\\memtotal")).empty(), "\\memtotal");
    check(!decode(encode("\\memused")).empty(), "\\memused");

    check(mem_total_mb() > 0, "mem_total_mb > 0");
    check(mem_available_mb() > 0, "mem_available_mb > 0");
    check(mem_used_mb() > 0, "mem_used_mb > 0");

    if (failures)
        return 1;
    printf("PASS: satellite_string table + system chars "
           "(user=%s threads=%u total=%luMB used=%luMB)\n",
           username().c_str(), hardware_threads(), mem_total_mb(),
           mem_used_mb());
    return 0;
}
