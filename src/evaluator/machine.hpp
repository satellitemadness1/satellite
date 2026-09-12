#pragma once

// The evaluator's control stack -- PLAN M9. DESIGN §7.5 is the rule and PLAN
// §2.5 is the decision that made this milestone build it rather than bound it.
//
// THE POINT OF THIS FILE IS THAT A SATELLITE PROGRAM'S RECURSION DEPTH IS
// BOUNDED BY MEMORY. Not by `ulimit -s`, not by a constant in a header, not by
// a ceiling derived from either. PLAN §2.5: "a walker that keeps its own stack
// ON THE HEAP has no depth at all -- the bound becomes memory, the same way a
// list's bound is memory. That is what 'no limits' means concretely, and it is
// the only thing that means it; every other answer is a bigger number."
//
// SO THERE IS NO eval(node) THAT CALLS eval(child). Four vectors carry what a
// recursive evaluator would have put in C++ frames:
//
//   work_    what is left to do, as {op, step} pairs. `step` is the return
//            address: an op that needs its children's answers pushes them and
//            asks to be resumed one step further on.
//   value_   the answers. An EXPRESSION op leaves exactly one value here; a
//            STATEMENT op leaves none. That contract is the whole type system
//            of this machine and it is checked by tests/eval_test.
//   slots_   every live frame's storage, end to end. DESIGN §7.2's
//            `std::vector<Value> slots`, one region per activation.
//   frames_  where each activation's regions start, and where a
//            `satellite.return` unwinds to.
//
// A CALL OWNS ITS FRAME FROM BOTH ENDS AND THERE IS NO SENTINEL OP. The
// obvious design pushes an "end of frame" marker under the body so that falling
// off the end of a capsule has something to pop the frame; this does not,
// because the CALL op is already sitting on the work stack underneath and can
// be resumed. Its step 2 is the implicit `satellite.return()`. A real return
// truncates the work stack to below that op, so the two paths leave identical
// state and there is one place the frame is popped -- unwind().
//
// WHAT THE CEILING IS, AND IT IS NOT A DEPTH. `satellite.library.system
// .max_depth` `1 14 2 2` is a MEMORY ceiling on this stack, in bytes, and unset
// means the machine -- the author's decision of 2026-09-01, and PLAN §8's M9
// entry carries the argument. Three things follow and this file is where all
// three land:
//
//   1. The check happens when the stack GROWS, not on every push. A push is a
//      size-against-capacity compare, which is what a vector does anyway; the
//      byte arithmetic runs once per doubling.
//   2. The refusal is a sentence about RECURSION. That is what M6's watchdog
//      cannot give -- it can only say the run is using N and MEMORY_MAX is M --
//      and SCRATCH.md/NO_LIMITS.md §8's first question is exactly that gap.
//   3. Unset means the machine, which is not a new rule: PLAN §4.5.4 settled it
//      for MEMORY_MAX in the same words, "a fraction is a number satl would
//      have invented about a program it has never seen".
//
// AND IT IS NOT A LIMIT THE LANGUAGE HAS. DESIGN §7.5 is untouched by it: a
// ceiling the USER sets on their own program is not a limit the language has,
// which is the distinction M8 drew for `division_digits` in the same words.
// With the dial unset a runaway recursion runs until the machine is full and
// then says so ABOUT RECURSION, which is a working answer and a better sentence
// than the one the watchdog was giving.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/closure.hpp"
#include "evaluator/globals.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace satellite::eval {

struct Handler;

