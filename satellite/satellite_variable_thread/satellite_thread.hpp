#pragma once
// satellite/satellite_variable_thread/satellite_thread.hpp -- `satellite.variable.thread`,
// `1 6 13`, arm 18 of satelliteObject (2026-09-23).
//
// The author's syntax, settled 2026-08-27 (003 DESIGN) and asked for again for 004 on
// 2026-09-23:
//
//     satellite.variable.thread my_thread = satellite.thread.new(capsule_name(args))
//     my_thread.start()
//     my_thread.join()        -- or my_thread.wait(), its second name (the author)
//     my_thread.stop()        -- asks it to stop at its next statement (the author)
//
// WHAT THIS HEADER IS: the thread as a VALUE -- what it will run, and what happened when
// it ran. What RUNS it (the walker, the OS thread, the stop check) is bytecode/
// thread_calls.cpp, so this header never includes the walker.
//
// THE CALL IS WORKED OUT AT `new` AND RUN AT `start()` (003's M23 §2.2). `new` finds the
// capsule and works out its arguments on the calling thread, then keeps them: a later
// change to a variable that was an argument is not seen by the thread. The capsule itself
// is not entered until start().
//
// EVERYTHING BELOW `started` IS WRITTEN BY TWO THREADS, and each field says how it is
// kept: the atomics are asked without a lock, the rest only under `lock`.

#include "satellite_thread_handle.hpp"
#include "../satellite_object/satellite_object.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>

namespace satellite004 {

struct CapsuleSite;

class satellite_thread {
public:
    // WHAT IT RUNS, fixed at `new` and never changed after.
    const CapsuleSite *site = nullptr;        // the capsule, found by the checker's rules
    std::vector<satelliteObject> arguments;   // worked out at `new`, moved in at start()
    std::string name;                         // as the program wrote it: "count_alone"

    // THE RUN.
    std::atomic<bool> started{false};         // exchange(true) at start(): exactly one start wins (003's D4)
    std::atomic<bool> stop_asked{false};      // .stop() sets it; the walker reads it between statements

    mutable std::mutex lock;
    std::condition_variable ended_signal;
    bool ended = false;                       // the capsule's walk is over       (under lock)
    bool launched = false;                    // os_thread holds the thread's id  (under lock) -- POSIX does
                                              // not promise pthread_create writes it before the thread runs
    bool os_joined = false;                   // pthread_join has been claimed once (under lock)
    bool joined_once = false;                 // a program's join() has been answered once (under lock)
    signed long long int code = 0;            // the machine code the walk ended on (under lock)
    satelliteObject answer;                   // what it handed back, when it did (under lock)
    bool answered = false;                    // (under lock)
    pthread_t os_thread{};                    // valid once started, until os_joined

    // For satellite.console.display(t): which capsule, and where it has got to --
    // (thread count_alone, running), in the shape a window displays in.
    std::string shown() const
    {
        std::lock_guard<std::mutex> hold(lock);
        const char *where = !started.load() ? "not started"
                            : !ended        ? "running"
                            : code == thread_stopped    ? "stopped"
                            : stops_the_program(code)   ? "failed"
                                                        : "finished";
        return "(thread " + name + ", " + where + ")";
    }
};

} // namespace satellite004
