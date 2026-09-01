// DESIGN §6's three compound statements, running -- and the one rule that makes
// them refuse rather than guess.
//
// SATELLITE HAS NO TRUTHINESS, AND THAT IS THE SUBJECT OF HALF THIS FILE. A
// condition is a `satellite.variable.bool` and nothing else: no zero that is
// false, no empty string that is false. DESIGN §1.1's rule is never to do
// anything behind the user's back, and a number quietly standing in for a test
// is the oldest way a language does exactly that.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"

#include <string>

namespace eval_test {

namespace {

std::string in_a_capsule(const std::string &body)
{
    return "satellite.capsule it(satellite.variable.number n)\n{\n" + body + "}\n";
}

std::string run_body(const std::string &body, long long argument)
{
    Run run;
    build(in_a_capsule(body), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {argument});
}

} // namespace

void section_control()
{
    using namespace satellite;

    // --- if, and the else that may not be there ------------------------------
    check(run_body("    satellite.statement.if (n < 5)\n"
                   "    {\n"
                   "        satellite.return(1)\n"
                   "    }\n"
                   "    satellite.return(2)\n",
                   1) == "1",
          "an `if` whose condition holds takes its block");
    check(run_body("    satellite.statement.if (n < 5)\n"
                   "    {\n"
                   "        satellite.return(1)\n"
                   "    }\n"
                   "    satellite.return(2)\n",
                   9) == "2",
          "and falls through when it does not");

    check(run_body("    satellite.statement.if (n < 5)\n"
                   "    {\n"
                   "        satellite.return(1)\n"
                   "    }\n"
                   "    satellite.statement.else\n"
                   "    {\n"
                   "        satellite.return(2)\n"
                   "    }\n",
                   9) == "2",
          "an `else` block");

    // AN `else if` CHAIN IS ONE WORK ITEM PER LINK AND NOT ONE PER CHAIN,
    // because op_if pops itself before pushing its branch. The answer is what
    // is checked here; tests/eval_test/depth.cpp checks the stack.
    check(run_body("    satellite.statement.if (n < 1)\n    {\n"
                   "        satellite.return(1)\n    }\n"
                   "    satellite.statement.else satellite.statement.if (n < 2)\n    {\n"
                   "        satellite.return(2)\n    }\n"
                   "    satellite.statement.else\n    {\n"
                   "        satellite.return(3)\n    }\n",
                   1) == "2",
          "an `else if` chain picks the middle arm");

    // --- while ---------------------------------------------------------------
    check(run_body("    satellite.variable.number total = 0\n"
                   "    satellite.variable.number i = 0\n"
                   "    satellite.statement.while (i < n)\n"
                   "    {\n"
                   "        total = total + i\n"
                   "        i = i + 1\n"
                   "    }\n"
                   "    satellite.return(total)\n",
                   10) == "45",
          "a `while` loop adds up 0..9");
    check(run_body("    satellite.variable.number total = 0\n"
                   "    satellite.statement.while (n < 0)\n"
                   "    {\n"
                   "        total = total + 1\n"
                   "    }\n"
                   "    satellite.return(total)\n",
                   5) == "0",
          "a `while` whose condition never holds runs no rounds");

    // --- for -----------------------------------------------------------------
    check(run_body("    satellite.variable.number total = 0\n"
                   "    satellite.statement.for (satellite.variable.number i = 0; "
                   "i < n; i = i + 1)\n"
                   "    {\n"
                   "        total = total + i\n"
                   "    }\n"
                   "    satellite.return(total)\n",
                   10) == "45",
          "a `for` loop adds up 0..9, and its initialiser has a slot of its own");

    // A `satellite.return` FROM INSIDE TWO LOOPS AND AN `if` DROPS ALL THREE IN
    // ONE RESIZE, which is what the explicit control stack buys over a recursive
    // evaluator threading a status code through every arm.
    check(run_body("    satellite.statement.while (0 < 1)\n    {\n"
                   "        satellite.statement.while (0 < 1)\n        {\n"
                   "            satellite.statement.if (0 < 1)\n            {\n"
                   "                satellite.return(7)\n            }\n"
                   "        }\n    }\n",
                   0) == "7",
          "a return unwinds out of two loops and an `if` at once");

    // --- a condition is a bool ----------------------------------------------
    {
        Run run;
        build(in_a_capsule("    satellite.statement.if (n)\n    {\n"
                           "        satellite.return(1)\n    }\n"
                           "    satellite.return(0)\n"),
              run);
        call(run, "it", {1});
        check(ran_into(errors::Code::EVAL_NOT_A_CONDITION),
              "S0710: a number is not a condition -- satellite has no truthiness");
    }

    // --- a declaration with no value is nothing, every round -----------------
    //
    // DESIGN §7.4 gives a name its own slot and never reuses one across scopes,
    // so a declaration inside a loop body is the SAME slot on every iteration.
    // A variable declared without a value must be nothing on round two as well
    // as on round one, which is the kind of thing that reads as working until a
    // loop runs twice.
    check(run_body("    satellite.variable.number seen = 0\n"
                   "    satellite.variable.number i = 0\n"
                   "    satellite.statement.while (i < 3)\n"
                   "    {\n"
                   "        satellite.variable.number fresh\n"
                   "        satellite.statement.if (fresh == 0)\n"
                   "        {\n"
                   "            seen = seen + 1\n"
                   "        }\n"
                   "        fresh = 1\n"
                   "        i = i + 1\n"
                   "    }\n"
                   "    satellite.return(seen)\n",
                   0) == "0",
          "a declared-and-empty local is `nothing` and not 0 -- and it is "
          "nothing again on the second round, not last round's answer");
}

} // namespace eval_test
