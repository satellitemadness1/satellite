// The satellite Compiled Closure Tree & Inline Caches implementation.
// Milestone 11 Prototype in prototype/M11.

#include "closure.hpp"
#include "search.hpp"

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
    // For satellite.main, if no args provided but parameter expected, default to empty list
    if (args.empty() && slot_count > 0 && name == "main") {
        ctx.current_frame().set(0, Value(std::make_shared<const List>()));
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
// Method Signature Resolver Helper
// ---------------------------------------------------------------------------

static std::string resolve_method_full_path(const std::string &type_prefix,
                                           const std::string &method_name,
                                           size_t arg_count)
{
    if (type_prefix == "satellite.container.list") {
        if (method_name == "append") return "satellite.container.list.append";
        if (method_name == "size") return "satellite.container.list.size";
        if (method_name == "sort") {
            return (arg_count == 0) ? "satellite.container.list.sort()" : "satellite.container.list.sort(direction)";
        }
        if (method_name == "sort_down") {
            return (arg_count == 0) ? "satellite.container.list.sort_down()" : "satellite.container.list.sort_down(key)";
        }
        if (method_name == "sort_up") return "satellite.container.list.sort_up(key)";
        if (method_name == "contains") return "satellite.container.list.contains(x)";
        if (method_name == "index_of") return "satellite.container.list.index_of(x)";
        if (method_name == "empty") return "satellite.container.list.empty";
        if (method_name == "clear") return "satellite.container.list.clear";
        if (method_name == "first") return "satellite.container.list.first";
        if (method_name == "last") return "satellite.container.list.last";
        if (method_name == "truncate") return "satellite.container.list.truncate(n)";
        if (method_name == "reserve") return "satellite.container.list.reserve(n)";
        if (method_name == "remove_first") return "satellite.container.list.remove_first()";
        if (method_name == "remove_last") return "satellite.container.list.remove_last()";
        if (method_name == "remove_at") return "satellite.container.list.remove_at(n)";
        if (method_name == "remove") return "satellite.container.list.remove(x)";
        if (method_name == "insert") return "satellite.container.list.insert(n, x)";
        if (method_name == "join") return "satellite.container.list.join(separator)";
        if (method_name == "reverse") return "satellite.container.list.reverse";
        if (method_name == "sum") return "satellite.container.list.sum";
        if (method_name == "max") return "satellite.container.list.max";
        if (method_name == "min") return "satellite.container.list.min";
    } else if (type_prefix == "satellite.container.map") {
        if (method_name == "set") return "satellite.container.map.set(k, v)";
        if (method_name == "get") return "satellite.container.map.get(k)";
        if (method_name == "has") return "satellite.container.map.has(k)";
        if (method_name == "size") return "satellite.container.map.size";
        if (method_name == "empty") return "satellite.container.map.empty";
        if (method_name == "clear") return "satellite.container.map.clear";
        if (method_name == "remove") return "satellite.container.map.remove(k)";
        if (method_name == "keys") return "satellite.container.map.keys";
        if (method_name == "values") return "satellite.container.map.values";
    } else if (type_prefix == "satellite.variable.string") {
        if (method_name == "size") return "satellite.variable.string.size";
        if (method_name == "empty") return "satellite.variable.string.empty";
        if (method_name == "find") return "satellite.variable.string.find(x)";
        if (method_name == "contains") return "satellite.variable.string.contains(x)";
        if (method_name == "substring") return "satellite.variable.string.substring(start, end)";
        if (method_name == "starts_with") return "satellite.variable.string.starts_with(x)";
        if (method_name == "ends_with") return "satellite.variable.string.ends_with(x)";
        if (method_name == "lower") return "satellite.variable.string.lower";
        if (method_name == "upper") return "satellite.variable.string.upper";
        if (method_name == "split") return "satellite.variable.string.split(separator)";
        if (method_name == "trim") return "satellite.variable.string.trim";
        if (method_name == "replace") return "satellite.variable.string.replace(a, b)";
        if (method_name == "to_number") return "satellite.variable.string.to_number";
        if (method_name == "append") return "satellite.variable.string.append(x)";
        if (method_name == "clear") return "satellite.variable.string.clear";
        if (method_name == "at") return "satellite.variable.string.at(n)";
    }
    return type_prefix + "." + method_name;
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
    std::vector<Value> eval_args;
    eval_args.reserve(args_.size());
    for (const auto &arg : args_) {
        eval_args.push_back(arg ? arg->eval(ctx) : Value::nil());
    }

    // 1. Check Inline Cache (DESIGN §2.4)
    if (cached_) {
        if (cached_handler_) {
            if (cached_handler_->binds_receiver && target_) {
                Value target_val = target_->eval(ctx);
                std::vector<Value> receiver_args;
                receiver_args.reserve(1 + eval_args.size());
                receiver_args.push_back(std::move(target_val));
                for (auto &a : eval_args) receiver_args.push_back(std::move(a));
                return cached_handler_->fn(ctx, receiver_args);
            }
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
            if (entry->binds_receiver && target_) {
                Value target_val = target_->eval(ctx);
                std::vector<Value> receiver_args;
                receiver_args.reserve(1 + eval_args.size());
                receiver_args.push_back(std::move(target_val));
                for (auto &a : eval_args) receiver_args.push_back(std::move(a));
                return entry->fn(ctx, receiver_args);
            }
            return entry->fn(ctx, eval_args);
        }
    }

    // Check if callee_name_ is a dotted path (e.g. satellite.system.threshold)
    if (!callee_name_.empty() && callee_name_.rfind("satellite.", 0) == 0) {
        std::string full_path = callee_name_;
        if (full_path == "satellite.system.threshold") {
            full_path = eval_args.empty() ? "satellite.system.threshold()" : "satellite.system.threshold(n)";
        }
        words::Walk w = words::walk(full_path);
        if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
            if (const auto *entry = DispatchTable::instance().get(w.id)) {
                cached_handler_ = entry;
                cached_path_id_ = w.id;
                cached_ = true;
                return entry->fn(ctx, eval_args);
            }
        }
    }

    // Dynamic method resolution on receiver value
    if (target_) {
        Value target_val = target_->eval(ctx);
        std::string method_name = callee_name_;

        if (method_name == "search" && (target_val.is_list() || std::holds_alternative<MapRef>(target_val))) {
            auto target_ptr = std::make_shared<Value>(target_val);
            auto pattern_ptr = eval_args.empty() ? std::make_shared<Value>(Value::nil()) : std::make_shared<Value>(eval_args[0]);
            std::string error;
            ValuePtr res = search_collect(target_ptr, pattern_ptr, true, ctx.max_depth(), error);
            if (res) return *res;
            return Value(std::make_shared<const List>());
        }

        std::string type_prefix;
        if (target_val.is_string()) {
            type_prefix = "satellite.variable.string";
        } else if (target_val.is_number()) {
            type_prefix = "satellite.variable.number";
        } else if (target_val.is_bool()) {
            type_prefix = "satellite.variable.bool";
        } else if (target_val.is_list()) {
            type_prefix = "satellite.container.list";
        } else if (std::holds_alternative<MapRef>(target_val)) {
            type_prefix = "satellite.container.map";
        }

        if (!type_prefix.empty()) {
            std::string full_path = resolve_method_full_path(type_prefix, method_name, eval_args.size());
            words::Walk w = words::walk(full_path);
            if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
                if (const auto *entry = DispatchTable::instance().get(w.id)) {
                    cached_handler_ = entry;
                    cached_path_id_ = w.id;
                    cached_ = true;
                    std::vector<Value> receiver_args;
                    receiver_args.reserve(1 + eval_args.size());
                    receiver_args.push_back(std::move(target_val));
                    for (auto &a : eval_args) receiver_args.push_back(std::move(a));
                    return entry->fn(ctx, receiver_args);
                }
            }
        }
    }

    return Value::nil();
}

} // namespace satellite

