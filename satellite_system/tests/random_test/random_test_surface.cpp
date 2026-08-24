// The module surface of satellite.random as a satellite program sees it: the
// two calls with exactly one right answer, and the errors every wrong spelling
// of a call has to produce.
//
// Part of satellite_system/tests/random_test/, split from a 375-line
// random_test.cpp. See random_test.hpp for what the pieces share.
//
// The checks are all check_output/check_error, which run real source through
// the interpreter; both are defined in random_test.cpp alongside the namespace
// counter that keeps one case's variables out of the next case's scope.

#include "random_test.hpp"

#include <string>

void test_surface()
{
    // The two cases with exactly one right answer, which are the only two a
    // random module can be checked against by value.
    check_output("satellite.console.display(satellite.random.fast(0))\n", "0\n",
                 "fast(0) draws from a one-member interval");
    check_output("satellite.console.display(satellite.random.fast.range(5, 5))\n",
                 "5\n", "range(5, 5) is 5");

    // Arity is per PATH, and `ultra` and `ultra.range` are different paths with
    // different arities — which is the whole reason §18 spells the second form
    // as a fourth segment (format.def).
    check_error("satellite.random.fast()\n", "takes 1 argument, got 0",
                "the digit form takes one argument");
    check_error("satellite.random.fast(1, 2)\n", "takes 1 argument, got 2",
                "and not two");
    check_error("satellite.random.fast.range(1)\n",
                "satellite.random.fast.range takes 2 arguments, got 1",
                "the range form takes two");

    check_error("satellite.random.fast(\"x\")\n", "wants a whole number of digits",
                "a digit count is a number");
    check_error("satellite.random.fast(1.5)\n", "wants a whole number of digits",
                "and a whole one");
    check_error("satellite.random.fast(0 - 1)\n", "draws between 0 and",
                "a negative digit count is refused");
    check_error("satellite.random.fast(100001)\n", "draws between 0 and",
                "and so is one past the ceiling");

    check_error("satellite.random.fast.range(1.5, 2)\n", "wants whole numbers",
                "a range wants whole bounds");
    check_error("satellite.random.fast.range(100, 1)\n", "is empty",
                "a backwards range is empty, and says so");

    // A tier nobody wrote is not a tier, and the error is the ordinary one for
    // a path the module surface does not know.
    check_error("satellite.random.quick(4)\n",
                "no such module function: satellite.random.quick",
                "there are three tiers and quick is not one");
    check_error("satellite.random.fast.middle(1, 2)\n", "no such module function",
                "and one tail segment, which is range");

    // Without the call parentheses it is a path, not a value — the same answer
    // satellite.console.display gives, and deliberately NOT the module-constant
    // treatment satellite.bool.true gets.
    check_error("satellite.variable.number x = satellite.random.ultra\n",
                "is a module path, not a value",
                "a tier is not a value on its own");
}
