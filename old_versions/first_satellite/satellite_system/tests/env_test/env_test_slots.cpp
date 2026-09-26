// Slot allocation: how many slots a capsule gets, and which name holds which.
//
// Part of the env_test binary (satellite_system/tests/env_test/), split from a
// 546-line env_test.cpp. The harness these assertions report through is
// declared in env_test.hpp.

#include "env_test.hpp"

void env_test_slot_allocation()
{
    // --- slot allocation ---------------------------------------------------
    {
        ParseResult parsed = parse(
            "satellite.capsule f(satellite.variable.number a, "
            "satellite.variable.string b)\n"
            "{\n"
            "    satellite.variable.number c = a\n"
            "    satellite.return(c)\n"
            "}\n");
        check(parsed.ok(), "slot program parses");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "slot program resolves clean");

        const CapsuleInfo *info = result.find("f");
        check(info != nullptr, "capsule f is in the table");
        if (info) {
            check(info->param_count == 2, "f has two parameters");
            check(info->slot_count == 3, "two parameters plus one local = 3 slots");
            check(info->slot_names == std::vector<std::string>({"a", "b", "c"}),
                  "slots are named in declaration order");
            // Declared types are static, and parallel to the slots.
            check(info->slot_types.size() == 3, "one declared type per slot");
            check(info->slot_types[1].name == "string",
                  "slot 1 keeps parameter b's declared type");

            Slots slots = slots_of(*info);
            check(slot_for(slots, "a") == 0, "parameter a is slot 0");
            check(slot_for(slots, "c") == 2, "local c is slot 2");
        }
    }

    // Locals declared in the body follow the parameters, never overlap them.
    {
        ParseResult parsed = parse(
            "satellite.capsule f(satellite.variable.number p)\n"
            "{\n"
            "    satellite.variable.number x = 1\n"
            "    satellite.variable.number y = 2\n"
            "    satellite.return(p)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        const CapsuleInfo *info = result.find("f");
        check(info && info->slot_count == 3, "one parameter and two locals");
        if (info) {
            Slots slots = slots_of(*info);
            check(slot_for(slots, "x") == 1 && slot_for(slots, "y") == 2,
                  "locals take slots after the parameters");
        }
    }
}
