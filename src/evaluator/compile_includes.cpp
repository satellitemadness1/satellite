// Compiling `satellite.include(spaceship)` -- PLAN M25, built 2026-09-13. See
// satellite_spaceship/shape.hpp for the four spellings and
// operations_include.cpp for what the op does when it runs.
//
// AN INCLUDE COMPILES TO ONE OP, op_include, WHICH NAMES A FILE AND CARRIES THE
// ARGUMENTS. Nothing about WHICH launch the arguments go to is decided here,
// and that is the author's rule rather than a gap: "we look for where the
// arguments fit into" is a question about the VALUES, and a value is known
// when the line runs. What is decided here is everything that is not a value:
// the file, and the argument expressions, compiled once where they are written.

#include "evaluator/evaluator_internal.hpp"

#include "satellite_spaceship/shape.hpp"

#include <vector>

namespace satellite {
namespace eval {

// NOT 0 FOR "NOTHING LOADED", because 0 is a file: a spaceship that includes
// the file satl was given is including file 0, and the first version of this
// function refused exactly that include as unloaded -- found by two files that
// include each other.
uint32_t Compiler::file_included(NodeIndex include) const
{
    if (file_ >= linking_.includes.size())
        return kNotLoaded;
    for (const auto &[node, file] : linking_.includes[file_])
        if (node == include)
            return file;
    return kNotLoaded;
}

// THE ARGUMENT NODES, LAST FIRST, which is the order a task stack wants them
// pushed in so they compile first to last -- visit_reversed()'s inversion, over
// a list that is not the Include node's own.
std::vector<NodeIndex> Compiler::include_arguments(NodeIndex node) const
{
    const spaceship::Shape shape = spaceship::shape_of(ast_, node);
    std::vector<NodeIndex> out;
    for (uint32_t i = shape.argument_count(ast_); i > 0; i--)
        out.push_back(ast_.list_at(shape.arguments, i - 1));
    return out;
}

OpIndex Compiler::include(NodeIndex node)
{
    const spaceship::Shape shape = spaceship::shape_of(ast_, node);
    const uint32_t count =
        shape.named == spaceship::Named::Spaceship ? shape.argument_count(ast_) : 0;

    // INSIDE A BODY THE ARGUMENTS WERE VISITED BY THE WALK AND ARE WAITING ON
    // THE RESULTS STACK; AT THE TOP OF A FILE THERE IS NO WALK, so each is a
    // tree of its own -- global()'s shape, one argument at a time.
    std::vector<OpIndex> arguments;
    if (!tasks_.empty()) {
        arguments = take_many(count);
    } else {
        for (uint32_t i = 0; i < count; i++)
            arguments.push_back(compile_tree(ast_.list_at(shape.arguments, i)));
    }

    const uint32_t file = file_included(node);
    if (shape.named != spaceship::Named::Spaceship || file == kNotLoaded)
        return not_built(node, "`satellite.include` of a spaceship",
                         "only a program satl builds from its file loads one -- "
                         "`satl file.satl`, `--check`, `--call` and the prompt's "
                         "`run`");
    return emit(op_include, node, file, out_.add_list(arguments), count);
}

} // namespace eval
} // namespace satellite
