// Spacesuit tests: satellite.variable.time — part of the spacesuit_test binary.
//
// An instant is read off the clock, never written down, and the difference of
// two of them is a plain number of nanoseconds (§8.2). These cases live with
// the spacesuit tests because satellite.variable.time is the first type whose
// methods are reached the same way an object's are.

#include "spacesuit_test.hpp"

void spacesuit_test_time()
{
    // --- satellite.variable.time --------------------------------------------
    // An instant is read off the clock, never written down, and the difference
    // of two is a plain number of nanoseconds (§8.2).
    check_output("satellite.variable.time t = satellite.time.now()\n"
                 "satellite.console.display(t.minus(t))\n",
                 "0\n", "an instant minus itself is zero nanoseconds");

    check_output("satellite.variable.time epoch\n"
                 "satellite.console.display(epoch)\n",
                 "1970-01-01T00:00:00.000000000Z\n",
                 "an uninitialised time is the epoch");

    check_output("satellite.variable.time epoch\n"
                 "satellite.console.display(epoch.nanoseconds())\n",
                 "0\n", "nanoseconds() is the count since the epoch");

    check_output("satellite.variable.time a = satellite.time.now()\n"
                 "satellite.variable.time b = satellite.time.now()\n"
                 "satellite.console.display(b.minus(a) >= 0)\n",
                 "true\n", "the clock does not run backwards within a program");

    check_error("satellite.variable.time t = 1\n", "cannot initialise",
                "a time is not a number");

    check_error("satellite.variable.time t = satellite.time.now()\n"
                "satellite.console.display(t.minus(1))\n",
                "wants a satellite.variable.time",
                "minus takes another instant");

    check_error("satellite.time.now(1)\n", "takes 0 arguments",
                "satellite.time.now takes no arguments");
}
