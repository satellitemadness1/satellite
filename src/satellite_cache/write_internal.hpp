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
// the split worth stating. note(), fixed(), numbered_chain() and numbered_form()
// all live in write.cpp; the two files beside it decide WHERE a number goes and
// never how it is spelled. A second place that formatted a path id would be a
// second place the numbering's spelling lives.

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

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
    Writer(const Ast &ast, const words::Words &words) : ast_(ast), words_(words) {}

    std::string take() { return std::move(out_); }

    // write_declarations.cpp -- DESIGN §6's top_level and its blocks.
    void program(NodeIndex node);
    void declaration(NodeIndex node);
    void capsule(NodeIndex node);
    void spacesuit(NodeIndex node);
    void members(ListId items);
    void block(NodeIndex node);
    void statement(NodeIndex node);
    std::string one(NodeIndex node);

    // write_expressions.cpp -- §6's expression, and §3.1's decision.
    std::string expression(NodeIndex node);
    std::string postfix(NodeIndex node);
    std::string type(NodeIndex node);
    std::string arguments(ListId list);
    std::string bracketed(NodeIndex node, int level, bool on_the_right);

    // write.cpp -- everything that turns a path id into digits.
    std::string numbered_chain(const PathMatch &match, bool is_call, ListId args);
    std::string numbered_form(const PathMatch &match, NodeIndex argument);
    std::string fixed(const std::string &path);
    std::string global_name(NodeIndex node);
    std::string capsule_name(NodeIndex node);
    void note(words::PathId id);
    void line(const std::string &text);

    std::string text(NodeIndex node) const { return std::string(ast_.text_of(node)); }

    const Ast &ast() const { return ast_; }
    const Node &node(NodeIndex index) const { return ast_[index]; }

    // NOT PRIVATE, AND THE ALTERNATIVE WAS WORSE. parser_internal.hpp keeps
    // its state private and its parts are members of one class declared there;
    // this class is the same arrangement, and the four fields below are read by
    // all three files. Accessors over them would be four functions that exist
    // to satisfy a keyword -- the class is internal to one module, its header
    // says so in its name, and nothing outside satellite_cache/ can see it.
    const Ast &ast_;
    const words::Words &words_;
    std::string out_;
    std::string comment_;
    int indent_ = 0;
};

} // namespace satellite::cache
