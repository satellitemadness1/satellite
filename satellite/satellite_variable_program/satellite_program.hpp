#pragma once
// satellite/satellite_variable_program/satellite_program.hpp -- `satellite.variable.program`,
// `1 6 23`, arm 19 of satelliteObject (2026-10-01).
//
// The author's syntax, 2026-10-01:
//
//     satellite.variable.program my_program = {"/dir/program", "arg1", "arg2"}
//     satellite.variable.program run_program = "/dir/some_program"    -- the program alone
//     my_program.start()       "have .start() return to satl while the output from the program
//                              is displayed ... forcing the user to use both .start() and .join()"
//     my_program.ok()          whether it could start
//     my_program.error()       why it could not
//     my_program.join()        "make .join() .end() and .code() wait for the program to finish,
//     my_program.code()        all 3 do the same thing, and return the error code" -- and then
//     my_program.return()      ".code() .return() ... will be for the return"; .end() kills it
//                              (bytecode/program_calls.hpp keeps his words for each)
//
// WHAT THIS HEADER IS: the program as a VALUE -- what it runs, and what happened when it last
// ran. What STARTS it is program_spawn.cpp; what watches it and answers its methods is
// bytecode/program_calls.cpp. So this header includes neither.
//
// WHAT IT RUNS IS FIXED WHEN IT IS DECLARED. `words` is the program and then its arguments,
// each ONE argument however many spaces it holds -- "a program and a list of arguments, with
// no shell" (RUNNING_PROGRAMS.md); a string on its own is the program's name alone and is
// never split.
//
// EVERYTHING UNDER `lock` IS WRITTEN BY TWO THREADS: the one that calls start() and join(),
// and the program's watcher, which hands its output to the screen and writes how it ended.

#include "satellite_program_handle.hpp"
#include "../machine/critical_report.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <pthread.h>
#include <string>
#include <sys/types.h>
#include <vector>

namespace satellite004 {

class satellite_program {
public:
    // WHAT IT RUNS, fixed at the declaration and never changed after.
    std::vector<std::string> words;           // the program, then each argument

    // THE LAST RUN, all under `lock`. RUNS ARE COUNTED, so a join() waits for the run that was
    // going when it was reached -- another thread may start the next one the moment it ends.
    mutable std::mutex lock;
    std::condition_variable ended_signal;
    bool started = false;                     // start() has run at least once
    std::uint64_t runs_started = 0;           // claimed by start() before anything is spawned
    std::uint64_t runs_ended = 0;             // set by the run's watcher, or by start() when it could not start
    bool joined = false;                      // a join() was reached for the last run ("both .start() and .join()")
    bool could_start = false;                 // the last start() got the program going -- ok()
    std::string why;                          // why it could not start, or how it ended -- error()
    long long code = 0;                       // its exit code: 0-255, 128 + N for signal N, 126 or 127
                                              // when it could not start (a shell's numbers for both)
    pid_t pid = 0;                            // while it runs
    int pidfd = -1;                           // while it runs: a signal sent through it can never reach
                                              // another process that was handed the same pid later
    std::string name;                         // the variable, as start() was written on it
    CriticalReport started_at;                // where that start() was, for the report at the end

    // ON THE END-OF-RUN LIST (program_calls.cpp) -- kept under that list's lock, not this one.
    bool listed = false;

    bool running() const { return runs_started > runs_ended; }   // under `lock`

    // For satellite.console.display(p): what it runs and how far it has got --
    // (program /usr/bin/make -j16, running).
    std::string shown() const
    {
        std::lock_guard<std::mutex> hold(lock);
        std::string what;
        for (const std::string &word : words) {
            if (!what.empty()) what += ' ';
            what += word;
        }
        const std::string where = !started     ? "not started"
                                  : running()  ? "running"
                                  : !could_start ? "could not start"
                                                 : "ended with " + std::to_string(code);
        return "(program " + what + ", " + where + ")";
    }
};

} // namespace satellite004
