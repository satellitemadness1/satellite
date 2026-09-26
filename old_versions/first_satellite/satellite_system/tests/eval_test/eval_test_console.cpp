// What the program says and who it says it to: satellite.help,
// satellite.console.display with its end= argument and its pace form, and the
// satellite.return that stops the program mid-file. Part of the eval_test
// binary; the harness these call is declared in eval_test.hpp.

#include "eval_test.hpp"

#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"

#include <string>

using namespace satellite;

void eval_test_help()
{
    // --- satellite.help -----------------------------------------------------
    // Reachable WITHOUT parentheses, on purpose: someone who needs help may not
    // remember the calling syntax, and that is the one place a language can
    // least afford to insist on it.
    {
        InterpResult bare = run_source("satellite.help\n", fresh_ns(), true);
        check(bare.ok && bare.output.find("the whole language") != std::string::npos,
              "satellite.help works with no parentheses");
        InterpResult called = run_source("satellite.help()\n", fresh_ns(), true);
        check(called.ok && called.output == bare.output,
              "satellite.help() gives exactly the same text");

        // Given a value, it answers with that value's methods -- and the table
        // it reads from is the one call_method dispatches on, so a listed
        // method is a method that exists.
        check_output("satellite.help(1)\n",
                     std::string("satellite.variable.number\n") +
                     "  .plus(n) .minus(n) .times(n) .divided_by(n) .modulo(n)\n" +
                     "  .abs() .floor() .ceil() .round() .to_string()\n" +
                     "  .digits() -> decimal digits   .size() -> bytes\n" +
                     "  no .length(): a number holds no items\n\n",
                     "help on a number lists number methods");
        InterpResult str = run_source("satellite.help(\"x\")\n", fresh_ns(), true);
        check(str.output.find(".starts_with(s)") != std::string::npos,
              "help on a string lists string methods");
        InterpResult nil = run_source("satellite.help(satellite)\n", fresh_ns(), true);
        check(nil.output.find("no methods") != std::string::npos,
              "help on nil says so");

        check_error("satellite.help(1, 2)\n", "takes 1 argument",
                    "help takes at most one value");
    }
}

void eval_test_display_end()
{
    // --- display(text, end=...) ---------------------------------------------
    check_output("satellite.console.display(\"a\", end=\"\")\n"
                 "satellite.console.display(\"b\")\n",
                 "ab\n", "end=\"\" displays without a newline");
    check_output("satellite.console.display(\"a\", end=\"-\")\n"
                 "satellite.console.display(\"b\")\n",
                 "a-b\n", "end= takes any string, not just empty");

    // A named argument is grammar, not a general facility, and says so
    // wherever it is not understood.
    check_error("satellite.console.display(\"a\", nope=\"\")\n",
                "named arguments are not part of the language",
                "an unknown named argument is refused by name");
    check_error("satellite.variable.number x = 1\nx.plus(end=1)\n",
                "named arguments are not part of the language",
                "a named argument to a method is refused");
}

void eval_test_display()
{
    // --- display and return ------------------------------------------------
    check_output("satellite.console.display(\"hello, world!\")\n",
                 "hello, world!\n", "console.display");
    check_output("satellite.console.display(1 + 1)\n", "2\n",
                 "display renders any value");
}

void eval_test_pace()
{
    // --- the pace form, satellite.console.display(100ms) --------------------
    //
    // What it DOES is timed in console_test, where the printer thread is; what
    // is checked here is the language half. There is no Console behind
    // run_source -- the buffer is the answer to a test, per §9 -- so a pace is
    // accepted and paces nothing, and displaying nothing is exactly right.
    check_output("satellite.console.display(100ms)\n", "",
                 "a pace displays nothing");
    check_output("satellite.console.display(100 ms)\n", "",
                 "the space is not a different literal");
    check_output("satellite.console.display(0ms)\n", "", "0ms turns pacing off");
    check_output("satellite.console.display(1.5ms)\n", "",
                 "a fractional millisecond is a duration too");
    check_output("satellite.console.display(100ms)\n"
                 "satellite.console.display(\"after\")\n",
                 "after\n", "a paced program still displays");

    // A duration is legal in exactly one position (§8.2 stands: there is no
    // satellite.variable.duration), and every other one names the form that
    // works rather than only refusing.
    check_error("satellite.variable.number x = 100ms\n", "a duration is not a value",
                "a duration cannot initialise a variable");
    check_error("satellite.variable.number x = 100ms\n",
                "satellite.console.display(100ms)",
                "the refusal names the form that works");
    check_error("satellite.console.display(100ms.plus(1))\n",
                "a duration is not a value", "a duration has no methods");
    check_error("satellite.console.display(1, 100ms)\n",
                "a duration is not a value",
                "a duration is not an argument among others");
    check_error("satellite.time.now(100ms)\n", "a duration is not a value",
                "no other module function takes one");
    check_error("satellite.console.display(2 + 100ms)\n",
                "a duration is not a value", "a duration is not an operand");

    // `ms` on the next line is a NAME, not a unit. The rule is the one the
    // postfix '[' already follows, and it is what keeps a line that happens to
    // start with `ms` from being swallowed by the line above.
    check_error("satellite.console.display(1)\nms\n", "no such variable: ms",
                "a unit does not reach across a newline");

    // satellite.return stops the program. It is a status enum, not a thrown
    // exception (§10) — what matters here is only that it stops.
    {
        std::string ns = fresh_ns();
        InterpResult r = run_source("satellite.variable.number a = 1\n"
                                    "satellite.return(2)\n"
                                    "satellite.variable.number b = 99\n",
                                    ns, true);
        check(r.ok, "return runs clean");
        check(Library::instance().get(ns, "a") != nullptr, "return: a was set");
        check(Library::instance().get(ns, "b") == nullptr,
              "return stopped before b");
    }
}
