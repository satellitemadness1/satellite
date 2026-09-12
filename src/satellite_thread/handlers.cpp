// The three rows under `satellite.thread` and `satellite.variable.thread`. See
// satellite_thread/handlers.hpp for why six numbered paths need three of them.

#include "satellite_thread/handlers.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_thread/thread_handle.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <utility>

namespace satellite::thread {

namespace {

// What the program wrote, for a sentence to quote -- satellite_file/handlers
// .cpp's `asked`, one module over, and the same one line.
std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

// THE RECEIVER, AS A THREAD. Every method row here takes one and all three
// refusals below are the same two sentences, so they are written once.
bool thread_at(eval::Machine &m, const Value *arguments, Thr *out)
{
    const Value &value = arguments[0];
    if (const Thr *held = std::get_if<Thr>(&value))
        if (*held) {
            *out = *held;
            return true;
        }

    // A DECLARED THREAD THAT WAS NEVER GIVEN ONE HOLDS NOTHING, which is
    // DESIGN §6.4 qualification 3's distinction and gets §6.4's sentence rather
    // than a wrong-type one. `satellite.variable.thread t` then `t.start()` is
    // a real thing to write by accident, and "holding nothing" is what is true.
    if (value.is_nothing()) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.thread`",
        type_name(value)));
    return false;
}

// `satellite.thread.new(capsule_name(args))` `1 23 1`.
//
// THE PACKAGING ALREADY HAPPENED AND THIS ROW DOES NOT DO IT. op_package built
// the `satellite.variable.capsule` before this handler was reached -- words
// .def's seventh list is the declaration and evaluator/compile_expressions.cpp
// is where it is honoured -- so what arrives here is a value of a type the
// language has, and `new` is an ordinary one-argument handler over it.
//
// THAT SPLIT IS WHY THE COMPILER DOES THE HOLE AND NOT THIS FILE. DESIGN §13
// says the handler "evaluates the arguments and stores (capsule number,
// argument values)", which reads as though `new` had the unevaluated call in
// hand -- it cannot, because by the time any handler runs, DESIGN §6's
// evaluation rule has already been applied to everything below it. The only
// place a call can be stopped from being performed is where it is compiled.
// MILESTONES/M23.md §2.2.
bool thread_new(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    const Cap *packaged = std::get_if<Cap>(&arguments[0]);
    if (packaged == nullptr || !*packaged) {
        // THE OTHER END OF S1401, and the compiler raises the same code for
        // the same mistake spelled as a literal. This arm is what a NAME takes:
        // `satellite.thread.new(x)` compiles as an ordinary argument, because
        // the seventh list's index says which argument is a deferred call and
        // not which argument must be one -- and if `x` ever holds a deferred
        // call it works, which is the property that makes `1 6 16` a type
        // rather than a compiler internal.
        m.refuse(errors::make<errors::Code::THREAD_NOT_A_CAPSULE_CALL>(
            m.span_of(m.here()), asked(m), type_name(arguments[0])));
        return false;
    }

    Thr handle = std::make_shared<ThreadHandle>();
    handle->body = *packaged;
    handle->program = &m.program();
    handle->ast = &m.ast();
    handle->globals = m.globals();
    handle->policy = m.policy();

    *answer = Value(std::move(handle));
    return true;
}

// `my_thread.start()` `1 6 13 1`.
bool thread_start(eval::Machine &m, const Value *arguments, uint32_t,
                  Value *answer)
{
    Thr handle;
    if (!thread_at(m, arguments, &handle))
        return false;

    if (handle->started.load(std::memory_order_relaxed)) {
        m.refuse(errors::make<errors::Code::THREAD_ALREADY_STARTED>(
            m.span_of(m.here())));
        return false;
    }

    std::string why;
    if (!launch(handle, why)) {
        m.refuse(errors::make<errors::Code::THREAD_CANNOT_START>(
            m.span_of(m.here()), why));
        return false;
    }

    // START ANSWERS NOTHING AND THAT IS THE LANGUAGE RATHER THAN AN OMISSION.
    // The capsule's answer does not EXIST yet -- that is what starting means --
    // so anything returned here would be a promise a program has to ask again,
    // which is a second type and a second word. `join()` is the one of the
    // three verbs with an answer to give, because it is the one that waits.
    *answer = Value::nothing();
    return true;
}

// `my_thread.join()` `1 6 13 2`.
bool thread_join(eval::Machine &m, const Value *arguments, uint32_t,
                 Value *answer)
{
    Thr handle;
    if (!thread_at(m, arguments, &handle))
        return false;

    if (!handle->started.load(std::memory_order_relaxed)) {
        m.refuse(errors::make<errors::Code::THREAD_NOT_STARTED>(
            m.span_of(m.here())));
        return false;
    }
    if (handle->joined) {
        m.refuse(errors::make<errors::Code::THREAD_ALREADY_JOINED>(
            m.span_of(m.here())));
        return false;
    }

    wait(handle);

    // THE THREAD'S OWN REFUSAL, RE-RAISED WHOLE AND NOT WRAPPED. errors.def's
    // S14xx block note is the argument: a division by zero inside a threaded
    // capsule is S0705 with S0705's caret pointing at the line that did it, and
    // the thread is HOW the capsule ran rather than WHAT went wrong with it.
    //
    // THE SPAN STILL POINTS INTO THE ONE SOURCE FILE, which is what makes this
    // work at all: both walks compiled the same program, so the child's
    // Diagnostic carries offsets into the same text the joining run renders
    // against. A second file would have made this a harder question and M25 is
    // where that arrives.
    //
    // THE FIRST ONE, because refuse() stops the run and a second sentence about
    // a walk that has already ended is noise. The rest stay on the handle.
    if (!handle->problems.empty()) {
        m.refuse(handle->problems.front());
        return false;
    }

    *answer = handle->answer;
    return true;
}

} // namespace

void install_handlers()
{
    using words::NodeId;
    auto &table = eval::Handlers::table();

    // `new` binds no receiver -- `satellite.thread` is a namespace and not a
    // value. The two methods do, which is DESIGN §6.4 qualification 2's tag,
    // and NEITHER MUTATES: a thread is a reference type, so `start()` changes
    // what the handle points AT rather than which handle the slot holds, and
    // there is nothing to write back. That is the file's arrangement one module
    // over and it is the first time the distinction has been load-bearing --
    // every mutating row in the language so far has been a container, where the
    // body is frozen and a change IS a new value.
    table.install(static_cast<words::PathId>(NodeId::THREAD_NEW),
                  eval::Handler{thread_new, false, 1, "M23"});
    table.install(static_cast<words::PathId>(NodeId::VARIABLE_THREAD_START_0),
                  eval::Handler{thread_start, true, 1, "M23"});
    table.install(static_cast<words::PathId>(NodeId::VARIABLE_THREAD_JOIN_0),
                  eval::Handler{thread_join, true, 1, "M23"});
}

} // namespace satellite::thread
