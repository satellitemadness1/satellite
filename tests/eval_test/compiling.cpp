// Every kind of node DESIGN §6 can produce becomes an op or a NAMED REFUSAL.
//
// THIS IS THE SECTION THAT KEEPS THE MILESTONE HONEST ABOUT ITS OWN EDGES. The
// grammar is wider than this evaluator and will be until M28: a subscript, a
// spacesuit, an include and `satellite` used as a value all parse today.
// errors.def's S0720 block note is the argument for saying so with a code, a
// caret and a milestone number -- "between now and M28 every milestone ships a
// language whose grammar is wider than its evaluator", and DESIGN §1.1 says
// never to do anything behind the user's back, the gap included.
//
// SO EVERY REFUSAL BELOW IS AN ASSERTION THAT M9 ADMITS WHAT IT CANNOT DO. The
// first satellite's answer to the same situation was a segfault or a silently
// wrong value.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/closure.hpp"
#include "evaluator/evaluator_internal.hpp"

#include <string>

namespace eval_test {

namespace {

// Whether a program compiles to something holding an op_refuse, and what it
// says when it is reached.
std::string refusal_in(const std::string &source, const std::string &capsule)
{
    Run run;
    build(source, run);
    if (!run.built)
        return "<did not compile>";
    call(run, capsule, {});
    if (last_problems.empty())
        return "<ran without complaint>";
    return satellite::errors::sentence(last_problems.front());
}

std::string body(const std::string &statement)
{
    return "satellite.capsule it()\n{\n    " + statement + "\n}\n";
}

} // namespace

void section_compile()
{
    using namespace satellite;

    // --- every arm has a name, which is what `satl --compile` prints ---------
    //
    // A -Wswitch WOULD HAVE GIVEN THIS FOR FREE AND op_name CANNOT BE A SWITCH,
    // because its key is a function ADDRESS. So the check is here instead: an
    // arm added to evaluator_internal.hpp without a row in op_name reads "?" in
    // every listing, silently.
    {
        const eval::OpFn kArms[] = {
            eval::op_no_op,    eval::op_constant,     eval::op_local,
            eval::op_global,   eval::op_unary,        eval::op_binary,
            eval::op_call,     eval::op_enter,        eval::op_dispatch,
            eval::op_refuse,   eval::op_block,        eval::op_expression,
            eval::op_store,    eval::op_store_global, eval::op_return,
            eval::op_if,       eval::op_while,        eval::op_for,
        };
        for (const eval::OpFn arm : kArms)
            check(std::string(eval::op_name(arm)) != "?",
                  "every op function has a name for `satl --compile` to print");
        check(std::string(eval::op_name(nullptr)) == "?",
              "and something that is not an arm reads as unknown rather than "
              "as whichever row happened to be last");
    }

    // --- a Value is 40 bytes, and it is DESIGN §8.2's budget -----------------
    //
    // THE static_assert IN value.hpp IS THE REAL CHECK and this is the one a
    // person reads. PLAN §6.1 calls sizeof(Number) "the one number that could
    // make this port not fit"; it is 32, the discriminator is 8, and there is
    // nothing to spare.
    check(sizeof(Value) == 40 || sizeof(void *) != 8,
          "DESIGN §8.2: a Value is 40 bytes on 64-bit");
    check(sizeof(eval::Op) == 24,
          "an Op is five words -- the same shape as the ast.hpp Node it came "
          "from, with a function pointer where the kind was");

    // --- what parses and does not run yet ------------------------------------
    check(holds(refusal_in(body("satellite.return(satellite)"), "it"), "M10"),
          "S0720: `satellite` as a value names M10, which brings the singleton "
          "and `satellite.return(satellite)`");

    check(holds(refusal_in("satellite.capsule it()\n{\n"
                           "    satellite.variable.number n = 1\n"
                           "    satellite.return(n[0])\n}\n",
                           "it"),
                "M16"),
          "S0720: a subscript names M16, which brings the containers");

    check(holds(refusal_in(body("satellite.return(x00FF)"), "it"),
                "no milestone"),
          "S0720: a hexadecimal literal says that NO milestone owns its value "
          "-- M3 lexes it, DESIGN §8.5 specifies it, and PLAN §8 gives it to "
          "neither M11 nor M16. A gap named is a gap somebody can close");

    // --- a language path with no handler behind it ---------------------------
    {
        Run run;
        build(body("satellite.console.display(\"x\")"), run);
        check(run.built, "a console call compiles");
        call(run, "it", {});
        check(ran_into(errors::Code::EVAL_NO_HANDLER),
              "S0721: `satellite.console.display` has a NUMBER and no handler "
              "-- PLAN §8's ledger gives M9 none of the 223 paths, because this "
              "milestone builds the table every other row dispatches through");
    }

    // --- a global, which is the one thing at the top level that runs ---------
    {
        Run run;
        build("satellite.library.total = 6 * 7\n"
              "satellite.capsule it()\n{\n"
              "    satellite.return(satellite.library.total)\n}\n",
              run);
        check(run.built, "a global with an initialiser compiles");
        check(answer_of(run, "it", {}) == "42",
              "DESIGN §7.2: `satellite.library` is shared state, its "
              "initialiser runs before any capsule, and a capsule can read it");
    }

    // --- the compiler keeps its own stack ------------------------------------
    //
    // THE FIFTH WALK IN THIS TREE TO DO SO, and the first that was born that
    // way rather than rewritten. M8.5 fixed four; DESIGN §7.5 has no exception
    // for a pass that runs once, and a program that PARSES at 100,000 deep and
    // then segfaults being COMPILED would have moved that crash rather than
    // removed it. This binary links no machine_limits, so this is 8 MiB.
    {
        std::string deep = "satellite.capsule it()\n{\n    satellite.return(1";
        for (int i = 1; i < 100000; i++)
            deep += " + 1";
        deep += ")\n}\n";

        Run run;
        build(deep, run);
        check(run.built,
              "a 100,000-deep expression COMPILES at 8 MiB of C++ stack -- the "
              "compiler is iterative for DESIGN §7.5's reason, like the four "
              "walks M8.5 rewrote");
    }
}

} // namespace eval_test
