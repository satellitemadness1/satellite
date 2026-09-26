#pragma once

// Putting the terminal into raw mode, and -- much more important -- getting it
// back out again under every way this process can end.
//
// WHAT RAW MODE COSTS, said here because §9 says the opposite about the PTY and
// both are true. §9's argument for keeping two processes is that "the PTY gives
// you the kernel tty line discipline for free -- echo, backspace, line editing,
// Ctrl-C", and that collapsing to one process would mean reimplementing all of
// it. Reading an arrow key means turning ICANON and ECHO off, which gives up
// exactly that line discipline for exactly as long as a prompt line is being
// read -- so console_input/ IS the reimplementation §9 warned about, paid for
// deliberately and scoped as narrowly as it can be.
//
// The scope is the whole mitigation, and it is why this is an RAII object and
// not a mode the process sits in. The terminal is raw only while a prompt line
// is being typed. The moment the line is accepted it goes back to cooked, so
// every satellite program the prompt then runs gets the kernel's line
// discipline for its own reads -- satellite.console.input() is an ordinary
// std::getline with echo and backspace working, exactly as it was before this
// module existed -- and a Ctrl-C during a run is a real SIGINT rather than a
// byte, which is what makes system_facts/interrupt.hpp work at all.
//
// A TERMINAL LEFT RAW IS THE WORST FAILURE THIS CODE HAS. There is no echo, so
// the user cannot see what they type; there is no line discipline, so Ctrl-C
// does not work; and the fix is to type `reset` into a screen that shows them
// nothing. So the restore is on four paths, not one: the destructor, an
// atexit() handler, the emergency hook the memory watchdog and the SIGINT
// escalation both run before their _exit(2), and restore_terminal() for
// anybody who needs it by hand.

namespace satellite {

// True if both ends of the prompt are a terminal. Raw mode needs stdin to be
// one to read from, and the redraw needs stdout to be one to write escape
// sequences to -- `satl < script.txt` and `satl | cat` each fail one half, and
// the reader falls back to a plain getline for both.
bool prompt_is_interactive();

// The terminal is raw for the lifetime of this object and cooked again after
// it. Constructing one when the terminal is not interactive is legal and does
// nothing, which is what keeps the caller free of a special case.
class RawMode {
public:
    RawMode();
    ~RawMode();

    RawMode(const RawMode &) = delete;
    RawMode &operator=(const RawMode &) = delete;

    // False when the terminal was left alone -- not a terminal, or tcgetattr
    // refused. The reader reads this to decide whether it may draw.
    bool active() const { return active_; }

private:
    bool active_ = false;
};

// Puts the terminal back if anything here changed it, and does nothing
// otherwise. Safe to call more than once, safe to call when no RawMode was
// ever constructed, and safe to call from a signal handler -- tcsetattr is on
// POSIX's async-signal-safe list, and the flag guarding it is a lock-free
// atomic for the same reason the SIGINT count is.
void restore_terminal();

} // namespace satellite
