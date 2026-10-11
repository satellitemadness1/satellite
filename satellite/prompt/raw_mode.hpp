#pragma once
// Raw mode while a prompt line is read, and the ways back out of it (PLAN M0.6).
// Ported from 003's satellite_prompt/raw_mode, with no dependency on either
// language: it knows a terminal and nothing else.
//
// THE SCOPE IS ONE LINE, NOT THE SESSION. A RawMode lives while a line is read
// and is gone before the line runs, so a running line finds the terminal cooked,
// with ISIG on: Ctrl-C at the prompt is the BYTE 0x03 (ISIG off here), and
// Ctrl-C while a line runs is SIGINT. Two meanings, one key, decided by what is
// running.
//
// BRACKETED PASTE IS ON WHILE RAW, and off whenever the terminal is restored. A
// paste then arrives between ESC [ 200 ~ and ESC [ 201 ~, so keys.hpp reads it
// as text rather than as keys.
//
// THE PATHS BACK, each covering an end the others do not:
//
//   1. ~RawMode()          a line was read, or abandoned
//   2. std::atexit         return from main(), or exit() from anywhere
//   3. restore_terminal()  by hand, and safe from a signal handler (SIGHUP, a
//                          second Ctrl-C)
//
// 003 had a fourth, its watchdog's emergency-exit hook. 004 has no watchdog yet;
// when one reaches _exit() it must call restore_terminal() first.
//
// NOTHING IS FLUSHED: the terminal is changed with TCSANOW both ways. 003 entered
// raw mode with TCSAFLUSH to throw away keys typed while a line ran (D0.6.5), and
// a flush takes whatever the kernel holds at that instant -- the middle of a
// paste whose start was already read, so its tail was lost, or two of its lines
// spliced into one that ran (the review of the port, 2026-09-17). D0.6.5 is kept
// by line_reader.cpp instead, which reads those keys and drops them, a paste
// whole. Restoring never waits on output a stopped terminal is not reading.

namespace satellite004::prompt {

// True when both ends are a terminal. A pipe on either side means no raw mode,
// no editing, no prompt text: line_reader.cpp reads with read(2) instead.
bool is_interactive(int in, int out);

class RawMode {
public:
    RawMode(int in, int out);
    ~RawMode();

    RawMode(const RawMode &) = delete;
    RawMode &operator=(const RawMode &) = delete;

    bool active() const { return active_; }

private:
    bool active_ = false;
};

// Puts the terminal back if raw mode changed it, and does nothing if not.
// ASYNC-SIGNAL-SAFE: tcsetattr() and write() only, behind a lock-free flag. The
// flag is set BEFORE the terminal is changed and cleared only AFTER it is put
// back, so a handler arriving at any moment in between restores it. 003 set it
// after and cleared it before, and a handler exiting in either gap left the
// terminal raw. Two arrivals may both restore, which does no harm.
void restore_terminal();

} // namespace satellite004::prompt