// THE TWO NUMBERS THE EVALUATOR OBEYS AND DOES NOT CHOOSE.
//
// HANDED IN RATHER THAN READ, which is the seam LAYOUT.md draws between
// machine_limits/ -- the policy -- and everything that lives under one. `satl`
// fills this from limits::max_depth_bytes() and limits::division_digits();
// tests/eval_test fills it itself, and that is what lets the depth fixtures
// link no machine_limits at all. M8.5 §4.1 is why that matters: a 20,000-deep
// resolve fixture spent a day passing against a raised stack it had never been
// given, because the test binary did not link the module that raises one.
//
// M15's `float_digits` was the third and it joined this struct rather than a
// second constructor argument -- which is the shape M8 wished for when
// `division_digits` arrived alone.
//
// AND SINCE M15 THIS STRUCT IS THE NAMESPACE'S RUN-TIME HALF. PLAN §4.5.3:
// the file seeds these at startup, and what a running program reads OR
// RETUNES through the `1 14 2` dials is this struct on this machine --
// satellite_system/handlers.cpp's rows answer from it and store into it, so
// a retune is one store and the next division observes it. Nothing here is
// process-wide, which is what keeps M22's many-programs future honest.
struct Policy {
    // A CEILING IN BYTES ON THE CONTROL STACK, not a count of frames.
    // `satellite.library.system.max_depth` `1 14 2 2`, the author's reading of
    // 2026-09-01. Unset means the machine.
    unsigned long long max_depth = 0;

    // DESIGN §8.1's significant digits for a division that does not terminate.
    // `satellite.library.system.division_digits`, M8's dial.
    unsigned division_digits = 34;

    // DESIGN §8.6: the DEFAULT length of a float's right half, for a result
    // that would otherwise have less -- not a global bound, because precision
    // travels with the value (`R`'s digit count IS the precision).
    // `satellite.library.system.float_digits` `1 14 2 4`, M15's own dial.
    // 34 BESIDE division_digits' 34 DELIBERATELY: both are "the default
    // width of an inexact result", and two different arbitrary constants
    // would be two facts where the language has one. Overrulable here and in
    // machine_limits/limits.hpp together, and nowhere else.
    unsigned float_digits = 34;

    // WHETHER CTRL-C HAS ARRIVED -- system_facts/interrupt.hpp's flag, M11,
    // HANDED IN AS A FUNCTION AND NOT READ, for the same reason the ceiling
    // is: the seam. `satl`'s arms pass interrupt_requested; tests/eval_test
    // passes a function of its own and can interrupt a run without a signal
    // ever being raised, which is what makes the statement-boundary contract
    // testable at all. Null means no one is listening, which is what a test
    // that is not about interruption wants.
    bool (*interrupted)() = nullptr;
};

// "1 argument" or "3 arguments" -- the {2} hole in S0722, from the three sites
// that raise it.
//
// A SENTENCE FRAGMENT GETS A FUNCTION BECAUSE THREE PLACES BUILT IT BY HAND AND
// ALL THREE SAID "1 arguments". That was invisible while nothing in the
// language had a handler to get wrong: M9's producers are a capsule call and a
// test's own row. M10 installs `satellite.console.display` with an arity of
// ONE, so the first person to write `display("a", "b")` is the first person to
// read the sentence, and errors.def's whole argument is that a message is a row
// somebody maintains rather than a literal at a call site.
std::string arity_text(uint32_t count);

// One thing left to do. Eight bytes, which is what makes a million-deep
// recursion eight megabytes of work stack rather than a segfault.
struct Work {
    OpIndex op = kNoOp;
    uint32_t step = 0;
};

// One activation. DESIGN §7.2's frame, plus the three heights a
// `satellite.return` rewinds to.
struct Frame {
    uint32_t slots = 0;       // where this frame's storage starts in slots_
    uint32_t work_floor = 0;  // where its call op sits in work_
    uint32_t value_floor = 0; // value_'s height when its body began
    words::PathId capsule = words::kNoPath;
    NodeIndex call = kNoNode; // the call SITE, which is what a FrameRef prints
};

