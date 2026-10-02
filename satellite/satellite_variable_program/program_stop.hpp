#pragma once
// satellite/satellite_variable_program/program_stop.hpp -- STOPPING PROGRAMS: the one end() was
// reached for, and every one still going when the run ends -- and the two lists that say which
// those are (2026-10-01). Split from bytecode/program_calls.cpp when satellite.variable.bash came
// (STEP 5), as program_watch.cpp was when pass() came; that file keeps the words.
//
// THE TWO LISTS. Every program with a run started that no join() has finished with yet, so the end
// of the run can stop it and say that no join() was reached for it -- the author's "forcing the
// user to use both .start() and .join() together" (S742); and every watcher thread not joined yet,
// since a watcher outlives its program's run while something the program left running still
// writes to the pipe.

#include "satellite_program.hpp"

#include <atomic>
#include <memory>
#include <pthread.h>

namespace satellite004 {

// ON THE END-OF-RUN LIST once, however often it is started.
void remember(const ProgramHandle &which);

// OFF IT AGAIN once its run is over -- unless another thread has started the next one, which the
// end of the run is still owed.
void forget(const ProgramHandle &which);

// A RUN'S WATCHER, kept to be joined: once it has finished, or at the end of the run.
void keep_watcher(pthread_t id, const ProgramHandle &program, std::shared_ptr<std::atomic<bool>> finished);

// WATCHERS THAT HAVE FINISHED, joined now, so a run that starts a program a million times keeps
// a list of the ones still going.
void join_finished_watchers();

// UNDER program.lock: through the pidfd, which can never reach a process given this pid later.
// Answers 0, or the machine's reason for refusing (EPERM: it runs as another user).
int signal_the_program(satellite_program &program, int signal);

// ASKED TO STOP -- the program and everything it started -- and killed if it has not a little
// later. Answers once the run has ended. `only` is the run end() means; 0 is whichever is going.
void stop_it(satellite_program &program, std::uint64_t only = 0);

// THE END OF THE RUN (structured-library.cpp): every program the run started and nobody joined is
// stopped, and reported -- when `run_ended_with` is success, the run's own ending. Answers
// program_never_joined when one was, or success.
signed long long int close_every_program(signed long long int run_ended_with);

} // namespace satellite004
