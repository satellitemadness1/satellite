// The satellite Evaluator & Runtime Engine implementation.
// Milestone 11 Prototype in prototype/M11.

#include "evaluator.hpp"

namespace satellite {

Evaluator::Evaluator(CompiledProgram program, int max_depth)
    : program_(std::move(program)),
      owned_ctx_(std::make_unique<ExecContext>(max_depth)),
      ctx_(*owned_ctx_)
{
    for (const auto &pair : program_.capsules) {
        ctx_.register_capsule(pair.first, pair.second);
    }
}

Evaluator::Evaluator(CompiledProgram program, ExecContext &shared_ctx)
    : program_(std::move(program)),
      ctx_(shared_ctx)
{
    for (const auto &pair : program_.capsules) {
        ctx_.register_capsule(pair.first, pair.second);
    }
}

ExecResult Evaluator::run_top_level()
{
    ExecResult last = ExecResult::normal();
    for (const auto &stmt : program_.top_level) {
        if (!stmt) continue;
        last = stmt->exec(ctx_);
        if (last.status == ExecStatus::Return || last.status == ExecStatus::Interrupted)
            return last;
    }
    return last;
}

Value Evaluator::invoke(const std::string &capsule_name, const std::vector<Value> &args)
{
    const auto *cap = ctx_.find_capsule(capsule_name);
    if (!cap) {
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
    const auto *cap = ctx_.find_capsule("satellite.main");
    if (!cap) cap = ctx_.find_capsule("main");

    if (cap) {
        if (args.empty() && cap->param_count > 0) {
            std::vector<Value> default_args = { Value(std::make_shared<const List>()) };
            return cap->invoke(ctx_, default_args);
        }
        return cap->invoke(ctx_, args);
    }
    return Value::nil();
}

} // namespace satellite

