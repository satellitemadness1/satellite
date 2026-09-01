// The closure tree, printed. See evaluator/dump.hpp.
//
// ONE OP PER LINE, IN ARENA ORDER, WHICH IS BUILD ORDER AND NOT SOURCE ORDER.
// The arena is built bottom up -- a node is added when its children are
// finished -- so op 1 is the deepest thing in the first expression and the last
// op is the program's top level. MILESTONES/M8.5.md §4.2 is the receipt for why
// that is worth saying out loud: a test walked the AST from the wrong end and
// passed on every tree, because `first_of` answers with the DEEPEST node of a
// nest. A listing that quietly renumbered into source order would hide the same
// thing here.

#include "evaluator/dump.hpp"

#include "evaluator/evaluator_internal.hpp"
#include "satellite_value/render.hpp"

#include <cstdio>
#include <string>

namespace satellite::eval {

namespace {

std::string padded(const std::string &text, size_t width)
{
    std::string out = text;
    while (out.size() < width)
        out += ' ';
    return out;
}

std::string number(uint32_t value) { return std::to_string(value); }

// A capsule's name, whichever half of the numbering it is in.
//
// TWO LOOKUPS AND NOT ONE, which words_runtime.hpp draws the line under:
// name_of() is "the user's own spelling" and answers EMPTY for a language word,
// because a language word "has a text rather than a name and the two are kept
// apart" -- only one of them may ever be written into a `.satc` (SATC.md §3).
// A listing wants whichever exists.
std::string name_of(const words::Words &words, words::PathId path)
{
    if (path == words::kNoPath)
        return "-";
    if (words::is_language_word(path))
        return std::string(words::path_text(static_cast<words::NodeId>(path)));
    const std::string_view name = words.name_of(path);
    return name.empty() ? "?" : std::string(name);
}

} // namespace

std::string dump_text(const std::string &path, const Ast &ast,
                      const words::Words &words, const Program &program)
{
    const Compiled &closures = program.closures;
    std::string out = "the closure tree of " + path + "\n\n";

    out += "  capsules\n";
    if (closures.capsules().empty())
        out += "    none -- this file declares no `satellite.capsule`\n";
    for (const Capsule &capsule : closures.capsules()) {
        out += "    " + padded(name_of(words, capsule.path), 24) + " frame " +
               number(capsule.slots) + " slots, " + number(capsule.parameters) +
               " of them parameters, body op " + number(capsule.body) + "\n";
    }

    out += "\n  ops\n";
    for (OpIndex i = 1; i < closures.size(); i++) {
        const Op &op = closures[i];
        const NodeIndex node = closures.node_of(i);
        out += "    " + padded(number(i), 6) + padded(op_name(op.fn), 14);
        out += padded(number(op.a) + " " + number(op.b) + " " + number(op.c) +
                          " " + number(op.d),
                      18);

        // THE SOURCE LINE, WHICH IS WHAT MAKES THE LISTING READABLE AT ALL.
        // An op index means nothing on its own; the line it came from is what a
        // person matches against the file they are looking at.
        if (node != kNoNode)
            out += "line " + number(ast.token_of(node).line);
        out += "\n";

        // A refusal says what it will refuse and when it will stop, because
        // that is the one op whose OPERANDS are words rather than indices.
        if (op.fn == op_refuse)
            out += "           " + closures.text(op.a) + " -- " +
                   closures.text(op.b) + "\n";
    }

    out += "\n  the whole file\n";
    out += "    " + number(static_cast<uint32_t>(closures.size() - 1)) +
           " ops, " + number(closures.globals()) + " globals, " +
           number(closures.caches()) + " call sites with an inline cache\n";
    out += "    top level is op " + number(closures.top()) + "\n";

    // WHAT AN Op COSTS, PRINTED RATHER THAN CLAIMED. closure.hpp asserts the 24
    // bytes and this is where a reader sees what the assert bought: the whole
    // compiled form of a program, in one number, against the arena it came from.
    out += "    " + number(static_cast<uint32_t>(closures.size() * 24)) +
           " bytes of ops against " + number(static_cast<uint32_t>(ast.size() * 24)) +
           " bytes of tree\n";
    return out;
}

} // namespace satellite::eval
