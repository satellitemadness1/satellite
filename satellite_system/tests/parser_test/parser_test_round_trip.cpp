// The round trip itself: hello world, the duration literal, declarations, and
// -- at the end of the run -- the idempotence of unparse. These are the
// sections that assert on canonical text rather than on tree shape.
//
// Part of satellite_system/tests/parser_test/, split from a 442-line
// parser_test.cpp.

#include "parser_test.hpp"

#include <string>
#include <variant>

using namespace satellite;

void parser_test_hello_world()
{
    // ---- hello world ------------------------------------------------------
    round_trip(hello_world_source);

    // Comments and blank lines are not in the tree, so they are the one thing
    // the round trip cannot reproduce — but they must not change the parse.
    {
        const std::string commented =
            "satellite.include(satellite) // all programs include satellite\n"
            "\n"
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.string> argz)\n"
            "{\n"
            "    // \"capsules\" are just functions in satellite\n"
            "    satellite.console.display(\"hello, world!\")\n"
            "    satellite.return(satellite) // return(0); in satellite\n"
            "}\n";
        ParseResult r = parse(commented);
        check(r.ok(), "the commented hello world parses");
        check(unparse(r.program) == hello_world_source,
              "comments do not change the parse");
    }
}

void parser_test_duration_literals()
{
    // ---- the duration literal ---------------------------------------------
    //
    // `100ms` and `100 ms` are ONE literal and one node, which is why they
    // unparse identically: the lexer stops a number at the first non-digit and
    // a word cannot start with a digit, so both spellings arrive as Number then
    // Word("ms") with nothing to tell them apart.
    check_expr("satellite.console.display(100ms)",
               "satellite.console.display(100ms)");
    check_expr("satellite.console.display(100 ms)",
               "satellite.console.display(100ms)");
    round_trip("satellite.console.display(100ms)\n");

    // As written, so a trailing zero survives -- the same reason NumberLit
    // keeps its text.
    check_expr("satellite.console.display(1.50ms)",
               "satellite.console.display(1.50ms)");

    // A unit is a unit only on the SAME line as its number, and only when it is
    // `ms`. Everything else is what it was before: a number, then a name the
    // grammar has no room for.
    check_fails("satellite.console.display(100\nms)",
                "a unit does not reach across a newline");
    check_fails("satellite.console.display(100 seconds)",
                "only ms is a unit");
}

void parser_test_declarations()
{
    // ---- declarations -----------------------------------------------------
    round_trip("satellite.variable.time my_time = satellite.time.now()\n");
    round_trip("satellite.variable.file my_file = satellite.file.new()\n");
    round_trip("satellite.variable.file my_file\n");
    round_trip("satellite.variable.number x = 1\n");
    round_trip("satellite.variable.string s = \"hi\"\n");
    round_trip("satellite.container.list<satellite.variable.string> names\n");
    round_trip("satellite.container.list<satellite.container.list"
               "<satellite.variable.string>> grid\n");

    // The declaration is found by segment 1, not by shape. Dispatching on
    // shape would read this as declaring a variable named `if`.
    {
        ParseResult r = parse(
            "satellite.statement.if(x)\n{\n    satellite.return(1)\n}\n");
        check(r.ok(), "an if statement parses");
        check(r.program.items.size() == 1, "an if is one item");
        const StmtPtr *s = std::get_if<StmtPtr>(&r.program.items[0]);
        check(s && std::holds_alternative<If>(**s),
              "satellite.statement.if is an If, not a declaration named 'if'");
    }
}

void parser_test_unparse_idempotence()
{
    // ---- unparse output always reparses ----------------------------------
    // Idempotence holds even where exact round tripping cannot: reparsing the
    // canonical form must give the canonical form back.
    for (const std::string &src :
         {std::string("a + (b + c) * -d\n"), std::string("((x))\n"),
          std::string("l[0].f()[1:2]\n"), hello_world_source}) {
        ParseResult first = parse(src);
        check(first.ok(), "sample parses: " + src);
        const std::string once = unparse(first.program);
        ParseResult second = parse(once);
        check(second.ok(), "canonical form reparses: " + once);
        check(unparse(second.program) == once,
              "unparse is idempotent: " + once);
    }
}
