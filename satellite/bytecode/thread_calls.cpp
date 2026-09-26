// satellite/bytecode/thread_calls.cpp -- the header says what these are for, and which of
// its rules are 003's.

#include "thread_calls.hpp"

#include "capsule_calls.hpp"
#include "capsule_scopes.hpp"
#include "program_walk.hpp"
#include "../machine/console_lock.hpp"
#include "../machine/critical_report.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/stack_share.hpp"
#include "../machine/thread_stop.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/object_lock.hpp"

#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <pthread.h>
#include <utility>

namespace satellite004 {
namespace {

token::Code code_here(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<token::Code>(row[at].to_ulong()) : 0;
}

// EVERY THREAD start() HAS MADE AND close_every_thread HAS NOT CLOSED YET.
struct Running {
    std::mutex lock;
    std::vector<ThreadHandle> threads;
};

Running &running()
{
    static Running one;
    return one;
}

// WHAT A NEW OS THREAD IS HANDED: the thread, and everything its walk reads. The registry,
// the capsule table and the libraries are read-only once a program runs, so every thread
// reads the same ones; MachineState is written by every satellite.return, so each thread
// has its OWN copy, taken from the thread that called start().
struct Launch {
    ThreadHandle thread;
    const BytecodeRegistry *program;
    const CapsuleTable *capsules;
    const FunctionTable *functions;
    MachineState state;
};

// S999 ON A THREAD, as main() says it on its own -- and guarded the same way, because
// reporting an out-of-memory allocates.
void say_the_thread_ran_out_of_memory()
{
    try {
        CriticalReport report;
        const SCode named = s_code_for(out_of_memory);
        report.code = named.code;
        report.name = named.name;
        report.description = named.means;
        report.notes.push_back("on a thread the program started");
        print_critical(report);
    } catch (const std::bad_alloc &) {
    }
}

void *run_the_thread(void *given)
{
    std::unique_ptr<Launch> launch(static_cast<Launch *>(given));
    satellite_thread &thread = *launch->thread;
    stop_of_this_thread = &thread.stop_asked;
    this_thread_runs(thread);
    Value answer;
    bool answered = false;
    signed long long int code = success;
    try {
        code = run_capsule_on_a_thread(*launch->program, *launch->capsules, *launch->functions, *thread.site,
                                       std::move(thread.arguments), thread.self, launch->state, answer, answered);
    } catch (const std::bad_alloc &) {
        // AN EXCEPTION OUT OF A THREAD IS std::terminate -- the whole process, with no
        // word said -- so it is caught here and becomes the thread's code, as main()
        // makes it satl's.
        say_the_thread_ran_out_of_memory();
        code = out_of_memory;
    }
    {
        const std::lock_guard<std::mutex> hold(thread.lock);
        thread.code = code;
        if (code == success && answered) {
            thread.answer = std::move(answer);
            thread.answered = true;
        }
        thread.ended = true;
    }
    this_thread_ended(thread);
    thread.ended_signal.notify_all();
    stop_of_this_thread = nullptr;
    return nullptr;
}

// WAIT FOR ITS WALK TO END, AND JOIN ITS OS THREAD EXACTLY ONCE (003's D5 and D9: two
// joiners at once must not both call pthread_join). Whoever claims it joins it; everyone
// else only waited for `ended`, which is set as the thread's last act.
void reap(satellite_thread &thread)
{
    std::unique_lock<std::mutex> hold(thread.lock);
    thread.ended_signal.wait(hold, [&thread] { return thread.ended && thread.launched; });
    if (thread.os_joined)
        return;
    thread.os_joined = true;
    const pthread_t which = thread.os_thread;
    hold.unlock();
    pthread_join(which, nullptr);
}

// WHAT A THREAD MAY NOT BE HANDED: a window, anywhere in what it is given (THREADS.md T3).
// A window is the main thread's -- one interpreter thread writes a piece (window_desk.hpp).
// OBJECTS AND FILES ARE SHARED (T2, the author): a thread handed one holds the same one, and
// its .lock() is what makes writes to it one at a time.
bool holds_a_window(const Value &value)
{
    if (value.is_window()) return true;
    if (const ListHandle *list = value.as_list(); list != nullptr && *list != nullptr)
        for (const Value &item : (*list)->items)
            if (holds_a_window(item)) return true;
    if (const IndexHandle *index = value.as_index(); index != nullptr && *index != nullptr)
        for (const auto &[key, item] : (*index)->entries)
            if (holds_a_window(key) || holds_a_window(item)) return true;
    return false;
}

Value start(const ThreadHandle &which, const std::string &name, ExpressionContext &context)
{
    satellite_thread &thread = *which;
    if (thread.started.exchange(true)) {
        context.refuse(thread_already_started, name + ".start() -- " + thread.name + " is already running or has run, "
                                                      "and a thread runs once; satellite.thread.new makes another");
        return Value();
    }
    // THE FLAG BEFORE THE THREAD (console_lock.hpp): from here every line is written whole.
    a_thread_was_started().store(true, std::memory_order_release);
    auto launch = std::make_unique<Launch>(
        Launch{which, context.state.program, context.state.capsules, &context.functions, context.state});
    {
        const std::lock_guard<std::mutex> hold(running().lock);
        running().threads.push_back(which);
    }
    pthread_attr_t shape;
    pthread_attr_init(&shape);
    pthread_attr_setstacksize(&shape, static_cast<std::size_t>(kStackFloorBytes));
    pthread_t id{};
    const int refused = pthread_create(&id, &shape, run_the_thread, launch.get());
    pthread_attr_destroy(&shape);
    // THE MACHINE REFUSED THE THREAD. It is marked ENDED with that code -- never set back
    // to "not started": another thread handed this one's handle may already be waiting in
    // join() for it to launch, and so may close_every_thread, and both would wait forever
    // (the review, 2026-09-23). Their join() answers S725 like this line does.
    if (refused != 0) {
        {
            const std::lock_guard<std::mutex> hold(thread.lock);
            thread.code = thread_cannot_start;
            thread.ended = true;
            thread.launched = true;
            thread.os_joined = true;     // there is no OS thread to join
        }
        thread.ended_signal.notify_all();
        context.refuse(thread_cannot_start, name + ".start() -- the machine said: " + std::strerror(refused));
        return Value();
    }
    launch.release();   // the thread owns it now
    {
        const std::lock_guard<std::mutex> hold(thread.lock);
        thread.os_thread = id;
        thread.launched = true;
    }
    thread.ended_signal.notify_all();
    return Value::of_thread(which);
}

Value join(const ThreadHandle &which, const std::string &name, const std::string &spelling, ExpressionContext &context)
{
    satellite_thread &thread = *which;
    if (!thread.started.load()) {
        context.refuse(thread_not_started, name + "." + spelling + "() -- " + thread.name + " was never started, so "
                                           "there is nothing to wait for; " + name + ".start() comes first");
        return Value();
    }
    // A THREAD WAITING FOR ITSELF waits forever (003's S1407, its smallest case). A wider
    // circle -- through joins and locks -- is caught by start_waiting_for below, S728.
    if (stop_of_this_thread == &thread.stop_asked) {
        context.refuse(satl_line_not_understood, name + "." + spelling + "() is this thread waiting for itself, and "
                                                 "that would never end");
        return Value();
    }
    // A JOIN THAT WOULD NEVER RETURN is S728 (satellite_object/object_lock.hpp): the thread
    // it waits for is -- through locks and joins -- waiting for this one. If that thread
    // closes the circle later, by waiting for a lock this one holds, ITS check says so and
    // it stops, and this join answers its S728.
    // AND IT LETS GO OF THIS LINE'S OBJECT LOCKS WHILE IT WAITS (object_lock.hpp), so the
    // thread it waits for can take them -- and then takes them back.
    let_go_while_waiting();
    if (start_waiting_for(thread) != success) {
        context.refuse(wait_never_ends, name + "." + spelling + "() would never return -- " + thread.name +
                                            " is waiting, through locks and joins, for this thread");
        return Value();
    }
    reap(thread);
    done_waiting_for_a_thread();
    const signed long long int back = take_back_after_waiting();
    if (back != success) {
        context.refuse(back, name + "." + spelling + "() returned, and taking this line's object locks back would "
                                 "never end");
        return Value();
    }
    bool again = false;
    signed long long int code = success;
    Value answer;
    bool answered = false;
    {
        const std::lock_guard<std::mutex> hold(thread.lock);
        again = thread.joined_once;
        thread.joined_once = true;
        code = thread.code;
        answered = thread.answered;
        if (answered)
            answer = thread.answer;
    }
    // AN ANSWER THAT IS AN OBJECT IS THE SAME OBJECT TO EVERY JOINER (T2): sharing is
    // allowed, and the object's .lock() is what makes two joiners' writes to it safe --
    // unlocked, two threads writing one field can crash satl (the review, 2026-09-23).
    // A SECOND JOIN IS DONE TOO (003's Q2, the author): the same answer again, and said
    // once as a notice rather than stopping anything.
    if (again) {
        const SCode named = s_code_for(thread_already_joined);
        CriticalReport notice;
        notice.code = named.code;
        notice.name = named.name;
        notice.description = named.means;
        place_here(notice, context.state);   // which join it was (M5); the log keeps it too
        print_notice(notice);
    }
    // STOPPED IS NOT FAILED: .stop() was asked for, and the answer is nothing.
    if (code == thread_stopped)
        return Value();
    // A REFUSAL ON THE THREAD WAS REPORTED THERE, with its own line and caret; the joiner
    // stops with the same code and says nothing more, as a called capsule's caller does.
    if (stops_the_program(code)) {
        context.refuse(code, name + "." + spelling + "() -- " + thread.name + " stopped on its thread");
        context.reported = true;
        return Value();
    }
    return answered ? answer : Value();
}

} // namespace

bool is_thread_word(token::Code code)
{
    return code == word::code_of(1, 23, 1);
}

Value call_thread_new(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    const std::size_t open = at;
    std::size_t k = at + 1;
    if (code_here(row, k) != token::name_token) {
        context.refuse(thread_needs_a_capsule_call, "satellite.thread.new runs a capsule of your own on a thread, so "
                                                    "what goes inside it is a call: my_capsule() or my_capsule(x)",
                       open);
        return Value();
    }
    std::vector<std::string> names;
    std::size_t past = k;
    dotted_names_at(row, past, names);
    if (code_here(row, past) != token::left_parenthesis_token) {
        context.refuse(thread_needs_a_capsule_call, "satellite.thread.new names a capsule and does not call it -- "
                                                    "write it with its brackets", open);
        return Value();
    }
    PackagedCall call;
    if (!package_capsule_call(row, k, names, past, context, call))
        return Value();
    if (code_here(row, k) != token::right_parenthesis_token) {
        context.refuse(satl_line_not_understood, "satellite.thread.new takes one capsule call and nothing after it",
                       open);
        return Value();
    }
    at = k + 1;
    for (std::size_t n = 0; n < call.arguments.size(); ++n) {
        if (holds_a_window(call.arguments[n])) {
            context.refuse(thread_cannot_share_yet,
                           call.written + "'s argument " + std::to_string(n + 1) + " is or holds a window, and a "
                               "window belongs to the main thread (THREADS.md T3)",
                           open);
            return Value();
        }
    }
    auto thread = std::make_shared<satellite_thread>();
    thread->site = call.site;
    thread->arguments = std::move(call.arguments);
    thread->self = std::move(call.self);
    thread->name = call.written;
    return Value::of_thread(std::move(thread));
}

int thread_method_arity(token::Code method)
{
    if (method == token::start_token || method == token::stop_token || method == token::join_token ||
        method == token::wait_method_token)
        return 0;
    return -1;
}

std::string thread_methods_are()
{
    return "a thread has .start(), .stop(), .join() and .wait()";
}

Value call_thread_method(token::Code method, const ThreadHandle &which, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string spelling = token::method_name_of(method);
    if (thread_method_arity(method) < 0) {
        context.refuse(types_do_not_meet, name + "." + spelling + " -- " + thread_methods_are());
        return Value();
    }
    if (!had_parentheses || !arguments.empty()) {
        context.refuse(satl_line_not_understood, name + "." + spelling + "() takes nothing, in its brackets");
        return Value();
    }
    if (which == nullptr) {
        context.refuse(satl_line_not_understood, name + " holds no thread yet -- give it one with "
                                                 "= satellite.thread.new(a_capsule())");
        return Value();
    }
    if (method == token::start_token)
        return start(which, name, context);
    if (method == token::stop_token) {
        which->stop_asked.store(true, std::memory_order_relaxed);
        return Value::of_thread(which);
    }
    return join(which, name, spelling, context);
}

signed long long int close_every_thread()
{
    signed long long int first_failure = success;
    std::vector<ThreadHandle> closing;
    {
        const std::lock_guard<std::mutex> hold(running().lock);
        closing.swap(running().threads);
    }
    // A THREAD MAY START ANOTHER WHILE THESE ARE CLOSING, so this goes round until a pass
    // finds none left.
    while (!closing.empty()) {
        for (const ThreadHandle &thread : closing)
            thread->stop_asked.store(true, std::memory_order_relaxed);
        for (const ThreadHandle &thread : closing) {
            reap(*thread);
            const std::lock_guard<std::mutex> hold(thread->lock);
            // joined_once is NOT set here: a program thread inside that thread's join() at
            // this moment would read it and say S724 for a join it made once (the review).
            if (!thread->joined_once && thread->code != thread_stopped && thread->code != program_returned &&
                stops_the_program(thread->code) && first_failure == success)
                first_failure = thread->code;
        }
        closing.clear();
        const std::lock_guard<std::mutex> hold(running().lock);
        closing.swap(running().threads);
    }
    return first_failure;
}

} // namespace satellite004
