// Frames, arguments, returns -- and DESIGN §7.1's defect, checked from the
// side it was invisible from.
//
// THE FIRST SATELLITE'S WORST VERIFIED DEFECT IS WHAT THIS FILE IS ABOUT.
// resolve.hpp carries the receipt: with one global registry keyed by
// "<capsule>.<variable>", a recursive `fact` returned 1 for EVERY input, and
// eight threads running a capsule with no recursion and no shared state
// produced 1585 wrong results out of 1600. M7 gave every name a slot; nothing
// could check that the slots were per-ACTIVATION until there were activations,
// and this milestone is where they arrive. `fact(5) == 120` is that check.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"

#include <string>

namespace eval_test {

namespace {

const char *kFactorial = R"(
satellite.capsule fact(satellite.variable.number n)
{
    satellite.statement.if (n < 2)
    {
        satellite.return(1)
    }
    satellite.return(n * fact(n - 1))
}
)";

// Mutual recursion, which is what DESIGN §7.3's four passes exist for and what
// a compiler that emitted a call as it met one could not do.
const char *kEvenOdd = R"(
satellite.capsule is_even(satellite.variable.number n)
{
    satellite.statement.if (n == 0)
    {
        satellite.return(1)
    }
    satellite.return(is_odd(n - 1))
}

satellite.capsule is_odd(satellite.variable.number n)
{
    satellite.statement.if (n == 0)
    {
        satellite.return(0)
    }
    satellite.return(is_even(n - 1))
}
)";

} // namespace

