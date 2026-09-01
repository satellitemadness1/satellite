// The depths a program may choose, and the promise that none of them is a
// limit. See tests/parser_test/parser_test.hpp for the harness.
//
// THIS SECTION IS M8.5's DONE-WHEN. DESIGN §7.5: the language has no depth
// limit, and no walker in it may use the C++ stack for a depth the user's
// program controls. `SCRATCH.md/NO_LIMITS.md` §7 is the acceptance test and
// this is the half of it that belongs to the parser and the printer -- the
// resolver's is tests/resolve_test/frames.cpp and the writer's is
// tests/satc_test.
//
// AND THIS BINARY DOES NOT RAISE ITS OWN STACK, WHICH IS WHAT MAKES THE CHECKS
// BELOW MEAN ANYTHING. `satl` asks the kernel for a share of the machine at
// startup -- 32 KiB for every MiB, machine_limits/limits.hpp -- and
// 065-tests.mk links no such thing into this binary. So every check here runs
// on whatever `ulimit -s` handed the shell, which is 8 MiB on an ordinary
// login, and 8 MiB is exactly where `satl --unparse` died at 19,000 nested
// brackets before this milestone. A test that ran with the raised stack would
// pass over a recursive parser and prove nothing.
//
// THE NUMBERS ARE DELIBERATELY ABSURD. A 100,000-deep expression is machine
// generated and no person will write one; the point is that the interpreter's
// answer does not depend on who generated the file, which is what "no limits"
// means. Where a number below is smaller it is because the SOURCE gets fat
// rather than because the depth does -- a nested type is twenty-four
// characters a level -- and each says so.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"
#include "parser/parser.hpp"

#include <string>

