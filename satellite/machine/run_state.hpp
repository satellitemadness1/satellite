#pragma once
// satellite/machine/run_state.hpp -- whether the interpreter is running something
// this instant, for the status bar across satl's own console (console_status.cpp).
//
// (the author, 2026-09-22) "can you add a bottom bar that says "idle/running" ...
// I dunno if running/not running threads is possible or not, and that may slow the
// interpreter down too". It does not: the interpreter STORES this at the edges of
// a run -- a line typed at the prompt, a whole file, a window's capsule -- never
// inside one, and the desk only LOADS it, twice a second. Relaxed, because the bar
// wants the latest value and nothing is ordered by it.
//
// Header-only and one flag for the process: an inline function's static is the
// same object in every file that includes this.

#include <atomic>

namespace satellite004 {

inline std::atomic<bool> &the_interpreter_is_running()
{
    static std::atomic<bool> running{false};
    return running;
}

} // namespace satellite004
