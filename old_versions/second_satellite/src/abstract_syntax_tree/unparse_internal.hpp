#pragma once

// The Printer, and the three files that fill it in. See
// abstract_syntax_tree/unparse.hpp for what round-trip means and why it is M4's
// done-when.
//
// THE SHAPE satellite_cache/write_internal.hpp ALREADY HAS, AND THAT IS THE
// POINT RATHER THAN A COINCIDENCE. write.cpp's header says the `.satc` writer
// "is unparse.cpp with the paths substituted" and calls the drift between them a
// risk answered by a test; the two now have the same seam as well as the same
// walk, so a reader comparing them is comparing files that line up:
//
//   unparse.cpp               the stack, and the two entry points
//   unparse_declarations.cpp  DESIGN §6's top_level, and the blocks
//   unparse_expressions.cpp   §6's expression and §6's type
//
// THE SPLIT WAS TAKEN AT M8.5 AND THE REASON IS THE LINE RULE. Making the
// printer keep its own stack (DESIGN §7.5) took one file from 341 lines to 565,
// which is 452 of code and the widest in the tree -- PLAN §3 asks a file to be
// written toward 300 and warns that a seam chosen to satisfy an arithmetic is a
// seam in the wrong place. This one is not: it is DESIGN §6's own three levels,
// which is where src/parser/ and the writer are already cut.

#include "abstract_syntax_tree/ast.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace satellite {

// Above the highest binary level in ast.cpp's table, so that "does this child
// bind looser than its parent" is true of every operator when the parent is a
// unary. satellite_cache/write_internal.hpp's constant of the same name is the
// same fact for the same reason, and both read precedence_of() rather than a
// table of their own.
inline constexpr int kBindsTighterThanAny = 99;

class Printer {
public:
    explicit Printer(const Ast &ast) : ast_(ast) {}

    std::string take() { return std::move(out_); }

    void program(NodeIndex node);

    // One node inline: an expression, or one of the three statements that can
    // be written inside a `for` head. A block-shaped statement has no inline
    // form and comes back as its kind.
    void inline_form(NodeIndex node);

    // WHAT A PIECE OF THE OUTPUT CAN BE. Four of these twelve do not name a
    // node at all -- text to append, the indentation a line opens with, and the
    // two that move the indent -- and they are the things recursion held in the
    // shape of the call stack itself.
    enum class Step : uint8_t {
        Text,         // append `text`
        Pad,          // the indentation a line opens with, read when it runs
        Indent,
        Dedent,
        Program,
        Declaration,
        Statement,
        Block,
        Members,      // `node` is a ListId here, and it is the only one
        Expression,
        Type,
        Inline,
    };

    struct Piece {
        Step step = Step::Text;
        uint32_t node = kNoNode;
        std::string text;
    };

private:
    void run();
    void flush();

    // unparse_declarations.cpp -- DESIGN §6's top_level and its blocks.
    void expand_program(NodeIndex node);
    void expand_declaration(NodeIndex node);
    void capsule(NodeIndex node);
    void constructor_arguments(ListId arguments);
    void spacesuit(NodeIndex node);
    void expand_members(ListId items);
    void expand_block(NodeIndex node);
    void expand_statement(NodeIndex node);
    void expand_inline(NodeIndex node);

    // unparse_expressions.cpp -- §6's expression, and §6's type.
    void expand_expression(NodeIndex node);
    void expand_type(NodeIndex node);
    void bracketed(NodeIndex node, int level, bool on_the_right);

    // The pieces one node names, in source order. flush() is the only place
    // they are reversed, which is what lets every case read forwards.
    void say(std::string text) { pieces_.push_back({Step::Text, kNoNode, std::move(text)}); }
    void pad() { pieces_.push_back({Step::Pad, kNoNode, {}}); }
    void newline() { say("\n"); }
    void indent() { pieces_.push_back({Step::Indent, kNoNode, {}}); }
    void dedent() { pieces_.push_back({Step::Dedent, kNoNode, {}}); }
    void decl(NodeIndex node) { pieces_.push_back({Step::Declaration, node, {}}); }
    void stmt(NodeIndex node) { pieces_.push_back({Step::Statement, node, {}}); }
    void block_of(NodeIndex node) { pieces_.push_back({Step::Block, node, {}}); }
    void members_of(ListId items) { pieces_.push_back({Step::Members, items, {}}); }
    void expr(NodeIndex node) { pieces_.push_back({Step::Expression, node, {}}); }
    void type_of(NodeIndex node) { pieces_.push_back({Step::Type, node, {}}); }
    void inline_of(NodeIndex node) { pieces_.push_back({Step::Inline, node, {}}); }

    // A STRING AND NOT A string_view, because every use of it is a
    // concatenation and a view does not concatenate with a literal. The cost is
    // one copy per token printed, in the one class in the tree whose job is to
    // build a string.
    std::string text(NodeIndex node) const { return std::string(ast_.text_of(node)); }

    const Ast &ast_;
    std::string out_;
    int indent_ = 0;

    // THE DEPTH OF THE PRINT LIVES HERE. `work_` is what is still to be
    // printed, deepest last; `pieces_` is the one node being expanded, named in
    // source order and emptied by flush() before the next is read.
    std::vector<Piece> work_;
    std::vector<Piece> pieces_;
};

} // namespace satellite
