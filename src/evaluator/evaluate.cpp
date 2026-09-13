// Compiling a resolved program. See evaluator/evaluate.hpp.
//
// THE FILE IS SHORT BECAUSE THE WALK IS NEXT DOOR. What is here is the door
// itself: the shape a caller sees, and the one lookup that turns a capsule's
// path into the index the machine calls it by.

#include "evaluator/evaluate.hpp"

#include "evaluator/evaluator_internal.hpp"

#include <memory>
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
    out.deferred = compiler.take_deferred_refusals();
    return out;
}

Program compile_run(const std::vector<Unit> &units, words::Words &words,
                    const std::vector<uint32_t> &setup_order)
{
    Linking linking;
    for (const Unit &unit : units) {
        linking.files.push_back(unit.resolved);
        linking.includes.push_back(unit.includes);
    }

    std::vector<std::unique_ptr<Compiler>> compilers;
    for (uint32_t f = 0; f < units.size(); f++)
        compilers.push_back(std::make_unique<Compiler>(
            linking, f, *units[f].ast, *units[f].resolved, words));

    // EVERY FILE REGISTERED BEFORE ANY IS COMPILED -- the forward reference
    // across files, which is DESIGN §7.3's within one.
    for (auto &compiler : compilers)
        compiler->register_file();
    for (uint32_t f = 0; f < units.size(); f++)
        linking.out.files()[f].name = units[f].name;
    for (auto &compiler : compilers)
        compiler->compile_fields();
    for (auto &compiler : compilers)
        compiler->compile_file();

    Program out;
    out.closures = Compiler::finish_run(linking, setup_order);
    out.problems = std::move(linking.problems);
    out.deferred = std::move(linking.deferred_refusals);
    return out;
}

} // namespace satellite::eval
