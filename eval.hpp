#pragma once

#include "ast.hpp"
#include "env.hpp"
#include "value.hpp"

#include <string>
#include <unordered_map>
#include <vector>

// The satellite evaluator: a tree walker over the AST the parser produces.
// No bytecode, no lowering pass — the tree stays the tree.
//
// Storage comes in exactly two kinds, and which one a name uses is decided
// before the walk starts, by resolve() (env.hpp):
//
//   * a TOP-LEVEL variable lives in satellite.library under one namespace,
//     which is what makes
//
//         satellite.variable.number x = 1
//
//     observable as satellite.library.main.x — the check that the evaluator
//     wrote into the real Library rather than a side table (§10);
//
//   * a CAPSULE LOCAL lives in a frame slot, reached by integer index, with
//     one frame per activation (§6).
//
// The second kind is not an optimisation. Routing locals through the Library
// instead is measured in §6 to make a recursive fact() return 1 for every
// input, and to make eight threads running a capsule with no recursion and no
// shared state produce 1585 wrong results out of 1600: the Library gives
// atomicity, and locals need isolation.
//
// An Evaluator therefore requires the ResolveResult for the Program it is
// about to run, and resolve() must already have finished — see the contract on
// Name::slot in ast.hpp.

namespace satellite {

struct EvalError {
    std::string message;
    Span span;
};

// satellite.return is a status, not a C++ exception. Measured in §10: 8.5 ns
// as an enum against 1537 ns thrown — 181× — which at one return per call is
// seconds of pure unwinding in any recursive program. Exceptions stay reserved
// for genuine errors, and errors here are not exceptional enough to throw:
// they are recorded and unwound by returning null.
enum class Flow {
    Normal,
    Return,
};

class Evaluator {
public:
    // `resolved` must come from resolve() run over the same Program, and must
    // outlive the Evaluator: it owns the slot counts and declared types every
    // capsule call is built from.
    //
    // `ns` is the satellite.library namespace top-level variables land in;
    // `echo` makes a top-level expression statement append its value to the
    // output, which is what makes the REPL print 1 for a bare `x`.
    explicit Evaluator(const ResolveResult &resolved, std::string ns = "main",
                       bool echo = false);

    void run(const Program &program);

    // Runs the program's top-level statements, then calls satellite.main if it
    // defines one, binding `args` to main's parameter.
    //
    // §11 promised M3 would replace the entry point's namespace-bound parameter
    // with a real frame and no satellite source changes; this is that. The
    // entry point is now an ordinary capsule call that the runtime happens to
    // make, so `argz` is a frame slot like any other parameter — the checks on
    // main's SIGNATURE stay here, because only the runtime knows what it is
    // about to pass.
    void run_entry(const Program &program, const List &args);

    bool ok() const { return errors_.empty(); }
    const std::vector<EvalError> &errors() const { return errors_; }

    // Everything satellite.console.display wrote, plus the echoed values.
    const std::string &output() const { return output_; }

    // The operand of the last satellite.return(...) that ran.
    ValuePtr returned() const { return returned_; }

private:
    // A storage location. Mutating methods need one: there is nowhere to write
    // back the result of foo().append(x).
    //
    // `index` is the discriminant, and it carries the same three values
    // Name::slot does, so a Slot is read straight off a resolved name:
    //
    //     >= 0          slot `index` of the current frame — a capsule local
    //     SLOT_GLOBAL   satellite.library.<ns>.<name>
    //
    // A frame slot leaves `ns` empty. `name` is filled either way, because
    // every diagnostic wants the name the user wrote rather than a number.
    // A third kind joined the two: a FIELD of the receiver, reached by index
    // into the current object exactly as a local is reached by index into the
    // current frame. It is the same trade for the same reason — resolve()
    // already did the search, so nothing here is looked up by string.
    struct Slot {
        int index = SLOT_GLOBAL;
        std::string ns;
        std::string name;
        bool valid = false;

        bool in_frame() const { return index >= 0; }
        bool in_field() const { return is_field_slot(index); }

        // satellite.library's key, and therefore meaningless for the other two.
        std::string key() const { return ns + "." + name; }
    };

    Flow exec(const Stmt &stmt);
    Flow exec_block(const Block &block);

    ValuePtr eval(const Expr &expr);
    ValuePtr eval_member(const Member &node, Span span);
    ValuePtr eval_call(const Call &node, Span span);
    ValuePtr eval_index(const Index &node, Span span);
    ValuePtr eval_slice(const Slice &node, Span span);
    ValuePtr eval_unary(const Unary &node, Span span);
    ValuePtr eval_binary(const Binary &node, Span span);

    bool eval_args(const std::vector<ExprPtr> &args,
                   std::vector<ValuePtr> &argv);
    ValuePtr call_method(const ValuePtr &recv, const Expr &recv_expr,
                         const std::string &name,
                         const std::vector<ValuePtr> &argv, Span span);
    ValuePtr call_mutator(const Expr &recv_expr, const std::string &name,
                          const std::vector<ValuePtr> &argv, Span span);
    ValuePtr call_module(const std::vector<std::string> &path,
                         const std::vector<ValuePtr> &argv, Span span);

