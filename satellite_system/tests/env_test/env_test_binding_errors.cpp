// Names the resolver refuses to bind: the reserved word at a binding site, and
// duplicates of a capsule or of a parameter.
//
// Part of the env_test binary (satellite_system/tests/env_test/), split from a
// 546-line env_test.cpp. Both sections are about a DECLARATION being rejected,
// as against the unknown-name errors in env_test_scoping.cpp, which are about
// a use that finds nothing to bind to.

#include "env_test.hpp"

void env_test_reserved_word()
{
    // --- the reserved word, at every binding site --------------------------
    check_rejects("satellite.capsule f(satellite.variable.number satellite)\n"
                  "{\n"
                  "    satellite.return(1)\n"
                  "}\n",
                  "cannot name a parameter",
                  "satellite is reserved as a parameter name too");
}

void env_test_duplicates()
{
    // --- duplicates ---------------------------------------------------------
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(1)\n"
                  "}\n"
                  "satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(2)\n"
                  "}\n",
                  "already defined", "a duplicate capsule is rejected");

    check_rejects("satellite.capsule f(satellite.variable.number n, "
                  "satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(n)\n"
                  "}\n",
                  "duplicate parameter n", "a duplicate parameter is rejected");

    // A failed parameter still occupies its slot, so later parameters keep
    // lining up with the arguments that will fill them.
    {
        ParseResult parsed = parse(
            "satellite.capsule f(satellite.variable.number n, "
            "satellite.variable.number n, satellite.variable.number z)\n"
            "{\n"
            "    satellite.return(z)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        const CapsuleInfo *info = result.find("f");
        check(info && info->param_count == 3, "arity counts every parameter");
        if (info)
            check(slot_for(slots_of(*info), "z") == 2,
                  "a duplicate parameter does not shift the ones after it");
    }
}
