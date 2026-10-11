#pragma once
// satellite/satellite_variable_program/program_watch.hpp -- THE WATCHER: one OS thread a run of a
// satellite.variable.program, which shows what the program writes, writes what pass() typed in, and
// says how it ended (2026-10-01). Split from bytecode/program_calls.cpp when pass() came (STEP 4);
// that file keeps the methods and the end of the run.
//
// ONE OS THREAD A RUN, as a satellite thread has one (bytecode/thread_calls.hpp says why a pool would
// not do), and it is the only thing that reaps its program -- so the pid cannot be handed to another
// process while the run is going.

#include "satellite_program.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <sys/types.h>

namespace satellite004 {

// WHAT A WATCHER IS HANDED, and owns once it has it: it closes every descriptor here.
struct Watch {
    ProgramHandle program;
    std::uint64_t run;                             // which run of it this is
    int out;                                       // the read end of its output, or -1 for start("hide")
    int in;                                        // the write end of its input: what pass() typed goes here
    int wake;                                      // the run's eventfd, also the program's input_wake
    int pidfd;                                     // readable once it has ended; -1 on a kernel with none
    pid_t pid;
    std::shared_ptr<std::atomic<bool>> finished;   // set as its last act, so it can be joined at once
};

// THE RUN IS CLOSING (program_calls.cpp's close_every_program): readable from the moment that begins
// until every watcher has been joined, so a watcher still showing what a program left running stops.
int closing_fd();

// The thread's body: pthread_create(..., watch_the_program, new Watch{...}).
void *watch_the_program(void *given);

} // namespace satellite004
