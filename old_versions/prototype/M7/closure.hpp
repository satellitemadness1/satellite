#pragma once

// The satellite Compiled Closure Tree & Inline Caches -- Milestone 7 Prototype.
//
// DESIGN §2.3 & PLAN §8: Fast callable closure tree compiled from arena AST.
// DESIGN §2.4: Inline caches on call sites caching PathId and handler pointers.
// Multi-threaded safe, zero AST re-traversal, no bytecode VM overhead.

#include "value.hpp"
#include "frame.hpp"
#include "limits.hpp"
#include "dispatch.hpp"
#include "satellite_words/words.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace satellite {

class CapsuleClosure;
class SpacesuitClosure;

enum class ExecStatus : uint8_t {
    Normal,
    Return,
    Break,
    Continue,
};

struct ExecResult {
    ExecStatus status = ExecStatus::Normal;
    Value value = Value::nil();

    static ExecResult normal(Value v = Value::nil()) { return {ExecStatus::Normal, std::move(v)}; }
    static ExecResult return_val(Value v) { return {ExecStatus::Return, std::move(v)}; }
};

// ---------------------------------------------------------------------------
// Execution Context
// ---------------------------------------------------------------------------

class ExecContext {
public:
    ExecContext(int max_depth = compute_derived_max_depth());

    Frame &current_frame();
    void push_frame(size_t slot_count);
    void pop_frame();

    Value get_global(const std::string &name) const;
    void set_global(const std::string &name, Value val);

    void record_output(const std::string &line) { output_lines_.push_back(line); }
    const std::vector<std::string> &output_lines() const { return output_lines_; }
    void clear_output() { output_lines_.clear(); }

    int depth() const { return depth_; }
    int &depth_ref() { return depth_; }
    int max_depth() const { return max_depth_; }
    void set_max_depth(int limit) { max_depth_ = limit; }

    void register_capsule(const std::string &name, std::shared_ptr<CapsuleClosure> cap);
    const CapsuleClosure *find_capsule(const std::string &name) const;
    const CapsuleClosure *find_capsule_by_path(words::PathId path_id) const;

private:
    std::vector<Frame> frames_;
    Frame dummy_frame_;
    std::unordered_map<std::string, Value> globals_;
    std::vector<std::string> output_lines_;
    std::unordered_map<std::string, std::shared_ptr<CapsuleClosure>> capsules_;
    std::unordered_map<words::PathId, std::shared_ptr<CapsuleClosure>> capsules_by_path_;
    int depth_ = 0;
    int max_depth_ = DEFAULT_MAX_DEPTH;
};

// ---------------------------------------------------------------------------
// Expression Closures
// ---------------------------------------------------------------------------

class IExprClosure {
public:
    virtual ~IExprClosure() = default;
    virtual Value eval(ExecContext &ctx) const = 0;
};

using ExprPtr = std::unique_ptr<IExprClosure>;

// ---------------------------------------------------------------------------
// Statement Closures
// ---------------------------------------------------------------------------

class IStmtClosure {
public:
    virtual ~IStmtClosure() = default;
    virtual ExecResult exec(ExecContext &ctx) const = 0;
};

using StmtPtr = std::unique_ptr<IStmtClosure>;

// ---------------------------------------------------------------------------
// Capsule Closure
// ---------------------------------------------------------------------------

class CapsuleClosure {
public:
    std::string name;
    size_t param_count = 0;
    size_t slot_count = 0;
    words::PathId path_id = words::kNoPath;
    StmtPtr body;

    Value invoke(ExecContext &ctx, const std::vector<Value> &args) const;
};

// ---------------------------------------------------------------------------
// Inline Cache Call Closure (DESIGN §2.4)
// ---------------------------------------------------------------------------

class CallExprClosure : public IExprClosure {
public:
    CallExprClosure(ExprPtr target, std::vector<ExprPtr> args,
                    words::PathId static_path_id = words::kNoPath,
                    std::string callee_name = "");

    Value eval(ExecContext &ctx) const override;

    // Cache inspection for testing verification
    bool is_cached() const { return cached_; }
    words::PathId cached_path_id() const { return cached_path_id_; }

private:
    ExprPtr target_;
    std::vector<ExprPtr> args_;
    words::PathId static_path_id_ = words::kNoPath;
    std::string callee_name_;

    // Inline cache cells (DESIGN §2.4)
    mutable bool cached_ = false;
    mutable words::PathId cached_path_id_ = words::kNoPath;
    mutable const HandlerEntry *cached_handler_ = nullptr;
    mutable const CapsuleClosure *cached_capsule_ = nullptr;
};

} // namespace satellite

