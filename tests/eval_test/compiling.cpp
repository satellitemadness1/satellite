// Every kind of node DESIGN §6 can produce becomes an op or a NAMED REFUSAL.
//
// THIS IS THE SECTION THAT KEEPS THE MILESTONE HONEST ABOUT ITS OWN EDGES. The
// grammar is wider than this evaluator and will be until M28: a subscript, a
// spacesuit and an include of a spaceship all parse today.
//
// errors.def's S0720 block note is the argument for saying so with a code, a
// caret and a milestone number -- "between now and M28 every milestone ships a
// language whose grammar is wider than its evaluator", and DESIGN §1.1 says
// never to do anything behind the user's back, the gap included.
//
// SO EVERY REFUSAL BELOW IS AN ASSERTION THAT THIS EVALUATOR ADMITS WHAT IT
// CANNOT DO. The first satellite's answer to the same situation was a segfault
// or a silently wrong value.
//
// AND ONE OF THEM STOPPED BEING ONE AT M10, WHICH IS WHAT THE LIST IS FOR.
// `satellite` used as a value was in the sentence above and in a fixture below,
// refusing with M10's number in it; M10 gave it a producer, so that fixture now
// asserts the ANSWER instead. A refusal naming a milestone is a test that
// milestone deletes -- which is the only thing that keeps the list honest.

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

    // --- `satellite` IS A VALUE, WHICH IS M10's FIRST HALF --------------------
    //
    // DESIGN §8's type table has always had a row for it -- "`satellite` | the
    // runtime singleton | `satellite.return(satellite)`" -- and §3 says what it
    // means: "the singleton runtime object, NOT A ZERO SENTINEL ... return the
    // runtime (that is, success)". Until M10 this compiled to a refusal naming
    // M10, and the whole of the change is a fifth arm on the variant with a
    // producer in compile_expressions.cpp.
    {
        Run run;
        build(body("satellite.return(satellite)"), run);
        check(run.built, "a capsule returning `satellite` compiles");
        check(answer_of(run, "it", {}) == "satellite",
              "and answers the runtime singleton, which renders as the word the "
              "program wrote rather than as blank or as `nothing`");
    }

    // AND IT IS NOT `nothing`, WHICH IS THE ARM IT WOULD HAVE COLLAPSED INTO.
    // Two empty structs in one variant look like waste until a program can tell
    // them apart: a capsule that answered `satellite` said it succeeded and a
    // capsule that answered nothing said nothing, and since M12 the language
    // can ASK -- `holding` answers "satellite" or "nothing". value.hpp's
    // Runtime note is the argument.
    {
        Run run;
        build("satellite.capsule ran()\n{\n    satellite.return(satellite)\n}\n"
              "satellite.capsule quiet()\n{\n    satellite.return()\n}\n",
              run);
        check(run.built, "both return shapes compile");
        check(answer_of(run, "ran", {}) == "satellite" &&
                  answer_of(run, "quiet", {}) == "nothing",
              "`satellite.return(satellite)` `1 15 1` and `satellite.return()` "
              "`1 15 0` are two answers and not one");
    }

    // --- what parses and does not run yet ------------------------------------

    // A SUBSCRIPT NAMED M16 HERE UNTIL 2026-09-06 and now compiles, so what
    // this fixture asks has moved one step along: `n[0]` on a NUMBER is a
    // wrong question rather than an unbuilt one, and the sentence says which
    // things a `[` can ask about instead of naming a milestone.
    //
    // THREE BECAME FOUR AT M20, which is this fixture doing its job from the
    // direction it was not pointed in. It pins the SENTENCE, so a milestone
    // that adds a fourth indexable thing and forgets to say so in the refusal
    // fails here -- and M20's arguments object is indexable, because its
    // command line is what `[i]` reads.
    check(holds(refusal_in("satellite.capsule it()\n{\n"
                           "    satellite.variable.number n = 1\n"
                           "    satellite.return(n[0])\n}\n",
                           "it"),
                "the four things a `[` can ask about"),
          "S0713: a subscript on a number names what CAN be indexed, and the "
          "arguments object made that list four long at M20");

    // A GAP NAMED IS A GAP SOMEBODY CAN CLOSE, AND M19.5 CLOSED ALL OF IT.
    // This fixture has now said three things in three days, which is why the
    // history is written here rather than deleted with each rewrite: until
    // 2026-09-08 it asked that a hexadecimal literal say NO milestone owned
    // its value; on 2026-09-08 that it name M19.5's second half; and since
    // 2026-09-09 there is no refusal to ask for, because the value exists.
    //
    // SO WHAT IS PINNED HERE IS THAT S0720 IS NOT REACHABLE FROM A LITERAL AT
    // ALL, which is PLAN §8's done-when for this milestone in its own words:
    // "`satl --compile` no longer has S0720 in reach for a literal, because
    // there is a producer." tests/eval_test/hex.cpp is where the VALUE is
    // tested; this is the gap's headstone.
    {
        Run run;
        build(body("satellite.return(x00FF)"), run);
        check(run.built,
              "S0720 IS NO LONGER IN REACH FOR A LITERAL -- every literal this "
              "language lexes now compiles to a value, which is what closed "
              "M19.5 on 2026-09-09");
    }

    // --- a language path with no handler behind it ---------------------------
    {
        Run run;
        build(body("satellite.console.display(\"x\")"), run);
        check(run.built, "a console call compiles");
        call(run, "it", {});
        // A PATH WITH A NUMBER AND NOTHING BEHIND IT, AND AT M10 THIS BINARY
        // IS WHY RATHER THAN THE MILESTONE. `satellite.console.display` `1 5 1`
        // has a real handler in `satl` now -- satellite_console/handlers.cpp,
        // the first row the table has ever held. This suite does not link that
        // module, deliberately: it links no machine_limits either, so the depth
        // fixtures run against the 8 MiB a login shell hands out (M8.5 §4.1),
        // and a console started by a test binary would print into the test's
        // own output. tests/console_test is where the row is checked.
        //
        // WHICH LEAVES THIS FIXTURE CHECKING THE SHAPE OF THE GAP, and that is
        // still worth an assertion: every milestone from here to M28 ships
        // paths the table has no row for, and S0721 is what one of them says.
        check(ran_into(errors::Code::EVAL_NO_HANDLER),
              "S0721: a path with a number and no handler in THIS binary is "
              "refused in words rather than crashing or answering nothing");
    }

    // --- the place parameter's two misuses, caught at compile ---------------
    //
    // M14's `input(prompt, target)` `1 5 4` -- words.def's one place row.
    // BOTH REFUSALS FIRE IN THIS BINARY, WHICH LINKS NO CONSOLE: op_misuse is
    // the compiler's op, raised before any dispatch could happen, and that is
    // done-when clause 5's "fails BEFORE the prompt prints" as a link-time
    // fact -- there is no prompt machinery here to have printed.
    {
        Run run;
        build(body("satellite.console.input(\"q? \", 3)"), run);
        check(run.built, "a non-place target still compiles -- a mistake in a "
                         "branch that never runs is a program that runs");
        call(run, "it", {});
        check(ran_into(errors::Code::CONSOLE_TARGET_NOT_A_PLACE),
              "S1002: the place must name a variable, refused with no console "
              "anywhere in this binary");
    }
    {
        Run run;
        build("satellite.capsule it()\n{\n"
              "    satellite.variable.string s = \"\"\n"
              "    satellite.variable.string t = "
              "satellite.console.input(\"q? \", s)\n"
              "    satellite.return(t)\n}\n",
              run);
        check(run.built, "an assigned place call still compiles");
        call(run, "it", {});
        check(ran_into(errors::Code::CONSOLE_ANSWER_IS_THE_PLACE),
              "S1003: `1 5 4` writes a place and yields nothing, so a "
              "position that could read its answer is refused");
    }

    // --- the three includes, and only one of them is a refusal --------------
    //
    // M17 OWNS TWO OF THESE ROWS AND M25 OWNS THE THIRD, which is the whole of
    // why they are checked together: the boundary is invisible from inside any
    // one of them. `include(satellite)` "is the one include form that does
    // nothing" (DESIGN §3) and `include()` does nothing for WORD_NUMBERS
    // §1.3's reason -- "0 means nothing in that position" -- so a program that
    // asked for nothing to be included got exactly what it asked for. Only
    // `include(spaceship)` names something this evaluator cannot load.
    {
        Run run;
        build("satellite.include()\n"
              "satellite.capsule it()\n{\n    satellite.return(7)\n}\n",
              run);
        check(run.built, "satellite.include() compiles");
        check(answer_of(run, "it", {}) == "7",
              "and it compiles to NO OP AT ALL -- the capsule beside it runs, "
              "which is the only way to see that nothing was emitted");
    }

    {
        Run run;
        build("satellite.include(satellite)\n"
              "satellite.capsule it()\n{\n    satellite.return(7)\n}\n",
              run);
        check(run.built, "satellite.include(satellite) compiles");
        check(answer_of(run, "it", {}) == "7", "and it too is no op at all");
    }

    // A SPACESHIP IS THE ONE THAT REFUSES, IN ALL THREE OF ITS SPELLINGS. The
    // bare identifier is the spelling both DESIGN §3 and WORD_NUMBERS §2.2
    // use, and it was the one that did NOT reach this refusal until M17 -- it
    // was diverted into scope lookup at resolve and came back as S0511,
    // "nothing called `cargo` is in scope here". Checking all three is what
    // says the boundary is drawn round the FORM and not round the way it
    // happens to be written.
    for (const char *const spelling :
         {"cargo", "\"cargo\"", "satellite.console"}) {
        Run run;
        build(std::string("satellite.include(") + spelling + ")\n" +
                  "satellite.capsule it()\n{\n    satellite.return(7)\n}\n",
              run);
        check(run.built,
              std::string("satellite.include(") + spelling + ") compiles");
        call(run, "it", {});
        check(ran_into(errors::Code::EVAL_NOT_BUILT),
              std::string("and S0720 refuses it naming M25, for `") + spelling +
                  "` as for the other two");
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
