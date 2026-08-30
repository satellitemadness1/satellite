// The satellite Compiled Closure Tree & Inline Caches implementation.

#include "closure.hpp"

#include <iostream>
#include <stdexcept>

namespace satellite {

// ---------------------------------------------------------------------------
// ExecContext Implementation
// ---------------------------------------------------------------------------

ExecContext::ExecContext(int max_depth)
    : max_depth_(max_depth)
{
}

Frame &ExecContext::current_frame()
{
    if (frames_.empty())
        return dummy_frame_;
    return frames_.back();
}

void ExecContext::push_frame(size_t slot_count)
{
    frames_.emplace_back(slot_count);
}

void ExecContext::pop_frame()
{
    if (!frames_.empty())
        frames_.pop_back();
}

Value ExecContext::get_global(const std::string &name) const
{
    auto it = globals_.find(name);
    if (it != globals_.end())
        return it->second;
    return Value::nil();
}

void ExecContext::set_global(const std::string &name, Value val)
{
    globals_[name] = std::move(val);
}

void ExecContext::register_capsule(const std::string &name, std::shared_ptr<CapsuleClosure> cap)
{
    capsules_[name] = cap;
    if (cap->path_id != words::kNoPath)
        capsules_by_path_[cap->path_id] = cap;
}

const CapsuleClosure *ExecContext::find_capsule(const std::string &name) const
{
    auto it = capsules_.find(name);
    if (it != capsules_.end())
        return it->second.get();
    return nullptr;
}

const CapsuleClosure *ExecContext::find_capsule_by_path(words::PathId path_id) const
{
    auto it = capsules_by_path_.find(path_id);
    if (it != capsules_by_path_.end())
        return it->second.get();
    return nullptr;
}

// ---------------------------------------------------------------------------
// Capsule Closure Invocation
// ---------------------------------------------------------------------------

Value CapsuleClosure::invoke(ExecContext &ctx, const std::vector<Value> &args) const
{
    // Bounded recursion check (DESIGN §7.5)
    RecursionGuard guard(ctx.depth_ref(), ctx.max_depth());

    // Allocate fresh stack frame (DESIGN §7.2)
    ctx.push_frame(slot_count);

    // Bind parameters
    for (size_t i = 0; i < args.size() && i < param_count; ++i) {
        ctx.current_frame().set(i, args[i]);
    }

    ExecResult res = ExecResult::normal();
    if (body) {
        res = body->exec(ctx);
    }

    ctx.pop_frame();
    return res.value;
}

// ---------------------------------------------------------------------------
// Inline Cache Call Closure Implementation (DESIGN §2.4)
// ---------------------------------------------------------------------------

CallExprClosure::CallExprClosure(ExprPtr target, std::vector<ExprPtr> args,
                                 words::PathId static_path_id,
                                 std::string callee_name)
    : target_(std::move(target)), args_(std::move(args)),
      static_path_id_(static_path_id), callee_name_(std::move(callee_name))
{
}

Value CallExprClosure::eval(ExecContext &ctx) const
{
    // Evaluate arguments once before taking any locks / dispatching (DESIGN §6.5)
    std::vector<Value> eval_args;
    eval_args.reserve(args_.size());
    for (const auto &arg : args_) {
        eval_args.push_back(arg ? arg->eval(ctx) : Value::nil());
    }

    // 1. Check Inline Cache (DESIGN §2.4)
    if (cached_) {
        if (cached_handler_) {
            return cached_handler_->fn(ctx, eval_args);
        }
        if (cached_capsule_) {
            return cached_capsule_->invoke(ctx, eval_args);
        }
    }

    // 2. First Execution: Resolve & Populate Inline Cache

    // Check if target is a known capsule
    if (!callee_name_.empty()) {
        if (const auto *cap = ctx.find_capsule(callee_name_)) {
            cached_capsule_ = cap;
            cached_ = true;
            return cap->invoke(ctx, eval_args);
        }
    }

    // Check static PathId from resolver / folding
    if (static_path_id_ != words::kNoPath) {
        if (const auto *entry = DispatchTable::instance().get(static_path_id_)) {
            cached_handler_ = entry;
            cached_path_id_ = static_path_id_;
            cached_ = true;
            return entry->fn(ctx, eval_args);
        }
    }

    // Dynamic resolution fallback
    if (target_) {
        Value target_val = target_->eval(ctx);
        // If target is a list or object method call
        if (target_val.is_list() && !callee_name_.empty()) {
            std::string full_path = "satellite.container.list." + callee_name_;
            words::Walk w = words::walk(full_path);
            if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
                if (const auto *entry = DispatchTable::instance().get(w.id)) {
                    std::vector<Value> receiver_args;
                    receiver_args.push_back(target_val);
                    for (auto &a : eval_args) receiver_args.push_back(std::move(a));
                    return entry->fn(ctx, receiver_args);
                }
            }
        }
    }

    return Value::nil();
}

} // namespace satellite

