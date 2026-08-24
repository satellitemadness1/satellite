// Where a name means what: lexical closure at the capsule boundary, block and
// for-loop scoping, redeclaration, shadowing, and the top level.
//
// Part of the env_test binary (satellite_system/tests/env_test/), split from a
// 546-line env_test.cpp. Three sections share this file because they are three
// halves of one question — given a name, which binding does the resolver hand
// it, and does that binding get a slot of its own.

#include "env_test.hpp"

void env_test_lexical_closure()
{
    // --- capsules are lexically closed -------------------------------------
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(nope)\n"
                  "}\n",
                  "unknown variable in capsule: nope",
                  "an unknown name inside a capsule is a resolve-time error");

    // ...but a top-level name is not, because the REPL declares on one line
    // and reads on the next — two Programs one resolve() cannot see at once.
    check_accepts("nope\n", "an unknown name at the top level is left to run time");
    check_accepts("satellite.variable.number x = 1\nx = x + 1\n",
                  "top-level declaration and use");

    // A global is still reachable from a capsule, by the four-segment path.
    check_accepts("satellite.variable.number g = 1\n"
                  "satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(satellite.library.main.g)\n"
                  "}\n",
                  "a capsule reaches a global through satellite.library");

    // The initialiser is resolved before the name is declared.
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.variable.number x = x\n"
                  "    satellite.return(x)\n"
                  "}\n",
                  "unknown variable in capsule: x",
                  "a declaration cannot read the slot it is writing");
}

void env_test_scoping()
{
    // --- scoping -----------------------------------------------------------
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    {\n"
                  "        satellite.variable.number inner = 1\n"
                  "    }\n"
                  "    satellite.return(inner)\n"
                  "}\n",
                  "unknown variable in capsule: inner",
                  "a block-scoped local does not leak out of its block");

    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.statement.for (satellite.variable.number i = 0; "
                  "i < 3; i = i + 1) {\n"
                  "        satellite.console.display(i)\n"
                  "    }\n"
                  "    satellite.return(i)\n"
                  "}\n",
                  "unknown variable in capsule: i",
                  "a for-loop variable is scoped to the loop");

    // A REDECLARATION REBINDS. It used to be an error; see the reasoning at
    // Resolver::declare in src/environment/scopes.cpp, which is that the idiom
    // it refused — build one thing, store it, reuse the name for the next — ran
    // to 341 sites in a single real program.
    check_accepts("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.variable.number x = 1\n"
                  "    satellite.variable.number x = 2\n"
                  "    satellite.return(x)\n"
                  "}\n",
                  "a redeclaration in one scope rebinds the name");

    // And the rebind goes to a FRESH SLOT, which is the half that is easy to
    // get wrong and impossible to notice. Two declarations through one name
    // must be two slots, or every reference taken to the first one silently
    // becomes a reference to the second — a list built this way would read back
    // as n copies of its last element, with no error anywhere.
    {
        ParseResult parsed = parse(
            "satellite.capsule f()\n"
            "{\n"
            "    satellite.variable.number x = 1\n"
            "    satellite.variable.number x = 2\n"
            "    satellite.return(x)\n"
            "}\n");
        check(parsed.ok(), "the two-declaration capsule parses");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "and resolves with no error");

        const CapsuleInfo *info = result.find("f");
        check(info != nullptr, "and f is in the table");
        if (info) {
            check(info->slot_count == 2,
                  "the second declaration took a slot of its own");
            check(info->slot_names ==
                      std::vector<std::string>({"x", "x"}),
                  "and both slots are recorded, under the one name");

            // The READ after both declarations sees the SECOND slot: the name
            // now denotes the newer binding, which is what "rebinds" means.
            const Slots slots = slots_of(*info);
            check(slot_for(slots, "x") == 0, "the first x is slot 0");
            check(slots.back().second == 1,
                  "and the return reads slot 1, the live binding");
        }
    }

    // A parameter shadows a top-level global of the same name: inside f, x is
    // slot 0, not satellite.library.main.x.
    {
        ParseResult parsed = parse(
            "satellite.variable.number x = 1\n"
            "satellite.capsule f(satellite.variable.number x)\n"
            "{\n"
            "    satellite.return(x)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "shadowing resolves clean");
        const CapsuleInfo *info = result.find("f");
        if (info)
            check(slot_for(slots_of(*info), "x") == 0,
                  "a parameter shadows a global of the same name");
    }
}

void env_test_top_level()
{
    // --- top level stays in satellite.library ------------------------------
    {
        ParseResult parsed = parse("satellite.variable.number top = 1\ntop\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "top-level program resolves clean");
        Slots slots;
        for (const TopLevel &item : parsed.program.items)
            if (const StmtPtr *stmt = std::get_if<StmtPtr>(&item))
                if (*stmt)
                    walk(**stmt, slots);
        check(slot_for(slots, "top") == SLOT_GLOBAL,
              "a top-level declaration stays a satellite.library global");
    }
}
