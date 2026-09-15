// DESIGN §6.2's postfix loop, the precedence table, and the same-line rule.
// See tests/parser_test/parser_test.hpp.
//
// THE CHECKS ARE WRITTEN AGAINST THE PRINTED FORM RATHER THAN AGAINST NODE
// INDICES, and that is deliberate: `((a - b) - c)` says what the tree is in the
// language the tree is about, and a chain of `ast[ast[n].a].kind ==` assertions
// says the same thing in a way nobody can read against DESIGN §6. The printer
// is checked separately -- section_roundtrip() is what makes it trustworthy
// enough to be used as a microscope here.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"

#include <string>

using satellite::NodeKind;

namespace parser_test {

namespace {

// The one expression in a fixture, printed.
std::string only(const std::string &expression)
{
    const Program program = in_capsule(expression);
    if (!program.ok())
        return "<error: " + program.first_error() + ">";
    const satellite::NodeIndex statement =
        first_of(program.ast(), NodeKind::ExprStmt);
    if (statement == satellite::kNoNode)
        return "<no expression statement>";
    return print(program.ast(), program.ast()[statement].a);
}

} // namespace

void section_expressions()
{
    // -- Precedence and associativity ----------------------------------------

    check(only("f(1 + 2 * 3)") == "f(1 + 2 * 3)",
          "* binds tighter than + and needs no brackets printed back");
    check(only("f((1 + 2) * 3)") == "f((1 + 2) * 3)",
          "and the brackets come BACK when the program wrote them -- no node "
          "records a parenthesis, so this is the precedence table read backwards");
    check(only("f(a - b - c)") == "f(a - b - c)",
          "subtraction is left-associative, so a - b - c needs no brackets");
    check(only("f(a - (b - c))") == "f(a - (b - c))",
          "AND THE RIGHT-HAND SIDE KEEPS ITS BRACKETS AT EQUAL PRECEDENCE. This "
          "is the one case that silently changes an answer: a - (b - c) printed "
          "as a - b - c is a different number");
    check(only("f(a == b + 1)") == "f(a == b + 1)",
          "+ binds tighter than ==");
    check(only("f((a == b) == c)") == "f(a == b == c)",
          "A REDUNDANT BRACKET IS DROPPED, and that is the printer being right "
          "rather than lossy: == is left-associative, so a == b == c already "
          "means (a == b) == c and the brackets said nothing. What makes it safe "
          "to drop them is the fixpoint -- the second printing is identical");
    check(only("f(a == (b == c))") == "f(a == (b == c))",
          "while the brackets that DO say something are kept");
    check(only("f(1 + 2 + 3 * 4 - 5)") == "f(1 + 2 + 3 * 4 - 5)",
          "a mixed chain climbs without gaining a bracket");

    // -- Unary ---------------------------------------------------------------

    check(only("f(-1)") == "f(-1)",
          "DESIGN §5.6 refuses to fold a sign into a Number, so -1 is a unary "
          "expression over a Number and comes back as one");
    check(only("f(a - -1)") == "f(a - -1)", "and a - -1 is a subtraction of one");
    check(only("f(-(a + b))") == "f(-(a + b))",
          "a unary binds tighter than any binary, so its operand keeps brackets");

    // -- The postfix loop, DESIGN §6.2 ---------------------------------------

    check(only("foo().bar()") == "foo().bar()",
          "THE DEFECT §6.2 EXISTS FOR: parse_primary then while(peek=='.') "
          "cannot parse this at all");
    check(only("satellite.time.now().some_function()") ==
              "satellite.time.now().some_function()",
          "Member and Call are peers, so a call in the middle of a path needs "
          "no rule of its own");
    check(only("my_list[0].f()[1:2]") == "my_list[0].f()[1:2]",
          "and so do an index, a call and a slice in one chain");
    check(only("a.b.c.d") == "a.b.c.d",
          "a path of any length is a flat Member chain -- DESIGN §6.3, which "
          "refuses to teach the parser that a library path is four segments");

    {
        const Program program = in_capsule("a.b.c.d");
        check(count_of(program.ast(), NodeKind::Member) == 3,
              "three dots, three Member nodes, and no node that knows it is a path");
        check(count_of(program.ast(), NodeKind::Name) == 1,
              "with one Name at the bottom of the chain");
    }

    // -- Index against slice -------------------------------------------------

    check(only("x[1]") == "x[1]", "a subscript with no colon is an Index");
    check(only("x[1:2]") == "x[1:2]", "with a colon it is a Slice");
    check(only("x[:2]") == "x[:2]", "and either half may be left out");
    check(only("x[1:]") == "x[1:]", "including the other one");
    check(only("x[:]") == "x[:]", "or both");

    {
        const Program program = in_capsule("x[1]\nx[1:1]");
        check(count_of(program.ast(), NodeKind::Index) == 1 &&
                  count_of(program.ast(), NodeKind::Slice) == 1,
              "an index and a slice are different KINDS and not one kind with an "
              "absent half -- x[1] is an element and x[1:1] is a list of them");
    }

    // -- The same-line rule, DESIGN §6.2 -------------------------------------

    {
        // The failure this rule exists for, written out: a line ending in `x`
        // and a line opening `(f(y))`. Without the rule the postfix loop
        // reaches across the break and parses one call instead of two
        // statements.
        const Program program = in_capsule("x\n(f(y))");
        check(program.ok(), "two statements parse: " + program.first_error());
        check(count_of(program.ast(), NodeKind::ExprStmt) == 2,
              "A POSTFIX OPENER ON THE NEXT LINE DOES NOT BIND. Two statements, "
              "not the call x(f(y)) -- and what enforces it is that the newline "
              "is a token the postfix loop never crosses");
    }

    {
        const Program program = in_capsule("x[1]\n[2]");
        check(count_of(program.ast(), NodeKind::Index) == 1,
              "and the rule covers '[' as well as '(', which DESIGN §6.2 says "
              "explicitly because the sketch it corrects only covered one");
    }

    {
        // The other half of the same rule: inside brackets, a newline is not a
        // terminator, or no argument list could ever span lines.
        const Program program = in_capsule("f(\n    1,\n    2\n)");
        check(program.ok(), "an argument list may span lines: " + program.first_error());
        check(only("f(\n1,\n2\n)") == "f(1, 2)",
              "and comes back as one call with two arguments");
    }

    // -- What is not an operator ---------------------------------------------

    {
        const Program program = in_capsule("a & b");
        check(!program.ok(),
              "`&` HAS NO MEANING IN ANY DOCUMENT IN THIS TREE, so it gets "
              "precedence 0, ends the expression, and is reported -- rather "
              "than being given one here by whoever typed first");
    }
}

} // namespace parser_test
