#pragma once

// The Writer, and the three files that fill it in -- PLAN M4.5. See
// satellite_cache/cache.hpp for what a `.satc` is.
//
// THE SHAPE parser_internal.hpp ALREADY HAS, and it is here for the same
// reason: one class whose parts are separate translation units, so each file
// stays about one thing and none of them is the 500-line file the whole writer
// was before it was cut. FORMAT/CXX.md §1 asks a file to be written toward 300
// lines from its first commit and warns that "splitting at a seam chosen to
// satisfy an arithmetic rather than a subject" is its own failure -- so the
// seam is DESIGN §6's own three levels, which is the seam src/parser/ is cut on
// one milestone earlier:
//
//   write.cpp               the entry points, and everything that emits a NUMBER
//   write_declarations.cpp  §6's top_level, and the blocks that hold statements
//   write_expressions.cpp   §6's expression, and §3.1's one real decision
//
// EVERYTHING THAT TURNS A PATH INTO DIGITS IS IN ONE FILE, which is the part of
// the split worth stating. note(), fixed(), chain() and form() all live in
// write.cpp; the two files beside it decide WHERE a number goes and never how
// it is spelled. A second place that formatted a path id would be a second
// place the numbering's spelling lives.
//
// THE WRITER KEEPS ITS OWN STACK, AND IT IS unparse.cpp's -- M8.5, DESIGN §7.5.
// write.cpp's header says this printer "is unparse.cpp with the paths
// substituted", so when that one stopped recursing this one took the same
// shape: pieces named in source order by an expand_*, reversed once by flush(),
// and drained into `out_` left to right. tests/satc_test is the check that the
// bytes did not move.
//
// AND THE COMMENT COLUMN IS WHY THERE ARE TWO KINDS OF LINE ENDING. note()
// remembers a path for the column and line_end() flushes what was remembered,
// padded against the width of the line it is closing -- which a streaming
// printer can still measure, because `out_` is the line and line_start_ is
// where it began.

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace satellite::cache {

// SATC.md §1.1's comment column, counted off the worked example there.
//
// A LINE LONGER THAN THIS GETS ONE SPACE AND NOT A NEW LINE. The column is for
// reading, and §1.1 says why the comments are there at all -- "the comment
// column is how a person confirms that `1.5.1` still says what they think it
// says" -- so a wrapped comment, which would have to be reflowed on every edit,
// buys nothing that one space does not.
inline constexpr size_t kCommentColumn = 41;

// Above the highest binary level in ast.cpp's table. unparse.cpp's constant of
// the same name is the same fact for the same reason, and both read
// precedence_of() rather than a table of their own.
inline constexpr int kBindsTighterThanAny = 99;

class Writer {
public:
    Writer(const Ast &ast, const words::Words &words, const Folds &folds)
        : ast_(ast), words_(words), folds_(folds)
    {
    }

    std::string take() { return std::move(out_); }

    void program(NodeIndex node);

    // WHAT A PIECE OF THE OUTPUT CAN BE, and unparse.cpp's list plus three.
    // The three are the ones a `.satc` has and a program does not: a path
    // written as digits, and the two names whose language-owned half is
    // numbered while the user's half stays a name.
    enum class Step : uint8_t {
        Text,         // append `text`
        Newline,      // a bare newline, which is not the end of a line
        LineStart,    // the indent, and where the comment column is measured from
        LineEnd,      // the comment column, then the newline
        Indent,
        Dedent,
        Declaration,
        Statement,
        Block,
        Members,      // `node` is a ListId here, and it is the only one
        Expression,
        Type,
        Inline,
        Fixed,        // `text` is a path, written as its number
        GlobalName,
        CapsuleName,
    };

    struct Piece {
        Step step = Step::Text;
        uint32_t node = kNoNode;
        std::string text;
    };

private:
    // write_declarations.cpp -- DESIGN §6's top_level and its blocks.
    void expand_declaration(NodeIndex node);
    void capsule(NodeIndex node);
    void constructor_arguments(ListId arguments);
    void spacesuit(NodeIndex node);
    void expand_members(ListId items);
    void expand_block(NodeIndex node);
    void expand_statement(NodeIndex node);
    void expand_inline(NodeIndex node);

    // write_expressions.cpp -- §6's expression, and §3.1's decision.
    void expand_expression(NodeIndex node);
    void postfix(NodeIndex node);
    void expand_type(NodeIndex node);
    void arguments(ListId list, bool first_is_an_option);
    void bracketed(NodeIndex node, int level, bool on_the_right);

    // write.cpp -- everything that turns a path id into digits, and the stack.
    void run();
    void flush();
    void chain(const PathMatch &match, bool is_call, ListId args);
    void form(const PathMatch &match, NodeIndex argument);
    void write_fixed(const std::string &path);
    void write_global_name(NodeIndex node);
    void write_capsule_name(NodeIndex node);
    void note(words::PathId id);
    void line_end();

    // The pieces one node names, in source order. flush() is the only place
    // they are reversed, which is what lets every case below read forwards.
    void say(std::string text) { pieces_.push_back({Step::Text, kNoNode, std::move(text)}); }
    void newline() { pieces_.push_back({Step::Newline, kNoNode, {}}); }
    void line_starts() { pieces_.push_back({Step::LineStart, kNoNode, {}}); }
    void line_ends() { pieces_.push_back({Step::LineEnd, kNoNode, {}}); }
    void indent() { pieces_.push_back({Step::Indent, kNoNode, {}}); }
    void dedent() { pieces_.push_back({Step::Dedent, kNoNode, {}}); }
    void decl(NodeIndex node) { pieces_.push_back({Step::Declaration, node, {}}); }
    void stmt(NodeIndex node) { pieces_.push_back({Step::Statement, node, {}}); }
    void block_of(NodeIndex node) { pieces_.push_back({Step::Block, node, {}}); }
    void members_of(ListId items) { pieces_.push_back({Step::Members, items, {}}); }
    void expr(NodeIndex node) { pieces_.push_back({Step::Expression, node, {}}); }
    void type_of(NodeIndex node) { pieces_.push_back({Step::Type, node, {}}); }
    void inline_of(NodeIndex node) { pieces_.push_back({Step::Inline, node, {}}); }
    void fixed(std::string path) { pieces_.push_back({Step::Fixed, kNoNode, std::move(path)}); }
    void global_name(NodeIndex node) { pieces_.push_back({Step::GlobalName, node, {}}); }
    void capsule_name(NodeIndex node) { pieces_.push_back({Step::CapsuleName, node, {}}); }

    // EVERY NOTE IS MADE WHERE ITS NUMBER IS PRINTED, which is what keeps the
    // comment column in the order the line reads. write.cpp's old header
    // warned that `expression(a) + " = " + expression(b)` is indeterminately
    // sequenced in C++17 and would emit the two paths in whichever order the
    // compiler chose; a piece is expanded when its output position is reached,
    // so the hazard is gone rather than avoided by discipline. That is why the
    // three note-making forms above are pieces and not strings computed early.

    std::string text(NodeIndex node) const { return std::string(ast_.text_of(node)); }

    const Ast &ast_;
    const words::Words &words_;

    // WHAT RESOLVE FOLDED, WHICH IS THE ONLY THING THIS WRITER KNOWS THAT THE
    // M4.5 ONE DID NOT. Empty for every caller that writes without resolving,
    // and cache.hpp's Folds says why that is a default rather than an error.
    const Folds &folds_;
    std::string out_;
    std::string comment_;
    int indent_ = 0;

    // Where the line being written began, for the comment column.
    size_t line_start_ = 0;

    std::vector<Piece> work_;
    std::vector<Piece> pieces_;
};

} // namespace satellite::cache