// What a run ended as.
//
// REFUSED AND STOPPED ARE TWO DIFFERENT THINGS AND THEY GET TWO EXIT STATUSES.
// A program that divided by zero or compared a string to a bool is WRONG, and
// programs/opening.hpp calls that EXIT_MALFORMED -- "a file satl was given is
// not what it has to be". A program that filled the control stack may be
// perfectly correct and simply asked for more than it was allowed, which is
// EXIT_LIMIT: "satl stopped itself: a machine limit was reached". M6 built that
// status for the watchdog and this is its second producer, which is the shape
// SCRATCH.md/NO_LIMITS.md §7 asks for -- the same event caught one layer in,
// with a better sentence and the same number for a script to read.
//
// AND AN INTERRUPT IS NEITHER, WHICH IS WHY THERE ARE FOUR AND NOT THREE.
// PLAN §8's M10 entry named the trap and left the arm to M11: without a
// fourth, a person pressing Ctrl-C is reported as a machine limit and a script
// testing for 4 reads it as a memory ceiling. An interrupted program may be
// perfectly right AND under every ceiling -- somebody outside it changed
// their mind, which no other Ending can say. programs/run_command.cpp answers
// it with 130, which is 128 + SIGINT and what a shell reports for a program
// killed this way; system_facts/interrupt.hpp is where that number lives.
enum class Ending : uint8_t {
    Finished,    // ran to the end
    Refused,     // the program was wrong -- `problems` is not empty
    Stopped,     // a ceiling was reached; the program may be right
    Interrupted, // Ctrl-C -- the program stopped at a statement boundary
};

class Machine {
public:
    // THE CEILING IS HANDED IN AND NOT READ, which is the seam LAYOUT.md
    // draws between machine_limits/ (the policy) and everything that obeys one.
    // `satl`'s arms pass limits::max_depth_bytes(); tests/eval_test passes its
    // own, and that is what lets the depth fixtures link no machine_limits at
    // all -- M8.5 §4.1 found the alternative the hard way, where a 20,000-deep
    // fixture was passing against a raised stack it had never been given.
    Machine(const Compiled &program, const Ast &ast, const Policy &policy);

    // A SECOND WALK OVER THE SAME PROGRAM -- M23's thread. Everything that is
    // read-only is shared by reference (the op arena, the ast), `satellite
    // .library` is shared through the handle, and EVERYTHING ELSE IS THIS
    // MACHINE'S OWN: four stacks, the inline caches, the problems, the ending,
    // the Policy and the search threshold.
    //
    // THE SPLIT IS NOT A DECISION THIS MILESTONE TOOK, it is closure.hpp's,
    // taken at M9 and written down there: the Cache is "MUTABLE AND IN A SIDE
    // TABLE, so the op arena stays immutable and shareable across threads
    // (DESIGN §10.5) while the cache is per-run". M23 is the first caller that
    // makes that sentence do any work, and it needed no change to be true.
    //
    // THE POLICY IS COPIED AND NOT SHARED, AND THAT IS A DECISION. A thread
    // inherits its parent's dials as they stood when `start()` ran, and a
    // `satellite.library.system.float_digits = 5` on either side afterwards is
    // that side's own. Sharing them would make a dial a fourth piece of
    // cross-thread state with none of §7.2's argument behind it -- a global is
    // shared because a program SAID `satellite.library`, and nobody says that
    // about a dial. MILESTONES/M23.md §2.7.
    Machine(const Compiled &program, const Ast &ast, const Policy &policy,
            std::shared_ptr<Globals> globals);

    // Run one capsule to completion and answer what it returned. Everything a
    // caller can ask for is here, because there is no console until M10.
    Value call(uint32_t capsule, const std::vector<Value> &arguments);

    // Every global's initialiser, in order. Must run before any capsule that
    // reads one -- DESIGN §7.2 reserves `satellite.library` for shared state.
    void run_top_level();

    // THE OUTERMOST CAPSULE'S SLOTS, AFTER IT HAS RETURNED -- M22's prompt.
    // Empty until a call has finished, and replaced by the next one. Read it
    // beside resolve's Frame for that capsule: slot i here is `names[i]` there,
    // which is what turns a vector of values back into named variables.
    //
    // IT IS THE ONE THING A FINISHED RUN LEAVES BEHIND, and that is deliberate
    // rather than convenient: everything else about a run is gone by design, so
    // a caller that wants a program's variables has to say so by asking here
    // rather than by holding on to something it was lent.
    const std::vector<Value> &last_frame() const { return last_frame_; }

    Ending ending() const { return ending_; }
    const std::vector<errors::Diagnostic> &problems() const { return problems_; }
    bool ok() const { return ending_ == Ending::Finished; }
    bool at_the_ceiling() const { return ending_ == Ending::Stopped; }

