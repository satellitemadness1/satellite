#pragma once

// Raw mode, and -- much more important -- the four ways back out of it.
//
// PLAN M22 CALLS THIS MILESTONE "THE ONLY REGISTRAR", and that sentence is the
// whole reason this file exists here rather than three milestones earlier. M6
// built the watchdog's exit path and deliberately registered NO emergency hook,
// because until there is a prompt nothing in this tree has ever put the terminal
// into a state that has to be undone. `system_facts/interrupt.hpp` has carried
// `set_emergency_exit_hook()` since then with no caller. This is the caller.
//
// WHAT RAW MODE COSTS. Reading an arrow key means turning ICANON and ECHO off,
// which gives up the kernel's line discipline for as long as a line is being
// read. So the scope is an RAII object and not a process-wide mode: raw while a
// prompt line is being read, cooked again the moment a program runs. A satellite
// program that calls `satellite.console.input` (M14) must find the terminal the
// way every other run of it does, and it does.
//
// ISIG IS THE ONE THAT MATTERS AND DESIGN §10.2 IS WHY. With ISIG off, Ctrl-C
// at the prompt arrives as the BYTE 0x03 and no SIGINT is ever raised -- so the
// prompt's Ctrl-C and a running program's Ctrl-C are two different mechanisms
// that happen to share a key. M11 built the other one (a flag the walk tests at
// the next statement boundary); this file is what makes the two coexist rather
// than one swallowing the other.
//
// THE FOUR PATHS BACK, and the count is not decoration -- each covers a way this
// process can end that the others do not:
//
//   1. ~RawMode()               -- the ordinary one: a line was read
//   2. std::atexit              -- return from main(), or exit() from anywhere
//   3. the emergency exit hook  -- M6's watchdog reaching _exit(EXIT_LIMIT)
//   4. restore_terminal()       -- by hand, and safe from a signal handler
//
// PATH 3 IS THE ONE WITH A MEASURED CONSEQUENCE. `_exit()` runs no atexit
// handlers by definition, so without the hook a memory ceiling struck while the
// prompt was reading would leave the user at a shell with no echo and no line
// editing -- a terminal that looks broken, from the one code path whose whole
// job is to fail safely. MILESTONES/M6.md §6.5 is where that was found.

namespace satellite::prompt {

// True when both ends of the prompt are a terminal. A pipe on either side means
// no raw mode, no editing and no redraw -- see line_reader.hpp, which reads
// cooked in that case rather than refusing to read at all.
bool is_interactive();

// Raw while it lives, cooked when it dies. Non-copyable because two of these
// would restore twice and the second restore would fight whatever came after.
class RawMode {
public:
    RawMode();
    ~RawMode();

    RawMode(const RawMode &) = delete;
    RawMode &operator=(const RawMode &) = delete;

    bool active() const { return active_; }

private:
    bool active_ = false;
};

// Puts the terminal back if anything changed it, and does nothing if not.
//
// ASYNC-SIGNAL-SAFE, WHICH CONSTRAINS WHAT IT MAY TOUCH. tcsetattr() is on
// POSIX's list; the flag it tests is a lock-free atomic exchanged once, so two
// arrivals cannot both restore. Nothing here allocates, locks or prints.
void restore_terminal();

} // namespace satellite::prompt
