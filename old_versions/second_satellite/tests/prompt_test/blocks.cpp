// Where a line goes, and whether it is finished. See prompt_test.hpp clause 3.

#include "prompt_test.hpp"

#include "satellite_prompt/block.hpp"

using namespace satellite::prompt;

namespace prompt_test {

void section_blocks()
{
    // 1 -- AN ORDINARY STATEMENT IS FINISHED AND GOES INSIDE main.
    {
        const Scan got = scan("satellite.console.display(\"hi\")");
        check(got.depth == 0, "a statement opens nothing");
        check(!got.opens_body, "and owes no body");
        check(got.placement == Placement::Statement, "it goes inside main");
        check(!got.empty && !got.lex_error, "and it lexed");
    }

    // 2 -- A BRACE INSIDE A STRING IS NOT A BRACE. THE CLAUSE THIS FILE EXISTS
    // FOR: the obvious implementation counts characters and gets this wrong,
    // and the failure is a prompt that will not run a line until the user
    // types a closing brace they never opened.
    {
        const Scan got = scan("satellite.console.display(\"{\")");
        check(got.depth == 0, "a `{` inside a string does not open a block");
    }
    {
        const Scan got = scan("satellite.console.display(\"}\")");
        check(got.depth == 0, "a `}` inside a string does not close one");
    }

    // 3 -- A COMMENT IS NOT CODE EITHER, which comes free from lexing rather
    // than from a second rule about `//`.
    {
        const Scan got = scan("// a comment holding { and }");
        check(got.empty, "a comment-only line holds nothing");
        check(got.depth == 0, "and its braces are not braces");
    }
    {
        const Scan got = scan("");
        check(got.empty, "an empty line is empty");
        const Scan spaces = scan("    ");
        check(spaces.empty, "so is a line of spaces");
    }

    // 4 -- REAL BRACES COUNT, AND THEY NEST.
    {
        check(scan("{").depth == 1, "an open brace is depth 1");
        check(scan("}").depth == -1, "a close brace is depth -1");
        check(scan("{ { }").depth == 1, "two open and one close is depth 1");
        check(scan("satellite.statement.if (x) {").depth == 1,
              "a head with its brace on the same line is depth 1");
    }

    // 5 -- THE OWED BODY. This language writes `{` on its OWN line, so a head
    // counts zero braces and would be run without its body -- measured exactly
    // that way before the flag existed.
    {
        const Scan got = scan("satellite.statement.for (satellite.variable.number i = 0; i < 5; i = i + 1)");
        check(got.depth == 0, "the head line counts no braces");
        check(got.opens_body, "but it owes a body");
    }
    {
        check(scan("satellite.statement.if (loud)").opens_body, "if owes a body");
        check(scan("satellite.statement.while (x)").opens_body, "while owes a body");
        check(scan("satellite.statement.else").opens_body, "else owes a body");
        check(scan("satellite.capsule double_it(satellite.variable.number n)").opens_body,
              "a capsule owes a body");
    }

    // 6 -- AND A HEAD THAT ALREADY HAS ITS BRACE OWES NOTHING, or the prompt
    // would wait for a body it has been given.
    {
        const Scan got = scan("satellite.statement.if (x) {");
        check(!got.opens_body, "a brace on the head line settles it");
        check(got.depth == 1, "the depth carries it instead");
    }

    // 7 -- THE FOUR TOP-LEVEL FORMS, WHICH ARE S0204's LIST.
    {
        check(scan("satellite.include(satellite)").placement == Placement::TopLevel,
              "include is top level");
        check(scan("satellite.capsule f()").placement == Placement::TopLevel,
              "capsule is top level");
        check(scan("satellite.spacesuit s").placement == Placement::TopLevel,
              "spacesuit is top level");
        check(scan("satellite.library.count = 0").placement == Placement::TopLevel,
              "a library global is top level");
    }

    // 8 -- AND include AND library OWE NO BODY, which is the difference between
    // the top-level list and the opens-a-body list. A prompt that confused them
    // would sit waiting for a block after `satellite.include(satellite)`.
    {
        check(!scan("satellite.include(satellite)").opens_body,
              "include owes no body");
        check(!scan("satellite.library.count = 0").opens_body,
              "a library global owes no body");
    }

    // 9 -- A BARE NAME IS THE USER'S AND IS NEVER A TOP-LEVEL FORM, DESIGN §1.
    // `capsule` on its own is a variable somebody may have called that.
    {
        check(scan("capsule").placement == Placement::Statement,
              "a bare `capsule` is not a declaration");
        check(scan("library.count = 0").placement == Placement::Statement,
              "a bare `library` is not one either");
        check(!scan("statement").opens_body, "and a bare `statement` owes no body");
    }

    // 10 -- THE LIBRARY GLOBAL'S NAME IS READ OUT, which is what lets the
    // session tell a declaration from an assignment. The same text is both,
    // depending on where it sits, and this scanner cannot see where it sits.
    {
        const Scan got = scan("satellite.library.counter = 5");
        check(got.placement == Placement::TopLevel, "it is a top-level form");
        check(got.library_name == "counter", "and the name is read out of it");
    }
    {
        check(scan("satellite.console.display(1)").library_name.empty(),
              "an ordinary statement names no global");
        check(scan("satellite.capsule f()").library_name.empty(),
              "and neither does a capsule");
    }

    // 11 -- AN UNTERMINATED STRING IS REPORTED AND NOT SWALLOWED. A prompt
    // that read it as depth 0 would run half a line.
    {
        const Scan got = scan("satellite.console.display(\"unterminated");
        check(got.lex_error, "an unterminated string is a lex error");
    }
}

} // namespace prompt_test
