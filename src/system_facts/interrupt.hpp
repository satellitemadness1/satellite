#pragma once

// Ctrl-C, as a flag the machine can read -- PLAN M11, and the port PLAN §8's
// M11 entry names: v1's `install_interrupt_handler()`, "installed without
// `SA_RESTART`, which §6 marks *hard-won; do not rediscover*". The v1 header
// and this one say the same things because the mechanism came across whole.
//
// IT LIVES IN system_facts AND NOT BESIDE THE EVALUATOR, for the reason v1
// gives: this is a process-level facility, not a terminal one and not a walk
// one. The machine never reads it directly either -- evaluator/machine.hpp's
// Policy carries a function pointer, handed in by programs/, which is the same
// seam that lets tests/eval_test test an interrupted run without this module
// ever seeing a signal.
//
// WHERE THE SIGNAL COMES FROM. The terminal's line discipline sends SIGINT to
// the foreground process group when it sees the INTR character, which is what
// makes this work identically under a shell and inside satl-term's PTY. It is
// therefore only reachable while the terminal is in COOKED mode: M22's line
// editor turns ISIG off, so a Ctrl-C typed at the prompt arrives as the byte
// 0x03 and never reaches here. That split is DESIGN §10.2's and is the whole
// design -- at the prompt a Ctrl-C cancels the line being typed, and only
// while a program is running does it need to reach the walk.
//
// WHAT IS SAFE IN THE HANDLER. Nothing but a lock-free store and a write(2).
// The count is std::atomic<unsigned> with a static_assert that it is always
// lock-free, which is what makes it one of the two things C++ permits a signal
// handler to touch; the escalation path uses write(2) and _exit(2) for the
// same reason, since printf() takes a lock that the interrupted thread may
// already hold.

namespace satellite {

// Installs the SIGINT handler. Idempotent -- call it from every entry point
// that can run a program, and the second call does nothing.
//
// SA_RESTART is deliberately NOT set. A blocked read must come back with EINTR
// so that M14's `satellite.console.input` can tell a Ctrl-C from an end of
// input; with SA_RESTART the read resumes and a program waiting for a line
// could not be interrupted at all.
void install_interrupt_handler();

// True once a SIGINT has arrived and not yet been cleared. The machine checks
// this at every statement boundary -- through the Policy pointer, not by
// calling here -- which is the one point every walk passes through often and
// cheaply.
bool interrupt_requested();

// How many have arrived since the last clear. The SECOND one escalates inside
// the handler itself -- see install_interrupt_handler's note -- so a caller
// normally sees only 0 or 1; it is exposed because a test needs to read it.
unsigned interrupt_count();

// Forgets any interrupt so far. Every run clears before it starts, or a Ctrl-C
// that stopped one program would immediately stop the next -- which matters
// today for M22's prompt running many programs in one process, and already for
// a test binary running many machines in one run.
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
// Escalation, in the handler: the FIRST SIGINT sets the flag and lets the walk
// stop itself at the next statement, which is what makes an interrupted
// program report the line it was on -- S0730's caret. The second means the
// first did not land -- the walk is inside something with no statement
// boundary in it, a long division or one enormous expression -- and there is
// nothing left to do but leave. That exit is 130, which is 128 + SIGINT and
// what every shell reports for a program killed this way; programs/
// run_command.cpp answers the same number for the orderly half, so a script
// cannot tell WHICH half fired and does not need to.
constexpr int INTERRUPT_EXIT_STATUS = 130;

// ---------------------------------------------------------------------------
// Leaving the process without unwinding
// ---------------------------------------------------------------------------
//
// The SIGINT escalation above calls _exit(2) rather than returning through
// main(). It runs no destructor and no atexit handler, which is the point of
// _exit -- but it means that whatever a line editor did to the terminal is
// still done when the process is gone, and a terminal left in raw mode has no
// echo and no line editing until the user types `reset` into a screen that
// shows them nothing.
//
// So there is exactly ONE hook, set by whoever put the terminal into a state
// it should not be left in. NOTHING IN THIS TREE REGISTERS IT YET: its only
// registrar in v1 is the line editor's raw mode, which is M22's, and the hook
// is ported with the module so that M22 finds the socket where v1 left it
// rather than adding one to a signal handler later. It is a bare function
// pointer rather than a std::function because the SIGINT handler calls it: an
// atomic pointer load and an indirect call are signal-safe, and a
// std::function's storage is not.
//
// system_facts deliberately does not learn what a terminal is. The registrar
// knows what to restore; this side only knows there is something to run.
void set_emergency_exit_hook(void (*hook)());

// Runs the hook, if one was set. Safe to call with none, and safe to call
// from a signal handler.
void run_emergency_exit_hook();

} // namespace satellite
