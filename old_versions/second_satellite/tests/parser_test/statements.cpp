// DESIGN §6.1's dispatch, and the collision the whole rule exists for.
// See tests/parser_test/parser_test.hpp.
//
// THE FIRST CHECK IN THIS FILE IS THE MILESTONE'S MOST IMPORTANT ONE. §6.1
// records that a purely structural rule -- "a dotted path followed by a bare
// word is a declaration" -- is not safe, and shows why with two lines that are
// the same four tokens:
//
//     satellite.control.return my_time    ->  Word . Word . Word Word
//     satellite.variable.time  my_time    ->  Word . Word . Word Word
//
// The structural rule declares a variable of type `satellite.control.return`
// and says nothing about it. A test that only checked the second line would
// pass against a parser with that defect, which is why the first line is here.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"

#include <string>

using satellite::NodeKind;

namespace parser_test {

void section_statements()
{
    // -- §6.1's collision ----------------------------------------------------

    {
        const Program good = in_capsule("satellite.variable.time my_time");
        check(good.ok(), "a type path followed by a bare word IS a declaration: " +
                             good.first_error());
        check(count_of(good.ast(), NodeKind::VarDecl) == 1,
              "satellite.variable.time my_time declares a variable");
    }

    {
        const Program bad = in_capsule("satellite.control.return my_time");
        check(count_of(bad.ast(), NodeKind::VarDecl) == 0,
              "AND THE SAME FOUR TOKENS UNDER A DIFFERENT SEGMENT 1 DECLARE "
              "NOTHING. This is the defect DESIGN §6.1 exists for: a structural "
              "rule cannot tell these apart, and one of them is a variable of "
              "type satellite.control.return that nobody wrote");
        check(!bad.ok(), "and it is reported rather than accepted quietly");
    }

    {
        // The same shape again with a word the language really does own at
        // segment 1, so that the dispatch is being checked and not the fact
        // that `control` is unknown.
        const Program bad = in_capsule("satellite.console.display my_time");
        check(count_of(bad.ast(), NodeKind::VarDecl) == 0,
              "satellite.console is a module, not a type space, so the bare word "
              "after it is not a declaration either");
    }

    // -- The three forms of a type -------------------------------------------

    {
        const Program program = in_capsule(
            "satellite.container.list<satellite.variable.string> rows\n"
            "my_suit local_object\n"
            "satellite my_thing");
        check(program.ok(), "all three type forms parse: " + program.first_error());
        check(count_of(program.ast(), NodeKind::VarDecl) == 3,
              "a generic language type, a spacesuit named bare (§13) and the "
              "singleton `satellite` are three declarations");
    }

    check(!in_capsule("satellite.container.list<satellite.variable.string rows").ok(),
          "an unclosed generic is reported");

    // -- Assignment ----------------------------------------------------------

    {
        const Program program = in_capsule("my_binary = binary_input");
        check(program.ok(), "an assignment parses: " + program.first_error());
        check(count_of(program.ast(), NodeKind::Assign) == 1, "as one Assign");
    }

    {
        const Program program = in_capsule("foo() = 1");
        check(!program.ok(),
              "THE LEFT OF AN ASSIGNMENT HAS TO NAME SOMEWHERE TO WRITE. DESIGN "
              "§6.4's `foo().append(x)` is the same finding one level up -- a "
              "call has nowhere to write back to");
    }

    check(in_capsule("x[0] = 1").ok(), "an index does name somewhere to write");
    check(in_capsule("satellite.library.counter = 1").ok(),
          "and so does a library path, which is what DESIGN §7.2 reserves "
          "satellite.library for");

    // -- if, else, while, for ------------------------------------------------

    {
        const Program program = in_capsule(
            "satellite.statement.if (n < 10)\n"
            "{\n"
            "    f()\n"
            "}\n");
        check(program.ok(), "an if parses: " + program.first_error());
        check(count_of(program.ast(), NodeKind::If) == 1, "as one If");
    }

    {
        const Program program = in_capsule(
            "satellite.statement.if (a)\n{\nf()\n}\n"
            "satellite.statement.else satellite.statement.if (b)\n{\ng()\n}\n"
            "satellite.statement.else\n{\nh()\n}\n");
        check(program.ok(), "an else-if chain parses: " + program.first_error());
        check(count_of(program.ast(), NodeKind::If) == 2,
              "AN ELSE-IF IS AN If IN THE ELSE SLOT, which is what keeps "
              "`else if` from being a form of its own -- DESIGN §6's "
              "`else ( block | if_stmt )`");
    }

    check(!in_capsule("satellite.statement.else\n{\nf()\n}").ok(),
          "an else with no if is named as such rather than reported as an "
          "unexpected word");
    check(!in_capsule("satellite.statement.unless (a)\n{\n}").ok(),
          "and a statement the language does not have is reported against the "
          "four it does");

    {
        const Program program = in_capsule(
            "satellite.statement.while (n > 0)\n{\nn = n - 1\n}");
        check(program.ok() && count_of(program.ast(), NodeKind::While) == 1,
              "a while parses: " + program.first_error());
    }

    {
        const Program program = in_capsule(
            "satellite.statement.for (satellite.variable.number i = 0; i < n; "
            "i = i + 1)\n{\nf(i)\n}");
        check(program.ok() && count_of(program.ast(), NodeKind::For) == 1,
              "a for with all three parts parses: " + program.first_error());
        check(count_of(program.ast(), NodeKind::VarDecl) == 1,
              "and its first part may be a declaration");
    }

    {
        const Program program = in_capsule("satellite.statement.for (;;)\n{\nf()\n}");
        check(program.ok(),
              "ALL THREE PARTS ARE OPTIONAL AND THE TWO SEMICOLONS ARE NOT: " +
                  program.first_error());
    }

    // -- What a block does not hold ------------------------------------------

    // AN INCLUDE DOES GO INSIDE A BLOCK SINCE 2026-09-13 -- M25, the author's
    // "satellite.include(filename(args)) in the middle of a capsule". This
    // clause asserted the opposite until then, and a declaration word that
    // still does not belong in a block takes its place below.
    {
        const Program program = in_capsule("satellite.include(cargo(1, \"two\"))");
        check(program.ok() && count_of(program.ast(), NodeKind::Include) == 1,
              "satellite.include is a statement inside a block: " +
                  program.first_error());
    }

    {
        const Program program = in_capsule("satellite.spacesuit inner()\n{\n}");
        check(!program.ok(),
              "satellite.spacesuit is a declaration and does not go inside a "
              "block, and the message says which word was written rather than "
              "'unexpected token'");
    }

    // -- Statements are newline-terminated -----------------------------------

    {
        const Program program = in_capsule("f() g()");
        check(!program.ok(),
              "TWO STATEMENTS ON ONE LINE HAVE NO SEPARATOR IN THIS LANGUAGE. "
              "DESIGN §6 says statements are newline-terminated and there is no "
              "`;` outside a for head, so this is reported rather than accepted");
    }

    {
        const Program program = in_capsule("{\nf()\n}");
        check(program.ok() && count_of(program.ast(), NodeKind::Block) == 2,
              "a bare block is a statement -- the fixture's own body is the other "
              "one: " + program.first_error());
    }

    // -- Return --------------------------------------------------------------

    {
        const Program program = in_capsule("satellite.return()");
        check(program.ok() && count_of(program.ast(), NodeKind::Return) == 1,
              "satellite.return() with no value parses: " + program.first_error());
        const satellite::NodeIndex node = first_of(program.ast(), NodeKind::Return);
        check(program.ast()[node].a == satellite::kNoNode,
              "and carries no value node -- 1 15 0 rather than 1 15 1");
    }

    check(in_capsule("satellite.return(satellite)").ok(),
          "and satellite.return(satellite) is the form DESIGN §3 ends on");
}

} // namespace parser_test
