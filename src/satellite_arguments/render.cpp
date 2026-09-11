// The arguments object as the text a person reads -- DESIGN §7.7's
// "displaying it bare prints all of it".
//
// WHY THIS IS NOT IN satellite_value/render.cpp WITH EVERY OTHER ARM. The
// renderer there turns a value into characters and knows nothing about the
// registry; this walks words.def for the object's children and asks
// system_facts for their answers, which is the module's own subject. The value
// renderer keeps one line -- the arm -- and calls in here.
//
// TWO COLUMNS, ONE ENTRY PER LINE, NAMES LEFT-ALIGNED -- v1's printer, ported.
// It is the one rendering in the language that is not a single line, and the
// reason is what §1.1's tie-breaker says it is for: this is the whole of what
// the runtime knows, handed over at once, and thirty-odd facts on one line
// would be a thing nobody could read.

#include "satellite_arguments/rows.hpp"

#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <utility>
#include <vector>

namespace satellite::arguments {

namespace {

using words::NodeId;

// Every fact under `arguments`, as name and text, in REGISTRY ORDER and never
// in a list written here -- read_group()'s argument one level up. The name is
// the path RELATIVE TO `arguments`, dotted, so what a reader sees printed is
// exactly what they would type: `machine.threads`, not `threads`.
void collect(NodeId parent, const std::string &prefix,
             std::vector<std::pair<std::string, std::string>> &into)
{
    for (words::PathId child = words::first_child(parent);
         child != words::kNoPath; child = words::next_sibling(child)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<NodeId>(child));
        if (spelling.empty() || spelling == "()")
            continue;

        const std::string name = prefix + std::string(spelling);
        Value value;
        if (answer_of(child, &value))
            into.push_back({name, text_of(value)});

        // A GROUP'S CHILDREN ARE PRINTED AND THE GROUP IS NOT, because the
        // group's own answer is a map of exactly these lines -- printing both
        // would say everything twice. `interpreter` is the exception in both
        // directions: it has an answer of its own AND children, so the line
        // above emitted it and this line walks into it.
        //
        // THE RECURSION IS TWO DEEP AND CANNOT BE MORE, which is why it is
        // recursion at all -- DESIGN §7.5 forbids a C++ stack over a depth the
        // USER chooses, and this one is over words.def, whose shape is fixed
        // at compile time. WORD_NUMBERS §4 calls this subtree the deepest in
        // the language at six numbers, and `arguments` sits at four of them.
        collect(static_cast<NodeId>(child), name + ".", into);
    }
}

} // namespace

std::string object_text(const Arguments &body)
{
    std::vector<std::pair<std::string, std::string>> lines;

    // THE COMMAND LINE FIRST, IN ARGV ORDER, which is the order it was typed
    // in and the order `[i]` reads it in.
    for (const CommandLineWord &word : body.words)
        lines.push_back({word.name, text_of(word.value)});

    collect(NodeId::LIBRARY_MAIN_ARGUMENTS, std::string(), lines);

    size_t width = 0;
    for (const auto &line : lines)
        if (line.first.size() > width)
            width = line.first.size();

    std::string out;
    for (size_t i = 0; i < lines.size(); i++) {
        if (i != 0)
            out += "\n";
        out += lines[i].first;
        out.append(width - lines[i].first.size() + 2, ' ');
        out += lines[i].second;
    }
    return out;
}

} // namespace satellite::arguments