    // One activation: a frame built on the C++ stack, arguments bound into
    // slots [0, param_count), the body walked with that frame current.
    //
    // `self` and `suit` are null for an ordinary capsule and set for a method,
    // which is the whole of the difference between the two. `suit` is the
    // spacesuit that DECLARED the method rather than the one the object is an
    // instance of: it is what an access check compares against, and after an
    // override those are not the same suit.
    ValuePtr call_capsule(const CapsuleInfo &info, const std::string &name,
                          const std::vector<ValuePtr> &argv, Span span,
                          ObjectPtr self = nullptr,
                          const SpacesuitInfo *suit = nullptr);

    // A fresh instance: field slots sized from the layout, every initialiser
    // run with the new object as receiver, then the constructor chain from the
    // superclass down. `argv` goes to the most derived constructor.
    ValuePtr construct(const SpacesuitInfo &suit,
                       const std::vector<ValuePtr> &argv, Span span);

    // Dispatch on the OBJECT's spacesuit, not on the caller's — which is what
    // makes an override visible to the inherited method that calls it.
    ValuePtr call_object_method(const ObjectPtr &object, const std::string &name,
                                const std::vector<ValuePtr> &argv, Span span);

    Slot slot_of(const Expr &expr);
    ValuePtr read_slot(const Slot &slot, const std::string &shown, Span span);
    bool write_slot(const Slot &slot, ValuePtr value, Span span);

    // The frame cell a slot names, or null (with the error already recorded)
    // when there is no frame to name it in.
    ValuePtr *frame_cell(const Slot &slot, const std::string &shown, Span span);

    // The same, for a field of the current receiver. Returns the atomic itself
    // rather than a Value, because a mutator has to read-modify-write it under
    // the object's write_lock.
    std::atomic<ValuePtr> *field_cell(const Slot &slot, const std::string &shown,
                                      Span span);

    // The declared type of a binding, or null when it has none. A frame slot's
    // type comes from the capsule (it is static, one per slot, shared by every
    // activation); a global's comes from declared_.
    const Type *declared_type(const Slot &slot) const;

    bool condition(const Expr &expr, bool &out);
    void fail(Span span, std::string message);
    bool failed() const { return !errors_.empty(); }

    const ResolveResult &resolved_;
    std::string ns_;
    bool echo_ = false;
    std::string output_;
    std::vector<EvalError> errors_;
    ValuePtr returned_;

    // The activation being walked, or null at the top level — which is the one
    // switch that decides whether a name means a frame slot or a Library key.
    // Top-level code has no frame, so it stays on the Library path and
    // satellite.library.main.x keeps working.
    //
    // A raw pointer, not ownership: the frame is a local in call_capsule, so
    // the C++ stack IS the call stack, and these two are saved and restored
    // around a call rather than pushed onto a side stack.
    Frame *current_ = nullptr;
    const CapsuleInfo *current_capsule_ = nullptr;

    // The receiver of the method being walked, or null outside one. Owning,
    // unlike current_: an object is a heap value with its own lifetime, not a
    // local on the C++ stack, and a method may outlive the expression that
    // named its receiver.
    ObjectPtr current_self_ = nullptr;

    // The spacesuit that DECLARED the method being walked — the lexical suit,
    // not current_self_->suit, which after an override is a descendant. It
    // answers exactly one question: may this code see a protected member.
    const SpacesuitInfo *current_suit_ = nullptr;

    // The declared type of each GLOBAL binding, stored ALONGSIDE the binding
    // rather than inside the Value (§7). Keyed the same way the Library is.
    // Frame slots are not in here: their types live in CapsuleInfo::slot_types,
    // one copy for the capsule instead of one per call.
    std::unordered_map<std::string, Type> declared_;

    int depth_ = 0;

    // Set by the constructor from satellite.library.system.max_depth, bounded
    // by what the C++ stack actually holds — the measurement is in eval.cpp,
    // next to DEFAULT_MAX_DEPTH.
    int max_depth_ = 0;

    // Significant digits kept by a division that does not terminate (§8.1),
    // from satellite.library.system.division_digits. Everything else the
    // language computes is exact and needs no such number.
    int division_digits_ = 0;
};

// Runtime type check, applied at declaration and at insertion only — not on
// every assignment, and not to slices, which preserve element type by
// construction (§7).
bool matches(const Type &type, const Value &value);

// The module path that owns a value's methods, or nullptr for nil, which has
// no module. Deliberately a table keyed on the variant index rather than
// derivation from the type name: satellite.container.list has two segments and
// satellite.time has one, so the path is not recoverable from the name (§7).
const char *module_of(const Value &value);

// Renders an error with the offending source line and a caret under it. Spans
// are byte offsets, never decoded positions — decode() is neither injective
// nor stable, so a caret computed from decoded text drifts (§10).
std::string format_error(const EvalError &error, const std::string &source);

} // namespace satellite
