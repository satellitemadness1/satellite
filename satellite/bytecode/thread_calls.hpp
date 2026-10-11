#pragma once
// satellite/bytecode/thread_calls.hpp -- the author's threads, in 004 (2026-09-23).
//
//     satellite.variable.thread my_thread = satellite.thread.new(capsule_name(args))
//     my_thread.start()        begins it on a thread of its own, and answers at once
//     my_thread.join()         waits, and answers what the capsule handed back
//     my_thread.wait()         the same as join() -- its second name (the author)
//     my_thread.stop()         asks it to stop at its next statement (the author)
//
// 003 BUILT THIS FIRST (its M23 and THREAD.md), and what is kept from it is written where
// it is kept. From 003, on the author's rulings of 2026-09-12 and -13:
//   - new() works out the arguments NOW, on this thread, and does not enter the capsule;
//   - start() runs once; a second is S722;
//   - join() before start() is S723; a second join() answers the same again, with S724 said
//     once as a notice -- "the thread is done, so the second join is done too";
//   - a refusal on the thread is reported where it happened, with its own line and caret,
//     and join() stops the joiner with the same code, as a called capsule's refusal does;
//   - "whenever the program reaches satellite.return(satellite), close everything": at the
//     end of the run every thread still running is asked to stop and waited for, and a
//     thread that failed and was never joined still fails the run;
//   - A FRESH OS THREAD FOR EACH start(), not a warm one from the pool: a pool of N threads
//     deadlocks the moment N+1 program threads wait on each other (003 M23 §2.4). The pool
//     keeps satl's own work -- tokenising a program on 1024 threads.
//
// WHAT IS NEW IN 004:
//   - stop() and wait(), the author's (2026-09-23);
//   - THREADS SHARE NOTHING YET (T1). A thread is handed its own copy of every value; an
//     object of a spacesuit, a file or a window would be SHARED, and sharing waits for
//     .lock() (THREADS.md T2) -- so handing one to a thread is S727, and so is a window
//     word on a thread, and a spacesuit's capsule as the thing a thread runs.
//
// STACK: a thread gets kStackFloorBytes (128 MB, machine/stack_share.hpp) -- about 40,000
// capsules deep. The main thread gets the machine's whole share, which on this machine is
// about 2 GB of address space: too much to hand every one of hundreds of threads.

#include "expression.hpp"
#include "token_codes.hpp"
#include "../satellite_variable_thread/satellite_thread.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// satellite.thread.new, 1 23 1.
bool is_thread_word(token::Code code);

// `at` is on satellite.thread.new's `(` and is left past its `)`. The checker has proved
// what is inside is a call to one of the program's own capsules.
Value call_thread_new(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context);

// start, stop, join and wait take nothing: 0. Any other method: -1.
int thread_method_arity(token::Code method);

// For a refusal: the methods a thread has.
std::string thread_methods_are();

Value call_thread_method(token::Code method, const ThreadHandle &which, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context);

// THE END OF THE RUN (structured-library.cpp): every thread still running is asked to stop
// and waited for. Answers the code of the first thread that failed and that nobody joined
// -- its report was printed when it happened -- or success.
signed long long int close_every_thread();

} // namespace satellite004
