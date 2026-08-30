// The `.satc` a program comes out as: the entry points, and everything that
// turns a path id into digits. See satellite_cache/cache.hpp for what a `.satc`
// is and satellite_cache/write_internal.hpp for why the writer is three files.
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
// SUBEXPRESSIONS GO INTO NAMED LOCALS BEFORE THEY ARE JOINED, in all three
// files, and that is not style. note() appends to the line's comment as a side
// effect, and C++17 leaves the operands of `a + b` INDETERMINATELY SEQUENCED --
// so `expression(n.a) + " = " + expression(n.b)` may run the right side first
// and emit a comment column whose paths are in the wrong order, on one compiler
// and not another. SATC.md §5.2 asks for byte-identical output so that a
// `.satc` is something a test can compare; a printer whose output depends on
// the compiler's argument order cannot promise that.

#include "satellite_cache/cache.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_cache/write_internal.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/version.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace satellite::cache {

// A matched row, with whatever of the call still has to be written.
//
// THREE READINGS OF ONE COLUMN, and PathMatch::shape_arity is the column. At -1
// the row has no argument list of its own and the call belongs to the program,
// so it is printed exactly as written -- empty parentheses included, because
// `satellite.console.display` and `satellite.console.display()` are different
// programs and a cache may not merge them. At 0 the row IS the empty call and
// 1.5.2 already says `input()`. Above 0 the row names the shape and the
// arguments carry the values.
std::string Writer::numbered_chain(const PathMatch &match, bool is_call, ListId args)
{
    note(match.id);
    std::string out = number_text(match.id);
    if (!is_call || match.absorbs_argument || match.shape_arity == 0)
        return out;
    return out + "(" + arguments(args) + ")";
}

// The Include and Return forms, whose one argument is a node and not a list.
// Same three readings; a second function rather than an overload because the
// parser gives those two statements a child instead of an argument list, and
// two functions whose signatures differ only by a trailing default were
// ambiguous at every call site -- found on this file's first compile.
std::string Writer::numbered_form(const PathMatch &match, NodeIndex argument)
{
    if (!match.found())
        return std::string();
    note(match.id);
    std::string out = number_text(match.id);
    if (match.absorbs_argument || match.shape_arity <= 0)
        return out;
    return out + "(" + expression(argument) + ")";
}

// A path the parser gave a node of its own, looked up as the text it always is.
// Falls back to the text on a walk that fails, which a tree the parser built
// cannot produce -- said rather than asserted, because the fallback is what
// keeps a hand-built tree printable instead of silently empty.
std::string Writer::fixed(const std::string &path)
{
    const words::Walk found = words::walk(path);
    if (found.error != words::WalkError::NONE)
        return path;
    note(found.id);
    return number_text(found.id);
}

// `satellite.library.<name>`: the language-owned prefix numbered, the user's
// segment left as a name. SATC.md §3's rule, and §3's own example --
// `1.2 fact(1.6.4 n)` -- is the same shape one level up.
std::string Writer::global_name(NodeIndex node)
{
    const words::PathId id = ast_[node].a;
    if (words::is_language_word(id)) {
        note(id);
        return number_text(id);
    }

    // THE PARENT IS ASKED FOR RATHER THAN SPELLED OUT, which is what the run's
    // numbering is passed in for. A user's PathId is valid inside one run only
    // (words_runtime.hpp), so the object that allocated the name is the only
    // thing that can say what it hangs under -- and that answer is
    // language-owned, which is what makes it legal to write down.
    const words::NodeId parent = words_.parent_of(id);
    if (parent == words::NodeId::NONE)
        return fixed("satellite.library") + "." + text(node);
    note(static_cast<words::PathId>(parent));
    return number_text(static_cast<words::PathId>(parent)) + "." + text(node);
}

// A capsule's name: `satellite.main` is the one the language owns a number for,
// and every other is the user's and stays a name.
std::string Writer::capsule_name(NodeIndex node)
{
    const words::PathId id = ast_[node].a;
    if (!words::is_language_word(id))
        return text(node);
    note(id);
    return number_text(id);
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

void Writer::line(const std::string &text)
{
    const size_t indent = static_cast<size_t>(indent_) * 4;
    out_.append(indent, ' ');
    out_ += text;
    if (!comment_.empty()) {
        const size_t width = indent + text.size();
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

std::string body_text(const Ast &ast, const words::Words &words)
{
    if (ast.root() == kNoNode)
        return std::string();
    Writer writer(ast, words);
    writer.program(ast.root());
    return writer.take();
}

std::string satc_text(const Ast &ast, const words::Words &words,
                      const Source &source)
{
    return header_text(source) + "\n" + body_text(ast, words);
}

} // namespace satellite::cache