void section_calls()
{
    using namespace satellite;

    // --- the recursive capsule the first satellite got wrong -----------------
    {
        Run run;
        build(kFactorial, run);
        check(run.built, "the factorial fixture compiles");

        check(answer_of(run, "fact", {1}) == "1", "fact(1)");
        check(answer_of(run, "fact", {5}) == "120",
              "DESIGN §7.1: fact(5) is 120, and the first satellite answered 1 "
              "for every input because one registry held one `n`");
        check(answer_of(run, "fact", {20}) == "2432902008176640000",
              "fact(20), which is exact and does not fit a double");

        // §8.1's ARBITRARY PRECISION REACHED FROM A PROGRAM. 100! is 158 digits
        // and every one of them is right; M8 built the arithmetic and this is
        // the first thing that can ASK for it in satellite.
        check(holds(answer_of(run, "fact", {100}), "9.33262154439441526816"),
              "fact(100) is exact to 158 digits");
    }

    // --- a call to a capsule declared FURTHER DOWN the file ------------------
    //
    // DESIGN §7.3's forward reference, which is why resolve runs in four passes
    // and why the compiler gives every capsule an index before it compiles any
    // body. A compiler that emitted a call as it met one would have nothing to
    // point half of these at.
    {
        Run run;
        build(kEvenOdd, run);
        check(run.built, "mutual recursion compiles");
        check(answer_of(run, "is_even", {10}) == "1", "10 is even");
        check(answer_of(run, "is_even", {7}) == "0", "7 is not");
        check(answer_of(run, "is_odd", {7}) == "1", "and 7 is odd");
    }

    // --- what a capsule answers when it says nothing -------------------------
    {
        Run run;
        build("satellite.capsule quiet(satellite.variable.number n)\n"
              "{\n"
              "    satellite.variable.number unused = n\n"
              "}\n"
              "satellite.capsule bare(satellite.variable.number n)\n"
              "{\n"
              "    satellite.return()\n"
              "}\n",
              run);
        check(run.built, "a capsule with no return compiles");
        check(answer_of(run, "quiet", {1}) == "nothing",
              "falling off the end of a body answers nothing -- and it PRINTS "
              "as a word, because a capsule that returned nothing and one that "
              "returned the empty string must not look the same");
        check(answer_of(run, "bare", {1}) == "nothing",
              "and a bare `satellite.return()` is the same answer");
    }

    // --- parameters are the frame's first slots, in order --------------------
    {
        Run run;
        build("satellite.capsule pick(satellite.variable.number a, "
              "satellite.variable.number b, satellite.variable.number c)\n"
              "{\n"
              "    satellite.return(a * 100 + b * 10 + c)\n"
              "}\n",
              run);
        check(run.built, "three parameters compile");
        check(answer_of(run, "pick", {1, 2, 3}) == "123",
              "DESIGN §7.2: slots [0, parameters) are the argument list IN ORDER");
    }

    // --- a locals-in-a-frame check the answer alone would not catch ----------
    //
    // Two activations of one capsule alive at the same time, each writing its
    // own local. If the storage were shared -- which is exactly what v1 did --
    // the inner call would overwrite the outer one's `mine` and the answer
    // would be 2 rather than 1.
    {
        Run run;
        build("satellite.capsule outer(satellite.variable.number n)\n"
              "{\n"
              "    satellite.variable.number mine = n\n"
              "    satellite.statement.if (n == 1)\n"
              "    {\n"
              "        satellite.variable.number ignored = outer(2)\n"
              "    }\n"
              "    satellite.return(mine)\n"
              "}\n",
              run);
        check(run.built, "the nested-activation fixture compiles");
        check(answer_of(run, "outer", {1}) == "1",
              "§7.2: two activations of one capsule have two `mine`s -- the "
              "inner call cannot reach the outer one's slot");
    }

    // --- satellite.main is a capsule and can be called like one --------------
    //
    // IT COULD NOT UNTIL 2026-09-07, and the failure was a sentence about the
    // wrong thing: `satellite.main()` answered S0721, "a path satellite has a
    // number for and nothing behind yet", so a program calling a capsule IT
    // HAD DECLARED was told the LANGUAGE had not built it.
    //
    // The cause is that main is the one capsule whose name the parser LOOKS UP
    // instead of defining -- capsule_decl's reserved arm -- so its path is
    // `1 3`, a language word, where every capsule of the user's own is interned
    // past kNodeCount. The compiler's numbering arm therefore caught the call
    // first and dispatched a row with no handler.
    //
    // THE FIXTURE CALLS IT FROM ANOTHER CAPSULE ON PURPOSE. A test that only
    // ran a file whose main calls itself would pass against a build that never
    // reached the arm at all, because main runs anyway.
    {
        Run run;
        build("satellite.capsule satellite.main()\n"
              "{\n"
              "    satellite.return(7)\n"
              "}\n"
              "satellite.capsule reach()\n"
              "{\n"
              "    satellite.return(satellite.main())\n"
              "}\n",
              run);
        check(run.built, "a capsule that calls satellite.main compiles");
        check(answer_of(run, "reach", {}) == "7",
              "satellite.main() reaches the capsule the program declared, and "
              "not handlers[1 3 0], which is empty and always will be");
    }

    // --- and calling it still counts the arguments ---------------------------
    //
    // The arm that made the clause above pass returns before the numbering arm
    // runs, so this is the clause that says it did not also skip the checks
    // every other capsule call gets. A main that declares the argument list and
    // is called with nothing is S0722 and not a crash.
    {
        Run run;
        build("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> arguments)\n"
              "{\n"
              "    satellite.return(1)\n"
              "}\n"
              "satellite.capsule reach()\n"
              "{\n"
              "    satellite.return(satellite.main())\n"
              "}\n",
              run);
        call(run, "reach", {});
        check(ran_into(errors::Code::EVAL_ARGUMENT_COUNT),
              "S0722: satellite.main takes its declared parameters like any "
              "other capsule -- the reserved name buys it no exemption");
    }

    // --- the wrong number of arguments --------------------------------------
    {
        Run run;
        build("satellite.capsule two(satellite.variable.number a, "
              "satellite.variable.number b)\n{\n    satellite.return(a)\n}\n"
              "satellite.capsule caller()\n{\n    satellite.return(two(1))\n}\n",
              run);
        call(run, "caller", {});
        check(ran_into(errors::Code::EVAL_ARGUMENT_COUNT),
              "S0722: a call with the wrong number of arguments is refused, and "
              "PLAN §2.3 says the count is checked before the body runs");
    }
}

} // namespace eval_test
