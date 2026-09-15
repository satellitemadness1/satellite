#pragma once

// The memory watchdog -- PLAN §4.5.2, and the one clause of M6's done-when that
// is demonstrated by killing the process.
//
// §4.5.2's INSTRUCTION IS FOLLOWED LITERALLY: "keep the thread, keep the exit
// path, change what it compares, and ideally keep both checks." So both are
// here, and they are not the same guarantee:
//
//   MEMORY_MAX   against facts::process_memory_bytes() -- what THIS RUN is
//                using. The stronger promise, the easier one to explain, and
//                the one nothing has ever demonstrated. Default: the machine's
//                whole MemTotal, because a fraction would be a number satl
//                invented about a program it has never seen.
//
//   min_free_mb  against facts::mem_available_mb() -- what the MACHINE has
//                left. v1's check, ported as a regression, and it protects
//                against something else eating the machine while satl behaves.
//                Default: OFF, which is a correction to v1 and not a port of
//                it -- see the note on the default in watchdog.cpp.
//
// THE EXIT PATH IS _exit AND NOT exit, AND THAT IS THE "keep the exit path"
// half. This runs on a detached thread while main() is still running, so exit()
// would run static destructors underneath a live main thread -- the exit-time
// crash pool.cpp refuses for the same reason. _exit runs nothing, which is why
// the buffers are flushed by hand first.
//
// AND THERE IS NO TERMINAL-RESTORING HOOK HERE, WHICH IS A CORRECTION TO THE
// DRAFT THIS MILESTONE COMES FROM. v1 leaves through run_emergency_exit_hook(),
// whose only registrar anywhere in v1 is the line editor's raw mode -- M22's.
// Until a prompt exists nothing has put the terminal into raw mode and the hook
// has nothing to undo, so registering one now would be a call that cannot be
// observed and a done-when clause that cannot fail. PLAN M6 says this in as
// many words: M22 inherits the exit path and adds the registration.

namespace satellite::limits {

// Start the detached thread that wakes once a second. Called once, from
// begin(), after the limits are settled.
void start_watchdog();

// `satl --watchdog [file]`: hold this process open and let the watchdog work.
//
// A FLAG THAT EXISTS BECAUSE THE DONE-WHEN CANNOT BE MET WITHOUT ONE, and that
// is worth saying rather than leaving as an oddity in the usage list. M6 asks
// for a demonstration in which "the watchdog fires ... and the process is gone
// within a second of the threshold being crossed" -- and a watchdog that never
// fires is indistinguishable from no watchdog, so the demonstration IS the one
// that kills the process. Nothing satl does today lasts a second: the longest
// command in the tree is `satl --words` at ~0.8 ms. The tenants that will
// outlive a second -- a running program at M10, the prompt at M22 -- are two
// and sixteen milestones away, so this is the only way to watch the thread
// work, and it stays useful afterwards as the way to watch a long run's ceiling
// without being the run.
//
// IT DOES NOT RETURN, BY EITHER ROUTE, AND AN EARLIER LINE HERE SAID IT
// RETURNED EXIT_FINE ON AN INTERRUPT. Measured against the installed binary on
// 2026-08-31: the watchdog's way out is _exit(EXIT_LIMIT), and the person's is
// the interrupt, which finds no handler and kills the process -- the shell sees
// 130 and not 0. The `return EXIT_FINE` at the foot of the function is
// unreachable and is there because the arm in main() must return an int. That
// is the right status for a signal and the wrong thing to have written down.
int hold_for_the_watchdog();

} // namespace satellite::limits
