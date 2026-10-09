#pragma once
// satellite/satellite_variable_program/program_spawn.hpp -- starting a program, with no shell.
//
// THE AUTHOR ASKED FOR posix_spawn (2026-10-01: "when all that is complete with posix_spawn"),
// and this is posix_spawn's own way in, with one step added. glibc's posix_spawn is
// clone(CLONE_VM | CLONE_VFORK) and a list of steps the child takes before exec; the list has no
// step that gives a child back the stack satl was given, and machine/stack_share.hpp's rule is
// that a child gets it back ("a satl started on 1.94 GiB reserved 1.94 TiB of address space for
// its 1,024 threads"). So this makes the same clone, takes the same steps, and adds that one.
//
// MEASURED 2026-10-01 (SCRATCH.md/RUNNING_PROGRAMS/spawn_race.cpp), 2,000 runs of /usr/bin/true
// from a process holding 1,024 threads and 2 GiB, its stack widened as satl's is:
//     posix_spawn                          565 us a run   child's stack 1,986,560 KiB
//     fork + the hook (prompt_run.cpp's)  8,481 us        8,192 KiB
//     this                                  594 us        8,192 KiB
//
// WHAT THE PROGRAM IS GIVEN: its input is `in` (the pipe pass() types into), or /dev/null; its
// output and its errors are ONE stream (`out`, or /dev/null), in the order it wrote them, as
// `2>&1` makes them; every other file satl
// has open is closed; every signal satl catches, and the SIGPIPE it ignores, are back to what a
// program starts with; nothing is blocked. It stays in satl's session and process group, so it
// has satl's terminal: sudo asks for its password there, as it would under bash.

#include <string>
#include <sys/types.h>
#include <vector>

namespace satellite004 {

struct ProgramStart {
    pid_t pid = -1;            // > 0 when it started
    int pidfd = -1;            // a descriptor that becomes readable when it ends; -1 if the kernel has none
    int error = 0;             // the errno when it could not start
    std::string why;           // and that, in words
    long long code = 0;        // and the code a shell answers for it: 127 not found, 126 any other
};

// `err` AS ITS OWN STREAM, for a caller that reads a program's output apart from its errors:
// satellite.test runs its programs in a child satl whose start-up block goes to stderr
// (bytecode/test_calls.hpp). The default keeps the two one stream, as above.
inline constexpr int kErrorsWithOutput = -2;

// WHERE IT RUNS AND WHAT IT IS TOLD, for satellite.test alone (satellite_test/test_run.hpp): its
// programs run in a folder of their own, with a HOME of their own, so a refusal they exist to
// make lands in that HOME's satellite.log and not the person's. Empty, the program starts where
// satl is, in the environment above -- what satellite.variable.program and .bash always give.
struct ProgramPlace {
    std::string folder;                     // its working directory; "" is satl's own
    std::vector<std::string> environment;   // NAME=value, each one in place of satl's NAME
};

// Starts words[0] -- a path, or a name looked up on the PATH -- with every word as an argument.
// `out`, `in` and `err` are -1 for /dev/null; `err` left out is the same stream as `out`.
ProgramStart start_a_program(const std::vector<std::string> &words, int out, int in, int err = kErrorsWithOutput,
                             const ProgramPlace &place = ProgramPlace());

// A PROCESS AS /proc NAMES IT: its pid, and the moment it started -- so a pid the machine has
// handed to another process since is never taken for it.
struct ProcessSeen {
    pid_t pid = 0;
    unsigned long long started = 0;
};

// EVERY PROCESS UNDER `root` NOW: its children, theirs, and on down (the review, 2026-10-01: a
// program's own `sleep` outlived "satellite stopped it"). A program shares satl's process group --
// so a Ctrl-C reaches it, and sudo asks on satl's terminal -- and that is why stopping it means
// finding what it started rather than signalling a group.
std::vector<ProcessSeen> processes_under(pid_t root);

// Whether it is still there, and still the same process -- a zombie counts as gone.
bool still_there(const ProcessSeen &process);

// `signal` to it, when it is still the same process.
void signal_if_still_there(const ProcessSeen &process, int signal);

// The process with this pid now, or pid 0 when there is none.
ProcessSeen seen_now(pid_t pid);

// WAITS UNTIL IT HAS STOPPED (state T in /proc) or gone -- a SIGSTOP is delivered when the process
// next runs, not when it is sent -- for at most `most_ms` milliseconds.
void wait_until_stopped(const ProcessSeen &process, int most_ms);

} // namespace satellite004
