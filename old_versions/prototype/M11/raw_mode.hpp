#pragma once

// Putting the terminal into raw mode, and -- much more important -- getting it
// back out again under every way this process can end.
//
// WHAT RAW MODE COSTS:
// Reading an arrow key means turning ICANON and ECHO off, which gives up the
// kernel line discipline for as long as a prompt line is being read.
// The scope is an RAII object: raw only while reading a prompt line, and cooked
// again during program execution.
//
// 4-path restoration:
// 1. Destructor
// 2. std::atexit() handler
// 3. Emergency exit hook (watchdog / SIGINT escalation before _exit(2))
// 4. restore_terminal() manual call (async-signal-safe)

namespace satellite {

// True if both ends of the prompt are a terminal.
bool prompt_is_interactive();

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

// Puts the terminal back if anything changed it. Safe to call from signal handler.
void restore_terminal();

} // namespace satellite

