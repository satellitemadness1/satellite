#pragma once

// The satellite Compiled Closure Tree & Inline Caches -- Milestone 9 Prototype.
//
// PLAN §8 (M9): Control Flow & Scalars.

#include "value.hpp"
#include "frame.hpp"
#include "limits.hpp"
#include "dispatch.hpp"
#include "interrupt.hpp"
#include "satellite_words/words.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace satellite {

class CapsuleClosure;

enum class ExecStatus : uint8_t {
    Normal,
    Return,
    Break,
    Continue,
    Interrupted,
};

struct ExecResult {
    ExecStatus status = ExecStatus::Normal;
    Value value = Value::nil();

    static ExecResult normal(Value v = Value::nil()) { return {ExecStatus::Normal, std::move(v)}; }
    static ExecResult return_val(Value v) { return {ExecStatus::Return, std::move(v)}; }
    static ExecResult break_loop() { return {ExecStatus::Break, Value::nil()}; }
    static ExecResult continue_loop() { return {ExecStatus::Continue, Value::nil()}; }
    static ExecResult interrupted() { return {ExecStatus::Interrupted, Value::nil()}; }
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
// Call Expression with Inline Cache (DESIGN §2.4)
// ---------------------------------------------------------------------------

class CallExprClosure : public IExprClosure {
public:
    CallExprClosure(ExprPtr target, std::vector<ExprPtr> args,
                    words::PathId static_path_id = words::kNoPath,
                    std::string callee_name = "");

    Value eval(ExecContext &ctx) const override;

    bool is_cached() const { return cached_; }
    words::PathId cached_path_id() const { return cached_path_id_; }

private:
    ExprPtr target_;
    std::vector<ExprPtr> args_;
    words::PathId static_path_id_ = words::kNoPath;
    std::string callee_name_;

    mutable bool cached_ = false;
    mutable words::PathId cached_path_id_ = words::kNoPath;
    mutable const DispatchEntry *cached_handler_ = nullptr;
    mutable const CapsuleClosure *cached_capsule_ = nullptr;
};

} // namespace satellite
