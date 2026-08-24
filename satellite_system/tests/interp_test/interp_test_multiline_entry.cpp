// Typing more than one line at the prompt: which heads open a body, what a
// brace inside a string is not, and how long a declaration typed at the prompt
// lives. Part of the interp_test binary; the harness it calls is declared in
// interp_test.hpp.

#include "interp_test.hpp"

#include "interpreter/interp.hpp"

#include <string>

using namespace satellite;

void interp_test_multiline_entry()
{
    // --- multi-line entry at the prompt ------------------------------------
    // scan_block lives in interp.* rather than main.cpp for the same reason
    // parse_run_command does: it is language knowledge, and this is where it
    // can be tested without linking gtk.
    {
        // A declaration head with no brace is the one shape the prompt is
        // allowed to add to.
        BlockScan capsule = scan_block(
            "satellite.capsule f(satellite.variable.number n)");
        check(capsule.opens_body && capsule.depth == 1 && !capsule.lex_error,
              "a capsule signature opens a body");

        check(scan_block("satellite.spacesuit box()").opens_body,
              "a spacesuit signature opens a body");
        check(scan_block("satellite.statement.if (n > 0)").opens_body,
              "an if opens a body");
        check(scan_block("satellite.statement.while (n > 0)").opens_body,
              "a while opens a body");
        check(scan_block("satellite.statement.for (;;)").opens_body,
              "a for opens a body");
        check(scan_block("satellite.statement.else").opens_body,
              "a bare else opens a body");
        check(scan_block("satellite.statement.else()").opens_body,
              "else with empty parens opens a body too (§19.8)");
        check(scan_block("satellite.protected").opens_body &&
                  scan_block("satellite.public").opens_body,
              "an access block opens a body");

        // Somebody who closed it themselves has said what they meant, and the
        // prompt must not add to working input.
        BlockScan whole = scan_block(
            "satellite.capsule f() { satellite.return(1) }");
        check(!whole.opens_body && whole.depth == 0,
              "a capsule written whole on one line opens nothing");

        // Depth is what every other multi-line form is detected by.
        check(scan_block("{").depth == 1, "an open brace opens a level");
        check(scan_block("}").depth == -1, "a close brace closes one");
        check(scan_block("} satellite.statement.else {").depth == 0,
              "a line that closes and reopens is net zero");

        // Ordinary lines open nothing at all.
        check(scan_block("satellite.console.display(\"hi\")").depth == 0 &&
                  !scan_block("satellite.console.display(\"hi\")").opens_body,
              "a plain statement opens nothing");
        check(!scan_block("satellite.variable.number x = 1").opens_body,
              "a declaration is not a body");

        // A BRACE INSIDE A STRING IS NOT A BRACE. This is the whole reason the
        // count is over tokens and not over bytes: the lexer has already
        // absorbed the literal, so the '{' never reaches the counter.
        check(scan_block("satellite.console.display(\"{\")").depth == 0,
              "a brace inside a string literal is not a brace");
        check(scan_block("satellite.console.display(\"}}}\")").depth == 0,
              "nor are three of them");
    }
}

void interp_test_prompt_session()
{
    // A prompt session's declarations survive the line they were typed on,
    // which is the half of §16's open question that makes multi-line entry
    // worth having. run_source(session = true) is what the REPL passes.
    {
        const std::string ns = fresh_ns();
        run_source("satellite.capsule twice(satellite.variable.number n)\n"
                   "{\n"
                   "    satellite.return(n + n)\n"
                   "}\n",
                   ns, true, nullptr, true);
        InterpResult call = run_source("twice(21)\n", ns, true, nullptr, true);
        check(call.output == "42\n", "a capsule typed earlier is callable later");

        // Newest wins, or redefining a capsule at the prompt would silently
        // keep running the first one.
        run_source("satellite.capsule twice(satellite.variable.number n)\n"
                   "{\n"
                   "    satellite.return(n + n + 1)\n"
                   "}\n",
                   ns, true, nullptr, true);
        InterpResult again = run_source("twice(21)\n", ns, true, nullptr, true);
        check(again.output == "43\n", "a redefinition replaces the earlier one");
    }

    // And WITHOUT the session flag it does not, which is what keeps five test
    // files from leaking declarations into each other.
    {
        const std::string ns = fresh_ns();
        run_source("satellite.capsule solo()\n{\n    satellite.return(1)\n}\n",
                   ns, true);
        InterpResult call = run_source("solo()\n", ns, true);
        check(call.output != "1\n",
              "a capsule does not survive a non-session run");
    }
}
