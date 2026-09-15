// The three shapes, and the refusals, through a compiled program. See
// tests/help_test/help_test.hpp.
//
// THE SWEEP IN FIXTURE 4 IS THE DONE-WHEN'S SECOND SELF-VERIFYING CHECK, and
// it is a sweep rather than the one example PLAN names because one example is
// what passed by accident before this milestone existed:
// `satellite.help(satellite.network)` refused in plain words on 2026-09-07,
// with S0721, because the ARGUMENT dispatched and died -- and it refused
// identically for `satellite.help(satellite.console)`, which is built. Asking
// every node, and asserting the CODE and not merely the refusal, is what makes
// the check unable to pass that way again.

#include "help_test.hpp"

#include "error_reporter/codes.hpp"
#include "satellite_help/built.hpp"
#include "satellite_help/help_text.hpp"
#include "satellite_help/render.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace help_test {

using namespace satellite;

namespace {

const char *kPrologue = "satellite.include(satellite)\n"
                        "\n"
                        "satellite.capsule satellite.main()\n"
                        "{\n";

std::string program(const std::string &body)
{
    return kPrologue + ("    " + body) + "\n}\n";
}

} // namespace

void section_walking()
{
    install_every_module();
    bool complained = false;
    int code = 0;

    // 1 -- THE DONE-WHEN'S OWN PROGRAM: a whole body of `satellite.help`.
    const std::string bare = run(program("satellite.help"), &complained, &code);
    check(!complained, "a program whose whole body is `satellite.help` runs");
    check(holds(bare, "satellite.console"),
          "and it names the console, which is built");
    check(holds(bare, "satellite.help"),
          "and it names help itself, which this milestone builds");
    check(!holds(bare, "satellite.network"),
          "and it does not name the network, which nothing implements");
    check(!holds(bare, "satellite.returns"),
          "and it does not offer `satellite.returns`, which is built and is "
          "not how a capsule is written here");
    // THIS CLAUSE WAS WRITTEN THE OTHER WAY ROUND AT M18 AND IS THE TEST DOING
    // ITS JOB. It read `!holds(bare, "satellite.file")` -- "it does not name
    // M19's file, which is the next milestone and not this one" -- and M19
    // landing is exactly what makes that false. A listing that still omitted
    // `satellite.file` the day files were built would be help drifting from
    // what exists, which is the one thing DESIGN §4.6 exists to prevent, so
    // this failing was the correct outcome and flipping it is the milestone
    // being recorded rather than the test being loosened.
    check(holds(bare, "satellite.file"),
          "and it names M19's file, which is built now");
    check(holds(bare, "satellite.directory"),
          "and the directory beside it");

    // 2 -- AND THE PARENTHESES ARE OPTIONAL. PLAN: "bare `satellite.help` and
    // `satellite.help()` start at the root", which is one walk and not two
    // routines, so the two answers must be the same byte for byte.
    const std::string called =
        run(program("satellite.help()"), &complained, &code);
    check(!complained, "`satellite.help()` runs too");
    check(called == bare,
          "and answers exactly what the bare word does -- one walk, not two");

    // 3 -- AND SO IS ASKING ABOUT THE ROOT, which is the same walk started at
    // the node the other two start at by default. If this ever diverges, the
    // root has become a special case and the "one walk at three depths" claim
    // has quietly stopped being true.
    const std::string root =
        run(program("satellite.help(satellite)"), &complained, &code);
    check(root == bare,
          "and so does asking about `satellite` itself -- the root is not a "
          "special case");

    // 4 -- EVERY NODE, ASKED ABOUT BY ITS OWN QUERY. Built ones answer, unbuilt
    // ones refuse with S1101, and nothing refuses with S0721 -- which is the
    // code that would mean the argument had been evaluated after all.
    const help::BuiltSet built = help::BuiltSet::now();
    size_t answered = 0;
    size_t refused = 0;
    for (words::PathId i = 1; i <= words::kNodeCount; i++) {
        if (help::head_of(i) != i)
            continue;   // one ask per group; the members share the answer
        const std::string ask =
            run(program("satellite.help(" + help::query_text(i) + ")"),
                &complained, &code);
        const std::string named = help::query_text(i);
        if (built.contains(i)) {
            answered++;
            check(!complained, "a built path answers: " + named);
            check(complained || holds(ask, named),
                  "and the answer names the path it was asked about: " + named);
        } else {
            refused++;
            check(complained, "an unbuilt path refuses: " + named);
            check(code == static_cast<int>(errors::Code::HELP_NOT_BUILT),
                  "and refuses for being unbuilt rather than for dying on the "
                  "way in -- S1101 and never S0721: " + named);
        }
    }
    check(answered > 40 && refused > 40,
          "the sweep reached both halves of the language -- 118 groups answered "
          "and 65 refused at M18");

    // 5 -- THE ARGUMENT IS NOT EVALUATED, SAID THE OTHER WAY ROUND. A module
    // that is built refuses S0721 when it is READ as a value, and answers when
    // it is ASKED about. Both lines are in one program, so a compiler that
    // started visiting the topic argument would fail this and nothing else.
    run(program("satellite.console"), &complained, &code);
    check(complained && code == static_cast<int>(errors::Code::EVAL_NO_HANDLER),
          "`satellite.console` read as a value still refuses with S0721");
    run(program("satellite.help(satellite.console)"), &complained, &code);
    check(!complained,
          "and the same path, asked about, answers -- which is the whole of "
          "what an unevaluated argument buys");

    // 6 -- A VARIABLE ANSWERS FOR ITS TYPE, and no value is consulted to do it:
    // resolve knew the declared type statically, so the fold happened at
    // compile time. The name is never even given a value here, which is what
    // makes that visible.
    const std::string typed =
        run(kPrologue + std::string("    satellite.variable.string ship\n"
                                    "    satellite.help(ship)\n}\n"),
            &complained, &code);
    check(!complained, "`satellite.help(x)` on a declared name runs");
    check(holds(typed, "satellite.variable.string"),
          "and answers for the name's declared type, with no value in it");

    // 7 -- AND THE THREE MISUSES. A bare word nobody declared, and everything
    // else the grammar allows between the parentheses.
    run(program("satellite.help(random)"), &complained, &code);
    check(complained && code == static_cast<int>(errors::Code::HELP_NO_SUCH_NAME),
          "a bare word gets S1102 and not resolve's S0511 -- it is a topic "
          "somebody left the `satellite.` off, not a variable being read");
    run(program("satellite.help(\"random\")"), &complained, &code);
    check(complained && code == static_cast<int>(errors::Code::HELP_NOT_A_TOPIC),
          "a quoted topic gets S1103 -- quotes are gone, and a path is how a "
          "topic is reached");
    run(program("satellite.help(1 + 1)"), &complained, &code);
    check(complained && code == static_cast<int>(errors::Code::HELP_NOT_A_TOPIC),
          "and so does an expression, which parses and means nothing here");

    // 8 -- AND A HELP CALL INSIDE ANOTHER EXPRESSION, which is a real shape and
    // is NOT the guard it was written to be. It was added while chasing a
    // mutation -- `visit_reversed` restored for the topic argument -- on the
    // theory that the visited argument's leftover result would be handed to the
    // enclosing call. It is not: the leftover sits BELOW the answer on the
    // compiler's results stack and nothing ever reads it, so the mutation stays
    // invisible and this clause passes either way. It is kept because a help
    // call in expression position is worth having a fixture for, and the
    // comment is kept because a clause whose stated purpose is wrong is worse
    // than no clause. MILESTONES/M18.md §3.
    const std::string nested =
        run(program("satellite.console.display(satellite.help(satellite.bool))"),
            &complained, &code);
    check(!complained, "a help call nested in another expression compiles");
    check(holds(nested, "satellite.bool"),
          "and answers about its own topic");
    check(holds(nested, "nothing"),
          "and the call around it gets help's answer -- `nothing` -- rather "
          "than the op left over from an argument that was never taken");

    // 9 -- AND A GLOBAL, WHICH IS THE ONE THAT WAS WRONG. A global is a
    // variable with no declared type -- `satellite.library.n = 0` is the whole
    // of the form and S0204 refuses a typed one -- so help cannot answer
    // without running the program. It got S1103 until this was probed, which
    // told somebody `n` was not the name of a variable while they were looking
    // at the line that declares it.
    run("satellite.include(satellite)\n\nsatellite.library.n = 0\n\n"
        "satellite.capsule satellite.main()\n{\n"
        "    satellite.help(satellite.library.n)\n}\n",
        &complained, &code);
    check(complained &&
              code == static_cast<int>(errors::Code::HELP_GLOBAL_HAS_NO_TYPE),
          "a global gets S1104, which says why rather than saying it is not a "
          "variable");
}

} // namespace help_test
