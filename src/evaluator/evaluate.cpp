// Compiling a resolved program. See evaluator/evaluate.hpp.
//
// THE FILE IS SHORT BECAUSE THE WALK IS NEXT DOOR. What is here is the door
// itself: the shape a caller sees, and the one lookup that turns a capsule's
// path into the index the machine calls it by.

#include "evaluator/evaluate.hpp"

#include "evaluator/evaluator_internal.hpp"

#include <utility>

namespace satellite::eval {

int Program::find(words::PathId path) const
{
    for (size_t i = 0; i < closures.capsules().size(); i++)
        if (closures.capsules()[i].path == path)
            return static_cast<int>(i);
    return -1;
}

Program compile(const Ast &ast, const resolve::Resolved &resolved, words::Words &words)
{
    Compiler compiler(ast, resolved, words);
    Program out;
    out.closures = compiler.compile();
    out.problems = compiler.take_problems();
    return out;
}

} // namespace satellite::eval
