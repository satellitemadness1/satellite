// Calls between capsules: forward calls, mutual and self recursion, a capsule
// name in call position, and static arity.
//
// Part of the env_test binary (satellite_system/tests/env_test/), split from a
// 546-line env_test.cpp. This group is the argument for the resolver existing
// as a pass of its own, so it is worth keeping in one place: every assertion
// here is about something a single-pass parser could not have answered.

#include "env_test.hpp"

void env_test_forward_calls()
{
    // --- what only a separate pass can do ----------------------------------
    // A forward call and mutual recursion are the whole reason this is not in
    // the parser: neither is resolvable in single-pass recursive descent.
    check_accepts("satellite.capsule first(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(second(n))\n"
                  "}\n"
                  "satellite.capsule second(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(n)\n"
                  "}\n",
                  "a capsule may call one defined further down");

    check_accepts("satellite.capsule is_even(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(is_odd(n))\n"
                  "}\n"
                  "satellite.capsule is_odd(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(is_even(n))\n"
                  "}\n",
                  "mutual recursion resolves");

    check_accepts("satellite.capsule fact(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(fact(n - 1))\n"
                  "}\n",
                  "self recursion resolves");

    // A capsule name in call position is marked as such, not read as a
    // variable.
    {
        ParseResult parsed = parse(
            "satellite.capsule helper()\n"
            "{\n"
            "    satellite.return(satellite)\n"
            "}\n"
            "satellite.capsule caller()\n"
            "{\n"
            "    satellite.return(helper())\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "call program resolves clean");
        const CapsuleInfo *info = result.find("caller");
        if (info)
            check(slot_for(slots_of(*info), "helper") == SLOT_CAPSULE,
                  "a called capsule is SLOT_CAPSULE, not a variable");
    }

    // Arity is knowable before anything runs.
    check_rejects("satellite.capsule f(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(n)\n"
                  "}\n"
                  "satellite.capsule g()\n"
                  "{\n"
                  "    satellite.return(f(1, 2))\n"
                  "}\n",
                  "takes 1 argument, got 2", "arity is checked statically");
}
