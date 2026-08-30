#pragma once

// The satellite Evaluator & Runtime Engine -- Milestone 9 Prototype.
//
// DESIGN §7 & §8: Executes compiled closure trees with isolated frames,
// bounded recursion, and dispatch through handlers[path_id].

#include "compiler.hpp"
#include "closure.hpp"
#include "value.hpp"

#include <memory>
#include <string>
#include <vector>

namespace satellite {

class Evaluator {
public:
    explicit Evaluator(CompiledProgram program, int max_depth = compute_derived_max_depth());

    ExecResult run_top_level();
    Value invoke(const std::string &capsule_name, const std::vector<Value> &args = {});
    Value run_main(const std::vector<Value> &args = {});

    const std::vector<std::string> &output() const { return ctx_.output_lines(); }
    void clear_output() { ctx_.clear_output(); }

    ExecContext &context() { return ctx_; }
    const ExecContext &context() const { return ctx_; }

    void set_max_depth(int limit) { ctx_.set_max_depth(limit); }

private:
    CompiledProgram program_;
    ExecContext ctx_;
};

} // namespace satellite

