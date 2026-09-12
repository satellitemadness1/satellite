// The `.satc` a program comes out as: the entry points, the stack the printer
// keeps, and everything that turns a path id into digits. See
// satellite_cache/cache.hpp for what a `.satc` is and
// satellite_cache/write_internal.hpp for why the writer is three files.
//
// THIS IS abstract_syntax_tree/unparse.cpp WITH THE PATHS SUBSTITUTED, and
// saying so is the whole safety argument for having written it twice. The two
// printers walk the same tree in the same order and differ only where a
// language-owned word is reached; everything else -- the brackets precedence
// puts back, the literals printed from their token's `text`, the four-space
// block -- is the same code written again. That is a drift risk and it is
// answered by a TEST rather than by structure: tests/satc_test checks the
// number against the numbering on every line of every acceptance program.
// Sharing one printer behind a mode flag was the other candidate and was
// rejected because nearly every line would branch -- every statement form emits
// a number instead of a keyword -- so the shared version would have been two
// printers wearing one name, with the equality nobody checked.
//
// THAT SENTENCE IS WHY THIS FILE STOPPED RECURSING WHEN unparse.cpp DID.
// M8.5, DESIGN §7.5: a walker may not use the C++ stack for a depth the user's
// program chooses, and `--satc` died at 20,000 nested brackets. The rewrite is
// the one next door -- pieces named in source order, reversed once by flush(),
// drained left to right -- because "the same printer twice" is a property worth
// keeping through a change of shape.
//
// ~~SUBEXPRESSIONS GO INTO NAMED LOCALS BEFORE THEY ARE JOINED~~ -- AND THAT
// RULE IS GONE WITH THE RECURSION, which is the one thing the rewrite deleted
// rather than moved. It read: note() appends to the line's comment as a side
// effect, and C++17 leaves the operands of `a + b` INDETERMINATELY SEQUENCED,
// so `expression(n.a) + " = " + expression(n.b)` may run the right side first
// and emit a comment column whose paths are in the wrong order, on one compiler
// and not another. A piece is expanded when its output position is reached, so
// the order of the comment column is now the order of the output by
// construction. SATC.md §5.2's byte-identical promise is kept by the machine
// instead of by a discipline every future line had to remember.

#include "satellite_cache/cache.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_cache/write_internal.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/version.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace satellite::cache {

// The printer, one piece at a time. Four of the steps are the shape of the
// output rather than a part of the tree -- the indent a line opens with, the
// two ends of a line, and the moves between them -- and those are exactly the
// things the recursive version held in the C++ stack's own shape.
void Writer::run()
{
    flush();
    while (!work_.empty()) {
        const Piece piece = std::move(work_.back());
        work_.pop_back();

        switch (piece.step) {
        case Step::Text:
            out_ += piece.text;
            continue;
        case Step::Newline:
            out_ += "\n";
            continue;
        case Step::LineStart:
            line_start_ = out_.size();
            out_.append(static_cast<size_t>(indent_) * 4, ' ');
            continue;
        case Step::LineEnd:
            line_end();
            continue;
        case Step::Indent:
            indent_++;
            continue;
        case Step::Dedent:
            indent_--;
            continue;

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
        case Step::Fixed:
            write_fixed(piece.text);
            break;
        case Step::GlobalName:
            write_global_name(piece.node);
            break;
        case Step::CapsuleName:
            write_capsule_name(piece.node);
            break;
        }
        flush();
    }
}

void Writer::flush()
{
    for (size_t i = pieces_.size(); i-- > 0;)
        work_.push_back(std::move(pieces_[i]));
    pieces_.clear();
}

// A matched row, with whatever of the call still has to be written.
//
// THREE READINGS OF ONE COLUMN, and PathMatch::shape_arity is the column. At -1
// the row has no argument list of its own and the call belongs to the program,
// so it is printed exactly as written -- empty parentheses included, because
// `satellite.console.display` and `satellite.console.display()` are different
// programs and a cache may not merge them. At 0 the row IS the empty call and
// 1.5.2 already says `input()`. Above 0 the row names the shape and the
// arguments carry the values.
void Writer::chain(const PathMatch &match, bool is_call, ListId args)
{
    note(match.id);
    say(number_text(match.id));
    if (!is_call || match.absorbs_argument || match.shape_arity == 0)
        return;
    say("(");

    // FALSE, AND IT IS A FACT ABOUT WHERE THE FOLD LIVES RATHER THAN A DEFAULT.
    // `fold_option()` is reached from one place -- names.cpp's
    // call_target_done(), on a SELECTOR whose receiver has a declared type --
    // so a language path never folds, whatever it is spelled with.
    // `satellite.file.open("f", "read")` is `1 8 2` with `mode` as a written
    // argument and the string stays a string, which persistence.satl's `.satc`
    // shows on its own line.
    arguments(args, false);
    say(")");
}

