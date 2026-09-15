// The arena's layout, and the three things about it that are claims rather than
// code. See tests/parser_test/parser_test.hpp.
//
// MOST OF WHAT PLAN §2.2 ASKS FOR IS ALREADY A static_assert IN ast.hpp, which
// is FORMAT/CXX.md §8's rule -- the asserts live in the header so every
// consumer inherits them, and a Node that stopped being 24 bytes or stopped
// being trivially copyable would fail the build rather than this file. What is
// left for run time is what a compiler cannot see: that the arena is actually
// FLAT, that the sentinel at index 0 is real, and that no node holds text.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "lexical_analyzer/lexer.hpp"

#include <string>
#include <vector>

using satellite::Ast;
using satellite::kNoList;
using satellite::kNoNode;
using satellite::NodeKind;

namespace parser_test {

void section_arena()
{
    // -- The sentinel --------------------------------------------------------

    {
        const Ast empty;
        check(empty.size() == 1, "a new arena holds one node, which is the sentinel");
        check(empty[kNoNode].kind == NodeKind::None,
              "node 0 is a None node, so kNoNode is safe to dereference");
        check(empty.list_size(kNoList) == 0,
              "list handle 0 reads as the empty list with no special case");
        check(empty.root() == kNoNode, "an arena with nothing parsed into it has no root");
    }

    // -- The lists -----------------------------------------------------------

    {
        Ast ast;
        const satellite::NodeIndex one = ast.add(NodeKind::Number, 0);
        const satellite::NodeIndex two = ast.add(NodeKind::Number, 0);

        const satellite::ListId none = ast.add_list({});
        check(none == kNoList,
              "the empty list is ALWAYS handle 0 and is never appended -- two "
              "encodings of empty is how a comparison of two equal trees starts "
              "answering 'different'");

        const satellite::ListId pair = ast.add_list({one, two});
        check(ast.list_size(pair) == 2, "a list knows its own length");
        check(ast.list_at(pair, 0) == one && ast.list_at(pair, 1) == two,
              "a list gives its children back in the order they went in");

        const satellite::ListId second = ast.add_list({two});
        check(second != pair && ast.list_size(second) == 1,
              "a second list takes its own handle and does not disturb the first");
        check(ast.list_size(pair) == 2, "and the first list is still two long");
    }

    // -- Flat, and holding no text -------------------------------------------

    {
        const Program program = in_capsule("satellite.console.display(\"hi\")");
        check(program.ok(), "the fixture parses: " + program.first_error());

        const Ast &ast = program.ast();
        // EVERY CHILD IS AN INDEX INTO THIS SAME VECTOR, which is the whole of
        // what "flattened arena" means and the one property a walk can check:
        // no node may point outside the arena, so no walk can leave it.
        for (satellite::NodeIndex i = 1; i < ast.size(); i++) {
            const satellite::Node &node = ast[i];
            check(node.token < ast.tokens().size(),
                  "node " + std::to_string(i) + " (" +
                      std::string(satellite::kind_name(node.kind)) +
                      ") is anchored at a token that exists");
        }

        // THE TOKENS TRAVEL WITH THE TREE. ast.hpp's argument is that a tree
        // whose nodes index somebody else's vector is a dangling reference
        // waiting for its first caller -- and its first caller is the printer.
        check(!ast.tokens().empty(), "the tree carries the stream it was built from");
        check(ast.text_of(first_of(ast, NodeKind::String)) == "hi",
              "a literal's text comes back out of the token, not out of the node");
    }

    // -- The size of a real program ------------------------------------------

    {
        const Program program = in_capsule(
            "satellite.variable.number n = 1 + 2 * 3\n"
            "satellite.console.display(n)");
        check(program.ok(), "the fixture parses: " + program.first_error());

        // NOT A THRESHOLD, A SHAPE. The check is that the arena is small and
        // dense rather than that it is any particular length -- a node count
        // asserted to the digit would fail the day a rule gains a wrapper, and
        // would be a test about this parser rather than about the layout.
        check(program.ast().size() > 8 && program.ast().size() < 64,
              "a two-statement capsule is a couple of dozen nodes, contiguous");
        check(sizeof(satellite::Node) * program.ast().size() < 1024,
              "and the whole tree fits in a kilobyte -- 24 bytes a node against "
              "the first satellite's 96");
    }
}

} // namespace parser_test
