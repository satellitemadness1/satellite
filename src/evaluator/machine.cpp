// The evaluator's control stack, and the ceiling on it. See
// evaluator/machine.hpp for what this shape is for and what it refuses to be.
//
// THE LOOP IS SIX LINES AND EVERYTHING ELSE HERE SERVES IT. run() pops nothing
// -- it reads the top of the work stack and calls the op's function, and the
// function decides whether that item stays. That is what makes `step` a return
// address rather than a state machine: an op that needs its children pushes
// them ON TOP of itself and asks to be resumed, exactly the way a C++ frame
// would have sat underneath its callee's.
//
// AND IT IS ONE INDIRECT CALL WITH NO TAG TEST, which is PLAN §2.3's promise
// about closure compilation. There is no switch over a kind in this file.

#include "evaluator/machine.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_spacesuit/suit_object.hpp"
#include "system_facts/facts.hpp"
#include "satellite_value/render.hpp"

#include <utility>

namespace satellite::eval {

namespace {

// How many frames of a call stack a diagnostic prints before it stops.
//
// A RENDERING BOUND AND NOT A LIMIT ON THE LANGUAGE, which is a distinction
// this tree has had to make twice already -- M8.5 §4.3 about a printer running
// out of room for its own answer, and NO_LIMITS §1.2 about a crash being called
// a limit. A recursion refused at two million frames has two million FrameRefs
// and nobody wants them; the note S0790 attaches says how many were dropped, so
// the number is in the output rather than in this header alone.
constexpr size_t kFramesPrinted = 8;

} // namespace

std::string arity_text(uint32_t count)
{
    return std::to_string(count) + (count == 1 ? " argument" : " arguments");
}

Machine::Machine(const Compiled &program, const Ast &ast, const Policy &policy)
    : Machine(program, ast, policy,
              std::make_shared<Globals>(program.globals()))
{
}

// A SECOND WALK OVER THE SAME PROGRAM -- M23. See machine.hpp for what is
// shared and what is not; the one line worth repeating here is that `caches_`
// is sized again rather than shared, which closure.hpp asked for at M9: the op
// arena is immutable and shareable, and the mutable half is a side table so
// that "DESIGN §10.5's threads walk one arena and hold one cache each".
Machine::Machine(const Compiled &program, const Ast &ast, const Policy &policy,
                 std::shared_ptr<Globals> globals)
    : program_(program), ast_(ast), globals_(std::move(globals)),
      policy_(policy), ceiling_(policy.max_depth)
{
    caches_.resize(program_.caches());
    if (program_.starts_threads())
        globals_->share();
}

unsigned long long Machine::control_bytes() const
{
    // WHAT THE STACK COSTS, COUNTED AS CAPACITY AND NOT AS SIZE. A vector that
    // has doubled to a million entries is holding a million entries' worth of
    // memory whether or not it is using them, and `max_depth` is a promise
    // about memory. Counting size() would let a program sit at 90% of its
    // ceiling with twice that much actually resident.
    return static_cast<unsigned long long>(work_.capacity()) * sizeof(Work) +
           static_cast<unsigned long long>(value_.capacity()) * sizeof(Value) +
           static_cast<unsigned long long>(slots_.capacity()) * sizeof(Value) +
           static_cast<unsigned long long>(frames_.capacity()) * sizeof(Frame);
}

template <typename T>
bool Machine::room(std::vector<T> &v)
{
    // THE COMMON CASE IS ONE COMPARE, and that is the whole of what PLAN §8's
    // M9 entry means by "the check happens when the stack GROWS rather than on
    // every push, so it costs nothing in the walk". A push_back does this same
    // compare internally; the arithmetic below runs once per doubling, which
    // over a million pushes is about twenty times.
    if (v.size() < v.capacity())
        return true;

    const size_t have = v.capacity();
    const size_t want = have ? have * 2 : 64;
    const unsigned long long after =
        control_bytes() + static_cast<unsigned long long>(want - have) * sizeof(T);

    if (after > ceiling_) {
        refuse_depth();
        return false;
    }

    v.reserve(want);
    if (after > peak_)
        peak_ = after;
    return true;
}

void Machine::push(OpIndex op)
{
    if (!room(work_))
        return;
    work_.push_back({op, 0});
}

void Machine::push_value(Value value)
{
    if (!room(value_))
        return;
    value_.push_back(std::move(value));
}

void Machine::enter(uint32_t capsule, uint32_t count, NodeIndex call)
{
    const Capsule &target = program_.capsules()[capsule];

    if (!room(frames_))
        return;

    // THE ARGUMENTS ARE ALREADY ON THE VALUE STACK, in order, and they are
    // MOVED into the frame rather than copied. A number that has promoted to a
    // bignum is a shared_ptr; a string always is; moving means a recursive
    // capsule passing a large value down does not touch a refcount per level.
    const uint32_t base = static_cast<uint32_t>(slots_.size());
    for (uint32_t i = 0; i < target.slots; i++) {
        if (!room(slots_))
            return;
        slots_.emplace_back();
    }
    for (uint32_t i = 0; i < count; i++)
        slots_[base + i] = std::move(value_[value_.size() - count + i]);
    value_.resize(value_.size() - count);

    // A METHOD TAKES ITS OBJECT FOR THE WHOLE CALL -- THREAD.md T2, the
    // author's access list. Only once there is a second thread; a program with
    // none pays the one load of `shared()`.
    Sui holding;
    if (target.method && globals_->shared() && count > 0)
        if (const Sui *object = std::get_if<Sui>(&slots_[base]); object && *object) {
            if (!thread::acquire((*object)->access, wait_)) {
                slots_.resize(base);
                refuse_wait((*object)->layout ? "an object of `" +
                                                    (*object)->layout->name + "`"
                                              : std::string("an object"));
                return;
            }
            holding = *object;
        }

    frames_.push_back({base, static_cast<uint32_t>(work_.size() - 1),
                       static_cast<uint32_t>(value_.size()), target.path, call,
                       std::move(holding)});
    push(target.body);
}

void Machine::unwind(Value answer)
{
    // ONE PLACE THE FRAME IS POPPED, and both ways out come here: an explicit
    // `satellite.return` and falling off the end of a body. machine.hpp's note
    // is why there is no end-of-frame sentinel op -- the CALL op is still on the
    // work stack underneath and truncating to it removes it.
    if (frames_.empty()) {
        // A `satellite.return` outside any capsule. Nothing is left to return
        // TO, so the run ends with that value as its answer.
        work_.clear();
        push_value(std::move(answer));
        return;
    }

    const Frame frame = std::move(frames_.back());
    frames_.pop_back();

    // THE ACCESS LIST GIVES BACK WHAT THIS CALL TOOK -- the object, and the
    // globals if the statement that took them was in this frame.
    if (frame.holding)
        thread::release(frame.holding->access);
    if (globals_depth_ != kNotHeld && frames_.size() < globals_depth_) {
        globals_depth_ = kNotHeld;
        thread::release(globals_->access);
    }

    // THE OUTERMOST FRAME IS KEPT, AND ONLY THAT ONE -- M22's prompt, which
    // needs a finished program's variables to still exist afterwards. The
    // resize below destroys them, and it has to: DESIGN §7.2's slots are one
    // vector end to end and a frame that did not give its storage back would
    // leak a capsule's locals per call, which is exactly the shape a recursion
    // makes unaffordable.
    //
    // SO THE COPY IS TAKEN ONCE PER RUN AND NOT ONCE PER CALL. `frames_.empty()`
    // is true only for the call that came in through Machine::call(), so a
    // program of a million frames copies nothing until the last of them returns.
    // What it costs is one vector of the outermost capsule's slots, which is
    // the thing a caller was about to ask for anyway.
    //
    // WHY A COPY AND NOT A PROMISE NOT TO RESIZE: the Machine outlives the call
    // and `satl --call` runs a second capsule through the same one, so a view
    // into slots_ would be a view into whatever ran next.
    if (frames_.empty())
        last_frame_.assign(slots_.begin() + frame.slots, slots_.end());

    work_.resize(frame.work_floor);
    value_.resize(frame.value_floor);
    slots_.resize(frame.slots);
    value_.push_back(std::move(answer));
}

errors::Span Machine::span_of(OpIndex op) const
{
    const NodeIndex node = program_.node_of(op);
    if (node == kNoNode)
        return errors::kNowhere;
    const Token &at = ast_.token_of(node);
    return errors::Span{at.start, at.end, at.line};
}

std::string_view Machine::text_of(OpIndex op) const
{
    const NodeIndex node = program_.node_of(op);
    if (node == kNoNode)
        return {};
    // A CALL'S ANCHOR TOKEN IS ITS `(` (ast.hpp's table), AND NO SENTENCE
    // WANTS THAT WORD. Every caller here is building a refusal that quotes
    // what was ASKED -- `held`, `size`, a capsule's name -- so a call answers
    // its target's text instead: the Member or Name the postfix chain hangs
    // off. Until M12 this returned the paren, and S0713/S0714 printed
    // "`(` was asked of a variable that holds nothing" -- found by M12's
    // done-when, which demands the refusal be BY NAME, and fixed here so
    // M11's sentences heal with it. The loop is for `f()()`, where the
    // target is itself a call.
    //
    // AND M16 ADDED Index AND Slice FOR THE SAME REASON ONE FORM ALONG. A
    // subscript's anchor token is its `[`, so without this every containers
    // refusal would open "`[` was asked about position 9" -- M12's exact
    // finding, arriving at the second bracketing form the grammar has.
    NodeIndex named = node;
    while ((ast_[named].kind == NodeKind::Call ||
            ast_[named].kind == NodeKind::Index ||
            ast_[named].kind == NodeKind::Slice) &&
           ast_[named].a != kNoNode)
        named = ast_[named].a;
    return ast_.text_of(named);
}

std::vector<errors::FrameRef> Machine::call_stack() const
{
    // INNERMOST FIRST, which is the order a person reads a stack trace in and
    // the order DESIGN §9's example prints. Only the innermost few, for the
    // reason kFramesPrinted carries.
    std::vector<errors::FrameRef> out;
    const size_t show = frames_.size() < kFramesPrinted ? frames_.size() : kFramesPrinted;
    for (size_t i = 0; i < show; i++) {
        const Frame &frame = frames_[frames_.size() - 1 - i];
        out.push_back({frame.capsule, frame.call == kNoNode
                                          ? errors::kNowhere
                                          : errors::Span{ast_.token_of(frame.call).start,
                                                         ast_.token_of(frame.call).end,
                                                         ast_.token_of(frame.call).line}});
    }
    return out;
}

bool Machine::call_handler(const Handler *handler, uint32_t count, Value *answer,
                           OpListId named)
{
    // THE NAMED VALUES SIT ABOVE THE POSITIONAL ONES on the value stack, in the
    // order the names list gives -- the compiler appended them -- so the
    // handler's `arguments` still starts at argument 0 and `count` is still the
    // positional count it always was.
    const uint32_t given = named == kNoOpList ? 0 : program_.list_size(named);
    const Value *arguments = value_.data() + value_.size() - count - given;
    active_names_ = named;
    active_values_ = given ? arguments + count : nullptr;
    const bool ok = handler->fn(*this, arguments, count, answer);
    active_names_ = kNoOpList;
    active_values_ = nullptr;
    if (!ok)
        return false;
    value_.resize(value_.size() - count - given);
    return true;
}

const Value *Machine::option(std::string_view name) const
{
    if (active_names_ == kNoOpList)
        return nullptr;
    for (uint32_t i = 0; i < program_.list_size(active_names_); i++)
        if (program_.text(program_.list_at(active_names_, i)) == name)
            return active_values_ + i;
    return nullptr;
}

void Machine::refuse(errors::Diagnostic problem)
{
    // THE FIRST SENTENCE IS THE ONE, and this guard is what makes the loop's
    // stopping rule safe rather than nearly safe. An arm that has just been
    // refused returns immediately, but it may already have pushed one of two
    // children before the second push hit the ceiling -- so refuse() can be
    // reached twice for one event. Reporting both would print the same
    // recursion twice with different numbers in it.
    if (ending_ != Ending::Finished)
        return;

    // A DIAGNOSTIC THAT ALREADY HAS FRAMES KEEPS THEM -- THREAD.md D13. The
    // only producer of one is `join()` re-raising a thread's refusal, and its
    // frames are the thread's: overwriting them with the joiner's stack blamed
    // `satellite.main` for an error in the capsule the thread ran.
    if (problem.frames.empty())
        problem.frames = call_stack();
    problems_.push_back(std::move(problem));
    ending_ = Ending::Refused;
}

void Machine::refuse_depth()
{
    // THE SENTENCE NAMES RECURSION, WHICH IS THE WHOLE REASON THIS CEILING
    // EXISTS RATHER THAN BEING LEFT TO THE WATCHDOG. M6's watchdog would stop
    // this run too -- touched pages are resident memory and it counts them --
    // but it can only say the run is using N and MEMORY_MAX is M.
    // SCRATCH.md/NO_LIMITS.md §8's first question is that gap, and PLAN §8's M9
    // entry is the decision that closes it: caught one layer in, where the thing
    // that is growing has a name.
    // THE BYTES ARE PRINTED AS A PERSON WRITES THEM, which is the same pairing
    // M6's watchdog uses one layer out -- "this run is using 1.0 GiB and
    // MEMORY_MAX is 1.0 GiB". facts::human_bytes moved out of machine_limits at
    // this milestone so that this sentence could have it without the evaluator
    // including the module it obeys; facts.hpp carries the argument.
    errors::Diagnostic problem = errors::make<errors::Code::EVAL_TOO_DEEP>(
        work_.empty() ? errors::kNowhere : span_of(work_.back().op),
        static_cast<unsigned long long>(frames_.size()),
        facts::human_bytes(control_bytes()), facts::human_bytes(ceiling_));

    if (frames_.size() > kFramesPrinted)
        problem.notes.push_back(errors::note<errors::Code::NOTE_EVAL_DEEPEST_CALL>(
            problem.at, static_cast<unsigned long long>(frames_.size() - kFramesPrinted)));

    refuse(std::move(problem));

    // AFTER refuse(), WHICH SET IT TO Refused. The order is the one thing to
    // get right here: refuse() is the one place a run stops, and this is the
    // one case where stopping is not the program's fault.
    ending_ = Ending::Stopped;
}

bool Machine::touch_globals()
{
    if (globals_depth_ != kNotHeld)
        return true;
    if (!thread::acquire(globals_->access, wait_)) {
        refuse_wait("`satellite.library`");
        return false;
    }
    globals_depth_ = static_cast<uint32_t>(frames_.size());
    return true;
}

void Machine::refuse_wait(const std::string &what)
{
    refuse(errors::make<errors::Code::THREAD_WAIT_NEVER_ENDS>(
        here_or_nowhere(), what));
}

void Machine::release_accesses()
{
    for (Frame &frame : frames_)
        if (frame.holding) {
            thread::release(frame.holding->access);
            frame.holding.reset();
        }
    if (globals_depth_ != kNotHeld) {
        globals_depth_ = kNotHeld;
        thread::release(globals_->access);
    }
}

bool Machine::interrupted(OpIndex at)
{
    // THE END OF A STATEMENT IS THE END OF ITS HOLD ON THE GLOBALS -- THREAD.md
    // T2 -- when the statement that took them is in this frame. A boundary in a
    // capsule the statement called is deeper, and lets go of nothing.
    if (globals_depth_ != kNotHeld && frames_.size() <= globals_depth_) {
        globals_depth_ = kNotHeld;
        thread::release(globals_->access);
    }

    if (policy_.interrupted == nullptr || !policy_.interrupted())
        return false;

    // THE CARET GOES UNDER WHAT DID NOT RUN. Everything above the reported
    // line has happened; nothing at it or after it has -- which is the one
    // fact a person interrupting a long run wants, and it is the same fact
    // whether the boundary was a block's next statement or a loop's next
    // iteration. The console's queue is drained by the caller's shutdown
    // before this sentence prints, so the output above the caret really is
    // everything the program said.
    errors::Diagnostic problem = errors::make<errors::Code::EVAL_INTERRUPTED>(
        at == kNoOp ? errors::kNowhere : span_of(at));
    refuse(std::move(problem));

    // AFTER refuse(), WHICH SET IT TO Refused -- refuse_depth() below is the
    // same two steps in the same order and carries the argument: refuse() is
    // the one place a run stops, and this is the second of the two cases where
    // stopping is not the program's fault.
    ending_ = Ending::Interrupted;
    return true;
}

void Machine::run()
{
    // THE STOPPING TEST IS IN THE LOOP AND NOT AT EVERY PUSH, which is a
    // correctness fix that turned out to be the cheaper shape as well.
    //
    // refuse() used to clear the work stack, on the reasoning that an empty
    // stack ends the loop. It does -- but an arm is not finished when it
    // refuses: `a + b` pushes two children, and if the FIRST push reaches the
    // ceiling the second one still runs, onto a stack that was just emptied.
    // Guarding every push against `ending_` would have fixed it and put a load
    // and a compare on the hottest path in the interpreter. This puts the same
    // compare in the loop, which already has one.
    while (!work_.empty() && ending_ == Ending::Finished) {
        const Work top = work_.back();
        const Op &op = program_[top.op];
        op.fn(*this, op, top.step);
    }

    // A RUN THAT ENDS -- by finishing, refusing or being stopped -- leaves
    // nothing on its access list, or every thread waiting on it would wait for
    // ever. A refused walk never unwinds its frames, so they are walked here.
    release_accesses();
}

void Machine::run_top_level()
{
    if (program_.top() == kNoOp)
        return;
    push(program_.top());
    run();
}

Value Machine::call(uint32_t capsule, const std::vector<Value> &arguments)
{
    const Capsule &target = program_.capsules()[capsule];
    if (arguments.size() != target.parameters) {
        refuse(errors::make<errors::Code::EVAL_ARGUMENT_COUNT>(
            target.node == kNoNode ? errors::kNowhere
                                   : errors::Span{ast_.token_of(target.node).start,
                                                  ast_.token_of(target.node).end,
                                                  ast_.token_of(target.node).line},
            std::string(ast_.text_of(target.node)),
            arity_text(target.parameters),
            std::to_string(arguments.size())));
        return Value::nothing();
    }

    // A CALL FROM OUTSIDE LOOKS LIKE A CALL FROM INSIDE, which is what keeps
    // there being one entry(). The op arena has an entry op for every capsule
    // precisely so that this path and op_call's path share every line of
    // enter() and unwind() -- a second way in is a second place a frame can be
    // got wrong, and DESIGN §7.1 is what that costs.
    for (const Value &argument : arguments)
        push_value(argument);

    push(program_.capsules()[capsule].entry);
    run();

    if (!ok() || value_.empty())
        return Value::nothing();
    return pop_value();
}

} // namespace satellite::eval
