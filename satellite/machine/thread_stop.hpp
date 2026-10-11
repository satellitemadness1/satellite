#pragma once
// satellite/machine/thread_stop.hpp -- WHERE A RUNNING THREAD LOOKS TO SEE IF IT WAS ASKED
// TO STOP (threads, 2026-09-23).
//
// `my_thread.stop()` never ends a thread in the middle of a statement: it sets the
// thread's flag, and the walker reads it at the top of every statement
// (program_walk.cpp's run_statements) and ends the walk with thread_stopped (61). One
// pointer per OS thread: the main thread's is null and costs one test of null, and a
// program thread's points at its own satellite_thread::stop_asked.
//
// SO A THREAD BUSY INSIDE ONE STATEMENT -- one huge multiply, one satellite.console.input
// waiting for a line -- stops when that statement ends, not before. That is the author's
// "stop" as a polite request, which is the only kind that cannot leave a list or a
// spacesuit half written behind for everyone else.

#include <atomic>

namespace satellite004 {

inline thread_local const std::atomic<bool> *stop_of_this_thread = nullptr;

// True on a thread a satellite program started, false on the main thread.
inline bool on_a_program_thread() { return stop_of_this_thread != nullptr; }

// satellite.return(satellite) REACHED ON ANY THREAD ENDS THE WHOLE PROGRAM (the author,
// 2026-09-25: "quit the interpreter from anywhere"). The thread that reaches it sets this;
// every walker -- main's and each thread's -- reads it between statements, exactly where a
// thread's own stop is read, and ends with program_returned. Set once, never cleared: a run
// that has been told to end does not start again.
inline std::atomic<bool> &program_quit()
{
    static std::atomic<bool> quit{false};
    return quit;
}

} // namespace satellite004
