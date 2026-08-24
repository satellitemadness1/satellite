#pragma once

#include "abstract_syntax_tree/ast.hpp"
#include "environment/env.hpp"
#include "satellite_value/value.hpp"

#include <functional>
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

// Only ever held as a pointer here, so the declaration is enough and
// <thread> stays out of every translation unit that includes this one.
class Console;

// One capsule activation.
//
// No mutex and no atomic, deliberately: a frame is reachable from exactly one
// thread. That is the entire difference from satellite.library, and it is what
// turns §6's 1585/1600 into 0/1600.
//
// It sits here rather than beside CapsuleInfo in env.hpp, which is where it was
// written, because it is the one thing in the resolver's output that mentions a
// Value — and an activation is the tree walker's idea, not the resolver's. The
// compiler shares resolve() and has no frames at all; a local there is an
// `alloca`. See the note at the top of env.hpp.
struct Frame {
    std::vector<ValuePtr> slots;
};

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
    //
    // Empty when a Console is attached, because output then goes to the
    // Console's printer thread instead of accumulating here. The REPL has no
    // Console and reads this, which is what makes `x` echo its value.
    const std::string &output() const { return output_; }

    // Sends output to `console` instead of accumulating it in output_.
    //
    // Null is the default and means the old behaviour, which the REPL still
    // needs: eval_line() runs one line and returns the text it produced, so
    // there the buffer IS the return value. A Console must outlive the
    // Evaluator attached to it.
    void set_console(Console *console) { console_ = console; }

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
    // The map's read surface, in src/evaluator/maps.cpp rather than inline in
    // methods.cpp: that file is 341 lines against the ~400 limit, and the key
    // canonicaliser needs more explanation than code.
    ValuePtr call_map_method(const ValuePtr &recv, const std::string &name,
                             const std::vector<ValuePtr> &argv, Span span);
    ValuePtr call_mutator(const Expr &recv_expr, const std::string &name,
                          const std::vector<ValuePtr> &argv, Span span);

    // One read-modify-write through a storage slot, for EVERY mutating method.
    //
    // Three storage kinds, and one place each rule is written down:
    //   FIELD  — under current_self_->write_lock, so the sequence is
    //            indivisible against a plain assignment to the same field (§14)
    //   FRAME  — no lock and no atomic: reachable from exactly one thread (§6)
    //   GLOBAL — Library::update(), which is where the lost-write guarantee is
    //
    // `transform` runs C++ only, never satellite code: argv is fully reduced in
    // src/evaluator/expr.cpp before this is reached, so §7's "nothing re-enters a held
    // lock" holds by construction and a new mutator cannot break it.
    //
    // It exists because three hand-copied lock protocols is three chances for
    // the next mutator to differ subtly, and the field arm has no
    // ThreadSanitizer coverage — library_test drives the Library from C++ and
    // never runs satellite code.
    // l[i] = x. Takes the index NODE, because the container it names is what
    // gets written back — an element is not storage of its own.
    bool assign_index(const Index &node, const Expr &value_expr, Span span);

    bool update_through_slot(
        const Slot &slot, Span span,
        const std::function<bool(const Value &current, Value &next,
                                 std::string &error)> &transform);
    ValuePtr call_module(const std::vector<std::string> &path,
                         const std::vector<ValuePtr> &argv, Span span);

    // satellite.console.display(100ms): the one call whose argument is a
    // duration, and the one that is a SETTING rather than a display. It reaches
    // the Console attached by set_console() and nothing else — with no Console
    // there is no printer thread and so nothing that could pause between two
    // lines, and the call is accepted and does nothing, which is what the REPL
    // echo path and every test that reads output() back need.
    //
    // Takes the node and not a value: a duration is not one (ast.hpp), so it is
    // never evaluated into argv the way every other argument is.
    ValuePtr set_display_pace(const DurationLit &node, Span span);

    // satellite.console.display(text, end=<expr>). Takes the two argument
    // NODES and not values, because a NamedArg is not a value (ast.hpp) and so
    // never reaches argv the way an ordinary argument does.
    ValuePtr display_with_end(const Expr &text, const Expr &ending, Span span);

    // satellite.console.input. `read_input_line` drains the Console first, so a
    // prompt written with end="" is on the terminal before the read blocks.
    // False means end of input, which is a condition and not an error.
    bool read_input_line(std::string &line);
    ValuePtr console_input(const std::string &prompt, Span span);

    // satellite.console.input(prompt, target) — the OUT-PARAMETER form. Takes
    // the target NODE, because it needs a place to write and not a value.
    ValuePtr console_input_into(const Expr &prompt, const Expr &target,
                                Span span);

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

    // The one place output leaves the evaluator, so there is exactly one
    // question to answer about where it goes rather than one per call site.
    void emit(std::string text);

    const ResolveResult &resolved_;
    std::string ns_;
    bool echo_ = false;
    std::string output_;
    Console *console_ = nullptr;
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
//
// Takes the SourceMap rather than one text because this is the call §16 named
// as the one that would otherwise lie: a runtime error inside an included
// spaceship rendered against the includer's text prints line N of the wrong
// file, confidently and with a caret.
std::string format_error(const EvalError &error, const SourceMap &sources);

} // namespace satellite