    // --- what an op function may do ----------------------------------------
    //
    // PUBLIC BECAUSE THE OP FUNCTIONS ARE FREE FUNCTIONS AND NOT METHODS, which
    // is what an OpFn being a plain function pointer requires. They are all in
    // evaluator/operations.cpp and this is the interface they are written
    // against; nothing outside that file and machine.cpp calls any of them.

    // WHICH OP IS RUNNING. An op function is handed its Op by reference and
    // not its index, because the index is four bytes it does not need on the
    // hot path; a diagnostic needs it, and this is where it comes from.
    OpIndex here() const { return work_.back().op; }

    // Resume this op one step further on, once its children have run.
    void again(uint32_t step) { work_.back().step = step; }

    // This op is finished. Its own work item goes.
    void done() { work_.pop_back(); }

    void push(OpIndex op);
    void push_value(Value value);

    Value pop_value()
    {
        Value out = std::move(value_.back());
        value_.pop_back();
        return out;
    }

    // The value `back` from the top, without taking it off. 0 is the top.
    const Value &value_from_top(size_t back) const
    {
        return value_[value_.size() - 1 - back];
    }

    // TWO VALUES BECOME ONE, IN PLACE. Every binary operator and every
    // comparison ends this way, and the obvious spelling -- pop, pop, push --
    // is three vector operations where this is one assignment and a pop. The
    // push is the expensive one of the three: it is the only one that has to
    // ask room() whether the ceiling has been reached, and a Value is a
    // 40-byte variant whose destructor is not trivial.
    //
    // MEASURED, NOT ASSUMED. MILESTONES/M9.md §6 has the before and after; the
    // reason it is worth a named function rather than three lines in each arm
    // is that the arms must not be able to disagree about which slot survives.
    void fold(Value answer)
    {
        value_.pop_back();
        value_.back() = std::move(answer);
    }

    // One value becomes another, in place -- fold's unary sibling.
    void set_top(Value answer) { value_.back() = std::move(answer); }

    const Value &local(uint32_t slot) const { return slots_[frames_.back().slots + slot]; }
    void set_local(uint32_t slot, Value value)
    {
        slots_[frames_.back().slots + slot] = std::move(value);
    }

    // A COPY AND NOT A REFERENCE SINCE M23 -- evaluator/globals.hpp carries
    // why, and the short form is that a reference into storage another walk may
    // be writing is a lock that protects the wrong thing. Every caller was
    // already copying.
    Value global(uint32_t index) const { return globals_->read(index); }
    void set_global(uint32_t index, Value value)
    {
        globals_->write(index, std::move(value));
    }

    // THE GLOBALS, TO HAND TO A THREAD. `satellite.variable.thread.start()` is
    // the one caller: it takes this, calls share() on it, and gives it to the
    // child's Machine, so both walks read and write the same `satellite
    // .library`. DESIGN §7.2 is the argument and globals.hpp is the mechanism.
    const std::shared_ptr<Globals> &globals() const { return globals_; }

    // THE TREE THE OPS CAME OUT OF. Read by anything that has to build a second
    // Machine over the same program -- M23's thread, which needs it for spans
    // in a diagnostic raised inside a threaded capsule.
    const Ast &ast() const { return ast_; }

    // Enter a capsule. The arguments are the top `count` values on the value
    // stack, in order, and they become slots [0, count).
    void enter(uint32_t capsule, uint32_t count, NodeIndex call);

    // Leave the innermost capsule with this answer. Both `satellite.return` and
    // falling off the end of a body come here.
    void unwind(Value answer);

    // Run a module handler over the top `count` values, and take them off.
    //
    // THE HANDLER MUST NOT TOUCH THE VALUE STACK, which is why it is handed a
    // pointer and a count rather than the machine's vector: `arguments` points
    // INTO value_, so a handler that pushed would reallocate under its own
    // feet. Everything a handler needs to say goes through refuse().
    bool call_handler(const Handler *handler, uint32_t count, Value *answer);

    // Stop, with a sentence. Nothing runs after this.
    void refuse(errors::Diagnostic problem);

