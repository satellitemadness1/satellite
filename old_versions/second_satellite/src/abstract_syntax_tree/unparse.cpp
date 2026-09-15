// Printing a tree back as a program: the stack the printer keeps, and the two
// entry points. See abstract_syntax_tree/unparse.hpp for what round-trip means
// and unparse_internal.hpp for why this is three files.
//
// THE PRINTER KEEPS ITS OWN STACK, WHICH IS DESIGN §7.5's RULE AND IS WHY THIS
// MODULE READS THE WAY IT DOES. Until M8.5 `expression()` RETURNED a string and
// built the answer out of its children's -- so `--unparse` died with signal 11
// at 19,000 nested brackets, which is the shallowest crash the tree had
// (`SCRATCH.md/NO_LIMITS.md` §2.4). What replaces the recursion is a stack of
// PIECES drained into one string left to right, which is the direction the
// output was already built in.
//
// A CASE NAMES ITS PIECES IN SOURCE ORDER AND flush() REVERSES THEM, so every
// case in the two files beside this one still reads the way its output does --
// `expr(n.a); say("["); expr(n.b); say("]")` prints the target, then the
// bracket. That is the whole of the translation: nothing about WHAT is printed
// changed, and tests/parser_test/roundtrip.cpp is the check that says so,
// character for character, with tests/parser_test/depth.cpp saying it still
// holds at a hundred thousand levels.

#include "abstract_syntax_tree/unparse.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse_internal.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace satellite {

void Printer::program(NodeIndex node)
{
    work_.push_back({Step::Program, node, {}});
    run();
}

void Printer::inline_form(NodeIndex node)
{
    work_.push_back({Step::Inline, node, {}});
    run();
}

// The printer, one piece at a time. Four of the steps are the shape of the
// output rather than a part of the tree -- the indent a line opens with and the
// two moves between levels -- and those are exactly the things the recursive
// version held in the C++ stack's own shape.
void Printer::run()
{
    while (!work_.empty()) {
        const Piece piece = std::move(work_.back());
        work_.pop_back();

        switch (piece.step) {
        case Step::Text:
            out_ += piece.text;
            continue;
        case Step::Pad:
            out_.append(static_cast<size_t>(indent_) * 4, ' ');
            continue;
        case Step::Indent:
            indent_++;
            continue;
        case Step::Dedent:
            indent_--;
            continue;

        case Step::Program:
            expand_program(piece.node);
            break;
        case Step::Declaration:
            expand_declaration(piece.node);
            break;
        case Step::Statement:
            expand_statement(piece.node);
            break;
        case Step::Block:
            expand_block(piece.node);
            break;
        case Step::Members:
            expand_members(piece.node);
            break;
        case Step::Expression:
            expand_expression(piece.node);
            break;
        case Step::Type:
            expand_type(piece.node);
            break;
        case Step::Inline:
            expand_inline(piece.node);
            break;
        }
        flush();
    }
}

// The pieces one node named, pushed so they come back off in the order they
// were named. This is the only place the reversal happens.
void Printer::flush()
{
    for (size_t i = pieces_.size(); i-- > 0;)
        work_.push_back(std::move(pieces_[i]));
    pieces_.clear();
}

std::string unparse(const Ast &ast)
{
    if (ast.root() == kNoNode)
        return std::string();
    Printer printer(ast);
    printer.program(ast.root());
    return printer.take();
}

std::string unparse(const Ast &ast, NodeIndex node)
{
    Printer printer(ast);
    printer.inline_form(node);
    return printer.take();
}

} // namespace satellite
