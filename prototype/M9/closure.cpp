// The satellite Compiled Closure Tree & Inline Caches implementation.
// Milestone 9 Prototype in prototype/M9.

#include "closure.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
// ExecContext Implementation
// ---------------------------------------------------------------------------

ExecContext::ExecContext(int max_depth)
    : dummy_frame_(0), max_depth_(max_depth)
{
    frames_.reserve(64);
}

Frame &ExecContext::current_frame()
{
    if (frames_.empty()) return dummy_frame_;
    return frames_.back();
}

void ExecContext::push_frame(size_t slot_count)
{
    frames_.emplace_back(slot_count);
}

void ExecContext::pop_frame()
{
    if (!frames_.empty()) {
        frames_.pop_back();
    }
}

Value ExecContext::get_global(const std::string &name) const
{
    auto it = globals_.find(name);
    if (it != globals_.end()) return it->second;
    return Value::nil();
}

void ExecContext::set_global(const std::string &name, Value val)
{
    globals_[name] = std::move(val);
}

void ExecContext::register_capsule(const std::string &name, std::shared_ptr<CapsuleClosure> cap)
{
    capsules_[name] = cap;
    if (cap && cap->path_id != words::kNoPath) {
        capsules_by_path_[cap->path_id] = cap;
    }
}

const CapsuleClosure *ExecContext::find_capsule(const std::string &name) const
{
    auto it = capsules_.find(name);
    if (it != capsules_.end()) return it->second.get();
    return nullptr;
}

const CapsuleClosure *ExecContext::find_capsule_by_path(words::PathId path_id) const
{
    auto it = capsules_by_path_.find(path_id);
    if (it != capsules_by_path_.end()) return it->second.get();
    return nullptr;
}

// ---------------------------------------------------------------------------
// CapsuleClosure Implementation
// ---------------------------------------------------------------------------

Value CapsuleClosure::invoke(ExecContext &ctx, const std::vector<Value> &args) const
{
    RecursionGuard guard(ctx.depth_ref(), ctx.max_depth());
    ctx.push_frame(slot_count);

    // Bind argument values into parameter slots
    for (size_t i = 0; i < args.size() && i < slot_count; ++i) {
        ctx.current_frame().set(i, args[i]);
    }

    Value ret_val = Value::nil();
    if (body) {
        ExecResult res = body->exec(ctx);
        if (res.status == ExecStatus::Return) {
            ret_val = std::move(res.value);
        }
    }

    ctx.pop_frame();
    return ret_val;
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
    // Evaluate arguments once before taking locks / dispatching (DESIGN §6.5)
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
        if (callee_name_.rfind("satellite.", 0) == 0) {
            std::string bare = callee_name_.substr(10);
            if (const auto *cap = ctx.find_capsule(bare)) {
                cached_capsule_ = cap;
                cached_ = true;
                return cap->invoke(ctx, eval_args);
            }
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

    // Dynamic method resolution on receiver value
    if (target_) {
        Value target_val = target_->eval(ctx);

        std::string method_name = callee_name_;

        std::string full_path;
        if (target_val.is_string()) {
            full_path = "satellite.variable.string." + method_name;
        } else if (target_val.is_number()) {
            full_path = "satellite.variable.number." + method_name;
        } else if (target_val.is_bool()) {
            full_path = "satellite.variable.bool." + method_name;
        } else if (target_val.is_list()) {
            full_path = "satellite.container.list." + method_name;
        }

        if (!full_path.empty()) {
            words::Walk w = words::walk(full_path);
            if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
                if (const auto *entry = DispatchTable::instance().get(w.id)) {
                    std::vector<Value> receiver_args;
                    receiver_args.reserve(1 + eval_args.size());
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