namespace parser_test {

namespace {

using satellite::Ast;
using satellite::NodeIndex;
using satellite::NodeKind;

// The depth NO_LIMITS §7 names. One constant, so a reader can change it and
// watch what happens rather than trusting the sentence above.
constexpr int kDeep = 100000;

std::string repeated(const std::string &text, int times)
{
    std::string out;
    out.reserve(text.size() * static_cast<size_t>(times));
    for (int i = 0; i < times; i++)
        out += text;
    return out;
}

// The LAST node of a kind, which is the outermost one. THE ARENA IS BUILT
// BOTTOM UP -- a node is added when its children are finished -- so first_of()
// answers with the DEEPEST node of a nest and the root is the one added last.
// Getting that backwards is a check that passes on a chain of length one.
NodeIndex last_of(const Ast &ast, NodeKind kind)
{
    NodeIndex found = satellite::kNoNode;
    for (NodeIndex i = 1; i < ast.size(); i++)
        if (ast[i].kind == kind)
            found = i;
    return found;
}

// How far a chain of one kind runs, following one of ast.hpp's payload words.
// A LOOP AND NOT A WALK, because a test that recursed to check that nothing
// recurses would be the first thing to die.
int chain_length(const Ast &ast, NodeIndex node, NodeKind kind, char word)
{
    int seen = 0;
    while (node != satellite::kNoNode && ast[node].kind == kind) {
        seen++;
        const satellite::Node &n = ast[node];
        node = word == 'a' ? n.a : (word == 'b' ? n.b : n.c);
    }
    return seen;
}

} // namespace

void section_depth()
{
    // --- expressions: the four ways in ------------------------------------

    // NESTED BRACKETS, which is the one NO_LIMITS §2.4 measured dying first --
    // `--unparse` at 19,000, `--satc` at 20,000, the parser at 32,000.
    {
        const Program program = in_capsule(
            "satellite.variable.number n = " + repeated("(1 + ", kDeep) + "1" +
            repeated(")", kDeep));
        check(program.ok(),
              "100,000 nested brackets parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Binary) == static_cast<size_t>(kDeep),
              "and every level of them is a Binary node");
    }

    // NESTED CALLS. The argument list is a second way back into an expression
    // and had a crash of its own.
    {
        const Program program = in_capsule("satellite.variable.number n = " +
                                           repeated("f(", kDeep) + "1" +
                                           repeated(")", kDeep));
        check(program.ok(),
              "100,000 nested calls parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Call) == static_cast<size_t>(kDeep),
              "and every level of them is a Call node");
    }

    // NESTED SUBSCRIPTS, the third way, and written as a subscript INSIDE a
    // subscript rather than `x[1][2][3]` -- that one is a loop in the postfix
    // chain and never recursed.
    {
        const Program program = in_capsule("satellite.variable.number n = " +
                                           repeated("x[", kDeep) + "1" +
                                           repeated("]", kDeep));
        check(program.ok(),
              "100,000 nested subscripts parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Index) == static_cast<size_t>(kDeep),
              "and every level of them is an Index node");
    }

    // A UNARY CHAIN. `unary := ( "-" | "!" ) unary` was recursion by name, and
    // the minus signs are spaced because `--` is two Punct tokens and a reader
    // should not have to know that to read this line.
    {
        const Program program = in_capsule("satellite.variable.number n = " +
                                           repeated("- ", kDeep) + "1");
        check(program.ok(),
              "100,000 unary minuses parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Unary) == static_cast<size_t>(kDeep),
              "and every one of them is a Unary node");
    }

    // --- and the two shapes that are wide rather than deep ------------------

    // LEFT-ASSOCIATIVE AT DEPTH, which is what the operator stack replaced
    // `expression(precedence + 1)` with. `1 - 1 - 1 - ...` must come out
    // `((1 - 1) - 1) - 1`, so the chain runs down the LEFT and the right child
    // of every level is a literal. Folding on `>` instead of `>=` would build
    // the mirror image of this tree and subtraction would be wrong.
    {
        const Program program =
            in_capsule("satellite.variable.number n = 1" + repeated(" - 1", kDeep));
        check(program.ok(),
              "a 100,000-term subtraction parses -- " + program.first_error());
        const NodeIndex root = last_of(program.ast(), NodeKind::Binary);
        check(chain_length(program.ast(), root, NodeKind::Binary, 'a') == kDeep,
              "and it is left-associative all the way down");
    }

    // --- statements: blocks, and the compound forms ------------------------

    // NESTED BLOCKS. 40,000 of these killed every command in the tree before
    // this milestone, which is NO_LIMITS §2.4's second row.
    {
        const Program program = in_capsule(repeated("{\n", kDeep) +
                                           "satellite.variable.number x = 1\n" +
                                           repeated("}\n", kDeep));
        check(program.ok(),
              "100,000 nested blocks parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Block) == static_cast<size_t>(kDeep) + 1,
              "and every one of them is a Block node, plus the capsule's own");
    }

    // NESTED `if` BLOCKS, which is the same nesting with a compound statement
    // between each level -- so it is the frame that waits for its body being
    // pushed 100,000 deep rather than a block opening one.
    {
        const Program program =
            in_capsule(repeated("satellite.statement.if (1 == 1)\n{\n", kDeep) +
                       "satellite.variable.number x = 1\n" + repeated("}\n", kDeep));
        check(program.ok(),
              "100,000 nested if blocks parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::If) == static_cast<size_t>(kDeep),
              "and every one of them is an If node");
    }

    // AN `else if` CHAIN, which is the one arm of the machine that hands a
    // statement to a frame rather than a block: `else if` is an If in an If.
    {
        const Program program = in_capsule(
            repeated("satellite.statement.if (1 == 1)\n{\n}\nsatellite.statement.else\n",
                     kDeep / 10) +
            "satellite.statement.if (1 == 1)\n{\n}\n");
        check(program.ok(),
              "10,000 else-if arms parse -- " + program.first_error());
        const NodeIndex root = last_of(program.ast(), NodeKind::If);
        check(chain_length(program.ast(), root, NodeKind::If, 'c') == kDeep / 10 + 1,
              "and every arm of it hangs in the else slot, which is `c`");
    }

    // --- types, and a spacesuit's sections ---------------------------------

    // NESTED GENERIC ARGUMENTS. Twenty-four characters a level, so the depth is
    // smaller and the SOURCE is about the same size as the ones above.
    {
        constexpr int deep = kDeep / 10;
        const Program program = in_capsule(
            repeated("satellite.container.list<", deep) +
            "satellite.variable.number" + repeated(">", deep) + " n");
        check(program.ok(),
              "10,000 nested generic arguments parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Type) == static_cast<size_t>(deep) + 1,
              "and every level of them is a Type node");
    }

    // NESTED SECTIONS, the fourth cycle: a suit body holds a section and a
    // section holds a suit body.
    {
        constexpr int deep = kDeep / 10;
        const Program program =
            run("satellite.spacesuit deep_suit\n{\n" +
                repeated("satellite.protected\n{\n", deep) + repeated("}\n", deep) +
                "}\n");
        check(program.ok(),
              "10,000 nested sections parse -- " + program.first_error());
        check(count_of(program.ast(), NodeKind::Section) == static_cast<size_t>(deep),
              "and every one of them is a Section node");
    }

    // --- and the printer, which is the other half of the promise ------------

    // THE ROUND TRIP AT DEPTH. parse -> print -> parse -> print, and the two
    // printouts must be identical: unparse.hpp's own form of the check, run
    // over a tree no person wrote. It is done on the brackets because their
    // printout is LINEAR in the depth -- the printer indents four spaces per
    // block, so a 100,000-deep block prints twenty billion spaces, which is the
    // machine running out of room to hold an answer rather than a walker
    // running off its stack. NO_LIMITS §2.4 measured that and it is unchanged.
    {
        const std::string source =
            "satellite.capsule deep()\n{\n    satellite.variable.number n = " +
            repeated("(1 + ", kDeep) + "1" + repeated(")", kDeep) + "\n}\n";

        const Program first = run(source);
        check(first.ok(), "the deep program parses for the round trip");
        const std::string printed = satellite::unparse(first.ast());

        const Program second = run(printed);
        check(second.ok(),
              "and its printout parses -- " + second.first_error());
        check(satellite::unparse(second.ast()) == printed,
              "and prints back identically at 100,000 deep");
    }
}

} // namespace parser_test
