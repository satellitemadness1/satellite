// What `satl --resolve` prints. See name_resolver/dump.hpp for why the pass has
// a consumer at all.
//
// TWO THINGS AND A COUNT. The frames are DESIGN §7.2's decision made visible --
// which name is in which slot of which capsule -- and the paths are DESIGN
// §6.3's walk made visible, every chain in the file beside the number it came
// out as. The count is MILESTONES/M4.5.md §5's clause: how many of those
// numbers were walked for and how many the `.satc` already knew.
//
// SORTED BY WHERE THEY WERE WRITTEN AND NOT BY NODE INDEX. The arena is filled
// bottom-up by a recursive-descent parser, so index order puts a call's
// arguments before the call -- deterministic, and not an order anybody reading
// their own program would recognise.

#include "name_resolver/dump.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite::resolve {

namespace {

// A path as it is spelled, whichever half of the numbering it is in.
//
// BOTH HALVES, BECAUSE A DUMP THAT PRINTED ONLY THE LANGUAGE'S WOULD BE MISSING
// THE ONES THIS MILESTONE IS ABOUT. words_runtime.hpp's table is the two: the
// language's words are frozen and quotable, and a user's capsule is numbered
// when it is met and is valid inside one run. `satl --resolve` prints a run.
std::string spelled(const words::Words &words, words::PathId id)
{
    if (words::is_language_word(id))
        return words::path_text(static_cast<words::NodeId>(id));
    return words::path_text(words.parent_of(id)) + "." +
           std::string(words.name_of(id));
}

std::string numbered(const words::Words &words, words::PathId id)
{
    if (words::is_language_word(id))
        return words::number_text(static_cast<words::NodeId>(id));
    return words::number_text(words.parent_of(id)) + " " +
           std::to_string(words.number_of(id));
}

// AT LEAST ONE SPACE, ALWAYS. A column that only pads up TO a width runs into
// the next one on the row that reaches it exactly -- `local_user_input` is 16
// characters, and in a 16-wide column it printed
// `local_user_inputsatellite.variable.string`. Found by running the command on
// example/advanced.satl, which is why every milestone in this tree builds one.
std::string padded(std::string text, size_t width)
{
    text.append(text.size() < width ? width - text.size() : 1, ' ');
    return text;
}

std::string counted(unsigned long long many, const char *one, const char *more)
{
    return std::to_string(many) + " " + (many == 1 ? one : more);
}

struct Row {
    uint32_t line = 0;
    uint32_t at = 0;
    std::string path;
    std::string number;
    Origin origin = Origin::Parsed;
};

const char *where_from(Origin origin)
{
    switch (origin) {
    case Origin::Cached: return "from the .satc";
    case Origin::Walked: return "walked";
    case Origin::Bound:  return "from its declaration";
    case Origin::Parsed: break;
    }
    // THE PARSER'S OWN, AND SAYING SO IS THE POINT. A capsule takes the next
    // number free under its parent when the name is first MET (WORD_NUMBERS
    // §3), which is parse time -- so this row cost this pass nothing and must
    // not be counted as though it had.
    return "at parse time";
}

} // namespace

std::string dump_text(const std::string &path, const Ast &ast,
                      const words::Words &words, const Resolved &resolved)
{
    std::string out = "satl resolved " + path + ".\n";

    for (const Frame &frame : resolved.frames) {
        out += "\n  " + spelled(words, frame.capsule) + "  (" +
               numbered(words, frame.capsule) + ")\n";

        if (frame.names.empty())
            out += "    no slots -- this capsule takes nothing and declares "
                   "nothing\n";

        for (size_t i = 0; i < frame.names.size(); i++) {
            // THE PARAMETERS AND THE LOCALS ARE ONE NUMBERING AND THE COLUMN
            // SAYS WHICH IS WHICH. DESIGN §7.1: "parameters are locals too",
            // and that is not a simplification -- it is the sentence that made
            // the first satellite's registry unfixable, because `arguments`
            // would have been a program-wide static. Two numberings here would
            // put that back.
            out += "    " + padded(std::to_string(i), 4) +
                   padded(std::string(frame.names[i]), 18) +
                   padded(unparse(ast, frame.types[i]), 52);
            out += i < frame.parameters ? "parameter" : "local";
            if (frame.arguments == static_cast<Slot>(i))
                out += ", and the arguments object -- DESIGN 7.7";
            out += "\n";
        }
    }

    // §7.4 MADE VISIBLE, AND IT IS THE ONE CLAUSE A DUMP CAN SHOW AND A TEST
    // CANNOT PHRASE ANY OTHER WAY. A redeclaration takes a FRESH slot, so a
    // capsule that declares `x` twice has two rows called `x` -- and if it ever
    // has one, the rule has been quietly dropped and a list built by that idiom
    // reads back as n copies of its last element with no error anywhere.

    std::vector<Row> rows;
    for (NodeIndex node = 1; node < ast.size(); node++) {
        const Info &info = resolved.at(node);
        if (info.path == words::kNoPath)
            continue;
        const Token &token = ast.token_of(node);
        rows.push_back({token.line, token.start, spelled(words, info.path),
                        numbered(words, info.path), info.origin});
    }
    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
        return a.at < b.at;
    });

    // AND WHOSE LINES THEY ARE, said out loud on a warm run. The tree came out
    // of a `.satc` and its spans index into THAT text -- no comments, and blank
    // lines where the writer put them -- so line 5 of this table is line 5 of
    // the cache and not of the file the reader has open.
    // programs/resolve_command.cpp takes the stronger step for a DIAGNOSTIC,
    // which is a caret and cannot be qualified by a sentence; a table can.
    out += "\n  the paths";
    if (resolved.from_cache != 0)
        out += "     -- the lines are the `.satc`'s, which is what was read";
    out += "\n";
    if (rows.empty())
        out += "    none -- this file names nothing the numbering has\n";
    for (const Row &row : rows) {
        out += "    " + padded(std::to_string(row.line), 6) +
               padded(row.path, 52) + padded(row.number, 18) +
               where_from(row.origin) + "\n";
    }

    out += "\n  " + padded("the walk", 18) +
           counted(resolved.walked, "path walked", "paths walked") + ", " +
           counted(resolved.from_cache, "taken from the .satc",
                   "taken from the .satc") +
           "\n";
    out += "  " + padded("the frames", 18) +
           counted(resolved.frames.size(), "capsule", "capsules") + "\n";

    // PASS 2 SAYS SO OUT LOUD, which is the whole of what makes it a named hole
    // rather than a pass that does not work. DESIGN §7.3 puts spacesuits second
    // in the order and PLAN §8 puts them at M26; a resolver that skipped them
    // in silence would be indistinguishable from one that resolved them wrong.
    if (resolved.spacesuits != 0)
        out += "  " + padded("spacesuits", 18) +
               counted(resolved.spacesuits, "seen", "seen") +
               " and none resolved -- what is inside one lands at M26\n";

    return out;
}

} // namespace satellite::resolve
