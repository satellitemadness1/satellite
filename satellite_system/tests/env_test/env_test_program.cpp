// The program as a whole: the satellite.main entry point, a program with no
// capsules in it at all, and the bound the resolver puts on its own recursion.
//
// Part of the env_test binary (satellite_system/tests/env_test/), split from a
// 546-line env_test.cpp. These are the assertions about a whole Program rather
// than about one name inside one capsule.

#include "env_test.hpp"

void env_test_entry_point()
{
    // --- satellite.main -----------------------------------------------------
    {
        ParseResult parsed = parse(
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.string> argz)\n"
            "{\n"
            "    satellite.return(argz)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "satellite.main resolves clean");
        const CapsuleInfo *info = result.find("satellite.main");
        check(info != nullptr, "the entry point is keyed satellite.main");
        check(result.find("main") == nullptr, "and not bare main");
        if (info) {
            check(info->param_count == 1 && info->slot_count == 1,
                  "satellite.main has one parameter and one slot");
            check(slot_for(slots_of(*info), "argz") == 0, "argz is slot 0");
        }
    }

    // A program with no capsules resolves to an empty table, not an error.
    {
        ParseResult parsed = parse("1 + 1\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok() && result.capsules.empty(),
              "a program with no capsules is legal");
    }
}

void env_test_recursion_bound()
{
    // --- the resolver bounds its own recursion ------------------------------
    // Without this the resolver segfaults on a deep tree exactly the way the
    // evaluator does, and it would do so before any evaluation guard could
    // possibly help.
    {
        std::string deep = "satellite.capsule f()\n{\n    satellite.return(";
        deep += std::string(3000, '-');
        deep += "1)\n}\n";
        ParseResult parsed = parse(deep);
        if (parsed.ok()) {
            ResolveResult result = resolve(parsed.program);
            check(!result.ok() && has_error(result, "nests deeper"),
                  "a deeply nested expression is an error, not a crash");
        }
    }
}