// The Include and Return forms, whose one argument is a node and not a list.
// Same three readings; a second function rather than an overload because the
// parser gives those two statements a child instead of an argument list, and
// two functions whose signatures differ only by a trailing default were
// ambiguous at every call site -- found on this file's first compile.
void Writer::form(const PathMatch &match, NodeIndex argument)
{
    if (!match.found())
        return;
    note(match.id);
    say(number_text(match.id));
    if (match.absorbs_argument || match.shape_arity <= 0)
        return;
    say("(");
    expr(argument);
    say(")");
}

// A path the parser gave a node of its own, looked up as the text it always is.
// Falls back to the text on a walk that fails, which a tree the parser built
// cannot produce -- said rather than asserted, because the fallback is what
// keeps a hand-built tree printable instead of silently empty.
void Writer::write_fixed(const std::string &path)
{
    const words::Walk found = words::walk(path);
    if (found.error != words::WalkError::NONE) {
        out_ += path;
        return;
    }
    note(found.id);
    out_ += number_text(found.id);
}

// `satellite.library.<name>`: the language-owned prefix numbered, the user's
// segment left as a name. SATC.md §3's rule, and §3's own example --
// `1.2 fact(1.6.4 n)` -- is the same shape one level up.
void Writer::write_global_name(NodeIndex node)
{
    const words::PathId id = ast_[node].a;
    if (words::is_language_word(id)) {
        note(id);
        out_ += number_text(id);
        return;
    }

    // THE PARENT IS ASKED FOR RATHER THAN SPELLED OUT, which is what the run's
    // numbering is passed in for. A user's PathId is valid inside one run only
    // (words_runtime.hpp), so the object that allocated the name is the only
    // thing that can say what it hangs under -- and that answer is
    // language-owned, which is what makes it legal to write down.
    const words::PathId parent = words_.parent_of(id);
    if (parent == static_cast<words::PathId>(words::NodeId::NONE)) {
        write_fixed("satellite.library");
        out_ += "." + text(node);
        return;
    }

    // A USER'S NAME UNDER A USER'S NAME IS WRITTEN AS TEXT ALL THE WAY DOWN --
    // M26, where a spacesuit's members became the first names whose parent is
    // itself a user name. The paragraph above is why: a user's PathId is valid
    // inside one run only, so the only part of this that may be written down is
    // the part the LANGUAGE owns, and a user parent owns none of it. Writing
    // the spelling is what `write_capsule_name` already does one function down
    // for exactly this reason, and SATC.md §3's rule is kept rather than bent.
    if (!words::is_language_word(parent)) {
        out_ += std::string(words_.name_of(parent)) + "." + text(node);
        return;
    }
    note(parent);
    out_ += number_text(parent) + "." + text(node);
}

// A capsule's name: `satellite.main` is the one the language owns a number for,
// and every other is the user's and stays a name.
void Writer::write_capsule_name(NodeIndex node)
{
    const words::PathId id = ast_[node].a;
    if (!words::is_language_word(id)) {
        out_ += text(node);
        return;
    }
    note(id);
    out_ += number_text(id);
}

// A path this line used, remembered for the comment column.
//
// A USER'S NUMBER IS NEVER NOTED, and the guard is the same predicate
// words_nodes.hpp says "a caller that is about to write a PathId down anywhere
// that outlives the run has to ask first". The comment column outlives the run
// -- it is in the file -- so a user's PathId noted here would put a number that
// is only true today next to a name that is true always.
void Writer::note(words::PathId id)
{
    if (!words::is_language_word(id))
        return;
    if (!comment_.empty())
        comment_ += ' ';
    comment_ += words::path_text(static_cast<words::NodeId>(id));
}

// The end of a line: what this line noted, padded to the column, then the
// newline. The width is measured off `out_` rather than off a string that was
// built first, which is the one thing this had to learn to do streaming.
void Writer::line_end()
{
    if (!comment_.empty()) {
        const size_t width = out_.size() - line_start_;
        out_.append(width < kCommentColumn ? kCommentColumn - width : 1, ' ');
        out_ += "// ";
        out_ += comment_;
        comment_.clear();
    }
    out_ += "\n";
}

std::string header_text(const Source &source)
{
    return "satc " + std::to_string(kFormatVersion) + "\n" +
           "words " SATELLITE_VERSION "." SATELLITE_REVISION " " +
           words::digest_text() + "\n" +
           "source " + source.name + " " + std::to_string(source.mtime) + " " +
           std::to_string(source.size) + "\n";
}

std::string body_text(const Ast &ast, const words::Words &words,
                      const Folds &folds)
{
    if (ast.root() == kNoNode)
        return std::string();
    Writer writer(ast, words, folds);
    writer.program(ast.root());
    return writer.take();
}

std::string satc_text(const Ast &ast, const words::Words &words,
                      const Source &source, const Folds &folds)
{
    return header_text(source) + "\n" + body_text(ast, words, folds);
}

} // namespace satellite::cache
