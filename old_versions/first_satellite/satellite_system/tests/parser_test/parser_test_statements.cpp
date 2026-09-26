// Statements: control flow, capsules, the same-line rule that keeps a line
// beginning '(' from being swallowed by the line above, nesting, and the order
// of top-level items.
//
// Part of satellite_system/tests/parser_test/, split from a 442-line
// parser_test.cpp.

#include "parser_test.hpp"

#include <variant>

using namespace satellite;

void parser_test_control_flow()
{
    // ---- control flow -----------------------------------------------------
    round_trip("satellite.statement.if(n <= 1)\n"
               "{\n"
               "    satellite.return(1)\n"
               "}\n");
    round_trip("satellite.statement.if(n <= 1)\n"
               "{\n"
               "    satellite.return(1)\n"
               "}\n"
               "satellite.statement.else\n"
               "{\n"
               "    satellite.return(2)\n"
               "}\n");
    round_trip("satellite.statement.while(n > 0)\n"
               "{\n"
               "    n = n - 1\n"
               "}\n");
    round_trip("satellite.statement.for(satellite.variable.number i = 0; "
               "i < 10; i = i + 1)\n"
               "{\n"
               "    satellite.console.display(argz[i])\n"
               "}\n");
    round_trip("satellite.statement.for(; ; )\n"
               "{\n"
               "    satellite.return(satellite)\n"
               "}\n");

    // else-if chains nest rather than needing a separate form.
    {
        ParseResult r = parse("satellite.statement.if(a)\n{\n}\n"
                              "satellite.statement.else\n"
                              "satellite.statement.if(b)\n{\n}\n");
        check(r.ok(), "an else-if chain parses");
        const StmtPtr *s = std::get_if<StmtPtr>(&r.program.items[0]);
        const If *outer = s ? std::get_if<If>(s->get()) : nullptr;
        check(outer && outer->else_branch &&
                  std::holds_alternative<If>(*outer->else_branch),
              "the else branch of an else-if is another If");
    }
}

void parser_test_capsules()
{
    // ---- capsules ---------------------------------------------------------
    round_trip("satellite.capsule fact(satellite.variable.number n) "
               "satellite.returns(satellite.variable.number)\n"
               "{\n"
               "    satellite.return(n)\n"
               "}\n");
    round_trip("satellite.capsule noop()\n{\n}\n");
    round_trip("satellite.capsule add(satellite.variable.number a, "
               "satellite.variable.number b)\n"
               "{\n"
               "    satellite.return(a + b)\n"
               "}\n");

    // A capsule the user writes is bare; only the entry point is prefixed.
    {
        ParseResult r = parse("satellite.capsule fact()\n{\n}\n");
        const Capsule *c = std::get_if<Capsule>(&r.program.items[0]);
        check(c && !c->reserved && c->name == "fact",
              "a user capsule is bare");

        ParseResult m = parse("satellite.capsule satellite.main()\n{\n}\n");
        const Capsule *mc = std::get_if<Capsule>(&m.program.items[0]);
        check(mc && mc->reserved && mc->name == "main",
              "satellite.main is the reserved entry point");
    }
}

void parser_test_same_line_rule()
{
    // ---- the same-line rule ----------------------------------------------
    // Without it, a line beginning '(' or '[' is silently absorbed by the line
    // above — the defect that forced JavaScript's semicolon-insertion rules.
    // The '(' case is the one observable today; '[' carries the same guard and
    // becomes observable when list literals exist.
    {
        ParseResult r = parse("display(x)\n(y)\n");
        check(r.ok(), "two statements on two lines parse");
        check(r.program.items.size() == 2,
              "a '(' on the next line does not call the line above");

        ParseResult joined = parse("display(x)(y)\n");
        check(joined.ok() && joined.program.items.size() == 1,
              "a '(' on the same line still calls");
    }

    // Statements are newline-separated, so a second statement crammed onto one
    // line is an error rather than a silent double parse.
    check_fails("display(x) display(y)\n", "two statements on one line");
}

void parser_test_nesting()
{
    // ---- nesting ----------------------------------------------------------
    round_trip("satellite.capsule satellite.main("
               "satellite.container.list<satellite.variable.string> argz)\n"
               "{\n"
               "    satellite.statement.for(satellite.variable.number i = 0; "
               "i < 10; i = i + 1)\n"
               "    {\n"
               "        satellite.statement.if(i == 5)\n"
               "        {\n"
               "            satellite.console.display(argz[i])\n"
               "        }\n"
               "    }\n"
               "    satellite.return(satellite)\n"
               "}\n");
}

void parser_test_top_level_order()
{
    // ---- multiple top-level items keep their order -----------------------
    round_trip("satellite.include(satellite)\n"
               "\n"
               "satellite.capsule a()\n{\n}\n"
               "\n"
               "satellite.capsule b()\n{\n}\n");
}
