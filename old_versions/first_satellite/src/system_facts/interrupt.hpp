#pragma once

// Ctrl-C, as a flag the tree walker can read.
//
// It lives in system_facts and not in console_input for the same reason
// start_memory_watchdog() does: this is a process-level facility, not a
// terminal one. The evaluator already includes system.hpp and must never be
// made to include a line editor to find out whether it has been interrupted --
// and `satl --run`, which has no line editor at all, wants exactly this half.
//
// WHERE THE SIGNAL COMES FROM. The terminal's line discipline sends SIGINT to
// the foreground process group when it sees the INTR character, which is what
// makes this work identically under a shell and inside satl-term's PTY. It is
// therefore only reachable while the terminal is in COOKED mode: the line
// editor turns ISIG off, so a Ctrl-C typed at the prompt arrives as the byte
// 0x03 and never reaches here. That split is deliberate and is the whole
// design -- at the prompt a Ctrl-C cancels the line being typed, and only
// while a program is running does it need to reach the walk.
//
// WHAT IS SAFE IN THE HANDLER. Nothing but a lock-free store and a write(2).
// The count is std::atomic<unsigned> with a static_assert that it is always
// lock-free, which is what makes it one of the two things C++ permits a signal
// handler to touch; the escalation path below uses write(2) and _exit(2) for
// the same reason, since printf() takes a lock that the interrupted thread may
// already hold.

namespace satellite {

// Installs the SIGINT handler. Idempotent -- call it from every entry point
// that can run a program, and the second call does nothing.
//
// SA_RESTART is deliberately NOT set. A blocked read must come back with EINTR
// so that satellite.console.input can tell a Ctrl-C from an end of input; with
// SA_RESTART the read resumes and a program waiting for a line could not be
// interrupted at all.
void install_interrupt_handler();

// True once a SIGINT has arrived and not yet been cleared. The evaluator
// checks this at every statement boundary, which is the same place the depth
// guard is checked and for the same reason: it is the one point every walk
// passes through often and cheaply.
bool interrupt_requested();

// How many have arrived since the last clear. The SECOND one escalates inside
// the handler itself -- see install_interrupt_handler's note -- so a caller
// normally sees only 0 or 1; it is exposed because a test needs to read it.
unsigned interrupt_count();

// Forgets any interrupt so far. Every run clears before it starts, or a Ctrl-C
// that stopped one program would immediately stop the next.
void clear_interrupt();

// BOTH PRESSES SAY SO OUT LOUD, on stderr:
//
//     SATELLITE: CTRL+C RECEIVED: QUITTING
//     SATELLITE: CTRL+C RECEIVED AGAIN: QUITTING NOW
//
// stderr rather than stdout, because a run whose stdout is piped or redirected
// -- possibly being read in another terminal entirely -- would otherwise
// swallow the one line explaining why it stopped. A program that stops without
// saying why looks like a crash.
//
// The PROMPT does not print either of them, and that is not an oversight: at a
// prompt nothing is quitting. Ctrl-C there cancels the line being typed, the
// terminal shows ^C where the cursor was, and the session carries on -- so a
// message announcing a quit would be a false statement about what just
// happened. See console_input/raw_mode.hpp for why the prompt never sees the
// signal in the first place.
//
// Escalation, in the handler: the FIRST SIGINT sets the flag and lets the walk
// stop itself at the next statement, which is what makes an interrupted
// program report the line it was on. The second means the first did not land
// -- the walk is inside something with no statement boundary in it, a long
// division or a satellite.random.ultra spin -- and there is nothing left to do
// but leave. That exit is 130, which is 128 + SIGINT and what every shell
// reports for a program killed this way.
constexpr int INTERRUPT_EXIT_STATUS = 130;

// ---------------------------------------------------------------------------
// Leaving the process without unwinding
// ---------------------------------------------------------------------------
//
// Two paths call _exit(2) rather than returning through main(): the SIGINT
// escalation above, and the memory watchdog. Neither runs a destructor and
// neither runs an atexit handler, which is the point of _exit -- but it means
// that whatever the line editor did to the terminal is still done when the
// process is gone, and a terminal left in raw mode has no echo and no line
// editing until the user types `reset` into a screen that shows them nothing.
//
// So there is exactly ONE hook, set by whoever put the terminal into a state
// it should not be left in. It is a bare function pointer rather than a
// std::function because the SIGINT handler calls it: an atomic pointer load
// and an indirect call are signal-safe, and a std::function's storage is not.
//
// system_facts deliberately does not learn what a terminal is. console_input
// registers the restore; this side only knows there is something to run.
void set_emergency_exit_hook(void (*hook)());

// Runs the hook, if one was set. Safe to call with none, and safe to call
// from a signal handler.
void run_emergency_exit_hook();

// ---------------------------------------------------------------------------
// Leaving the process without unwinding
// ---------------------------------------------------------------------------
//
// Two paths call _exit(2) rather than returning through main(): the SIGINT
// escalation above, and the memory watchdog. Neither runs a destructor and
// neither runs an atexit handler, which is the point of _exit -- but it means
// that whatever the line editor did to the terminal is still done when the
// process is gone, and a terminal left in raw mode has no echo and no line
// editing until the user types `reset` into a screen that shows them nothing.
//
// So there is exactly ONE hook, set by whoever put the terminal into a state
// it should not be left in. It is a bare function pointer rather than a
// std::function because the SIGINT handler calls it: an atomic pointer load
// and an indirect call are signal-safe and a std::function's storage is not.
//
// system_facts deliberately does not learn what a terminal is. console_input
// registers the restore; this side only knows there is something to run.
void set_emergency_exit_hook(void (*hook)());

// Runs the hook, if one was set. Safe to call with none, and safe to call
// from a signal handler.
void run_emergency_exit_hook();

} // namespace satellite
