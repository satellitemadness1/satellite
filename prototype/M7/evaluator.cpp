// The satellite Evaluator & Runtime Engine implementation.

#include "evaluator.hpp"

#include <iostream>

namespace satellite {

Evaluator::Evaluator(CompiledProgram program, int max_depth)
    : program_(std::move(program)), ctx_(max_depth)
{
    // Register all compiled capsules in the execution context
    for (const auto &pair : program_.capsules) {
        ctx_.register_capsule(pair.first, pair.second);
    }
}

ExecResult Evaluator::run_top_level()
{
    for (const auto &stmt : program_.top_level) {
        if (!stmt) continue;
        ExecResult res = stmt->exec(ctx_);
        if (res.status == ExecStatus::Return)
            return res;
    }
    return ExecResult::normal();
}

Value Evaluator::invoke(const std::string &capsule_name, const std::vector<Value> &args)
{
    const auto *cap = ctx_.find_capsule(capsule_name);
    if (!cap) {
        // Try searching for bare name if prefixed with satellite.
        if (capsule_name.rfind("satellite.", 0) == 0) {
            std::string bare = capsule_name.substr(10);
            cap = ctx_.find_capsule(bare);
        }
    }
    if (cap) {
        return cap->invoke(ctx_, args);
    }
    return Value::nil();
}

Value Evaluator::run_main(const std::vector<Value> &args)
{
    // Try satellite.main first, then main
    const auto *cap = ctx_.find_capsule("satellite.main");
    if (!cap) cap = ctx_.find_capsule("main");

    if (cap) {
        return cap->invoke(ctx_, args);
    }
    return Value::nil();
}

} // namespace satellite