    // Ctrl-C, checked at a statement boundary -- M11's half of DESIGN §10.2.
    // True means the run just ended: the caller returns without pushing
    // anything, and `at` is the op whose line S0730's caret reports, which is
    // the statement that was ABOUT to run. Three arms call this -- op_block
    // between statements, op_while and op_for once per iteration -- because
    // those are the boundaries v1's handler comment promises: "the first
    // SIGINT sets the flag and lets the walk stop itself at the next
    // statement". The common case is one indirect call and one relaxed load.
    bool interrupted(OpIndex at);

    // The span an op's node covers, for a diagnostic raised inside it.
    errors::Span span_of(OpIndex op) const;

    // The source text under an op's node -- the selector a method sentence
    // quotes. op_dispatch carries its callee's spelling as a compiled text;
    // the method ops spend that operand on the write-back slot instead, and
    // this is where their sentences get a name from. A call op answers its
    // TARGET's text rather than its own anchor token, which is the `(` --
    // machine.cpp says when that was found and why every caller wants it so.
    // Empty for an op with no node, which no method op is.
    std::string_view text_of(OpIndex op) const;

    // The call stack, innermost first, as DESIGN §9's fourth field.
    std::vector<errors::FrameRef> call_stack() const;

    const Compiled &program() const { return program_; }
    Cache &cache(uint32_t index) { return caches_[index]; }

    // What the control stack is holding, in bytes. `satl --compile` prints the
    // high-water mark and MILESTONES/M9.md measures it.
    unsigned long long control_bytes() const;
    unsigned long long peak_bytes() const { return peak_; }
    unsigned long long ceiling() const { return ceiling_; }
    const Policy &policy() const { return policy_; }

    // THE RETUNE'S WRITE HALF -- M15. satellite_system's assigners store
    // through these three and every read reads policy(), so a retuned dial is
    // observed by the very next operation that consults it. Named methods
    // rather than a mutable policy(), because max_depth is the one dial the
    // machine caches (ceiling_, the hot compare in room()) and a bare field
    // write would quietly leave the cache stale.
    void retune_division_digits(unsigned digits)
    {
        policy_.division_digits = digits;
    }
    void retune_float_digits(unsigned digits)
    {
        policy_.float_digits = digits;
    }
    void retune_max_depth(unsigned long long bytes)
    {
        policy_.max_depth = bytes;
        ceiling_ = bytes;
    }

    // THE SEARCH DIAL -- M16. `satellite.system.threshold()` `1 22 5` reads
    // it, `(n)` `1 22 6` sets it, and every subscript search and `.search()`
    // reads it here. It starts at 1 -- the exact match every program written
    // before the power existed already assumed -- and it is the MACHINE's
    // rather than v1's thread_local, for the reason search.hpp records: until
    // M23 there are no threads to separate, and nothing on this machine is
    // process-wide. Not in Policy, because the config file does not seed it:
    // it is a knob a program moves mid-run, spelled as a call and not as a
    // retunable `1 14 2` dial.
    int search_threshold() const { return search_threshold_; }
    void set_search_threshold(int level) { search_threshold_ = level; }

private:
    // ROOM TO GROW ONE MORE ELEMENT, and this is the whole of the ceiling's
    // machinery. A vector with room left answers true after one compare; a
    // vector at capacity is about to double, so this is where the bytes are
    // counted and where a program that has asked for too much is refused.
    template <typename T>
    bool room(std::vector<T> &v);

    void refuse_depth();

    const Compiled &program_;
    const Ast &ast_;

    std::vector<Work> work_;
    std::vector<Value> value_;
    std::vector<Value> slots_;
    std::vector<Frame> frames_;
    std::shared_ptr<Globals> globals_;

    // The outermost frame's storage, copied out of slots_ before it is given
    // back. See last_frame() above.
    std::vector<Value> last_frame_;
    std::vector<Cache> caches_;

    std::vector<errors::Diagnostic> problems_;
    Ending ending_ = Ending::Finished;

    Policy policy_;
    unsigned long long ceiling_ = 0;
    unsigned long long peak_ = 0;
    int search_threshold_ = 1; // SEARCH_EXACT; the accessor above says why

    void run();
};

} // namespace satellite::eval
