#pragma once

// The reader thread -- PLAN M14, and DESIGN §10.1's printer with the polarity
// named: "the printer's producer is the program and its consumer is a
// dedicated thread; the reader is that reversed -- the dedicated thread
// blocks on stdin and pushes whole lines in, and the program asks and is
// answered at once." The invariant that survives both directions, and the
// reason no satellite program ever learns what a terminal mode is: THE
// PROGRAM'S OWN THREAD NEVER BLOCKS ON THE TERMINAL.
//
// ONE CONSUMER OF STDIN AT ANY INSTANT, AND IT IS THIS THREAD. A reader
// parked in read() and an `input()` calling getline on the walking thread
// would be two consumers racing one fd -- a half-typed answer split between
// them per syscall -- so `input` `1 5 2`-`1 5 4` asks the queue and waits,
// `typed()` `1 5 5` asks and does not, and nothing else in the process reads
// descriptor 0. M22's raw-mode prompt is the other reader the language has,
// never live at the same time; whoever takes the terminal parks this thread
// first, and the control pipe below is the mechanism that will do it.
//
// THE THREAD NEVER SITS BARE IN read(2). It parks in poll() on TWO
// descriptors -- stdin and a self-pipe -- which is what answers the two
// questions PLAN M14 left open in one mechanism:
//
//   Ctrl-C:  a process-directed SIGINT lands on an arbitrary thread, so
//            EINTR surfaces somewhere useful only by luck. The handler
//            writes one byte into the pipe (system_facts/interrupt.cpp) and
//            the wake is deterministic: poll returns, the flag is read, and
//            whoever was waiting on the queue is notified.
//   the end: a thread blocked in read() cannot be joined; one parked in
//            poll() can always be handed a byte that means stop. `satl`
//            exits cleanly -- no detach, no leak -- as the console
//            shutdown's fifth step.
//
// A blocking poll() on two fds is NOT the VMIN=0 poll LOOP QUAD.md §3.4
// retires: it blocks indefinitely at zero CPU, and done-when clause 2 is the
// assertion.
//
// NOT A POOL TENANT, like the printer and for the printer's reason -- PLAN
// §4.5.1, corrected 2026-09-02: that pool takes work that FINISHES, and this
// thread waits for the life of the run.

#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include <vector>

namespace satellite::console {

// The one reader, `Console::the()`'s shape for `Console`'s reason: there is
// one stdin and a program cannot ask for a second. Built on first use;
// the THREAD starts the first time something asks for input.
class Reader {
public:
    static Reader &the();

    // What an ask came back as. Three answers and not one empty string --
    // v1's comment ported whole: "an empty line and no line at all are
    // different answers, and a program that cannot tell them apart loops
    // forever on a closed stdin. A THIRD answer joins them: the read was
    // interrupted. Saying 'end of input' about a Ctrl-C would be a false
    // statement about the terminal."
    enum class Got : uint8_t { line, end, interrupted };

    // Ask, and wait -- the read side of `input` `1 5 2`-`1 5 4`. The caller
    // wrote its prompt and drained the console FIRST (the barrier DESIGN
    // §10.1 keeps for exactly this); what waits here is the program's thread,
    // on the queue, never on the terminal. `interrupted` is Policy's seam --
    // the same function pointer the statement boundary and sleep consult --
    // and null means nobody is listening.
    Got read_line(std::string *line, bool (*interrupted)());

    // A line, or nothing, immediately -- `typed()` `1 5 5`. True with *line
    // filled when a whole line is waiting; false when nothing is, which
    // after the end of input is forever: nobody typing is nothing, and a
    // closed stdin is nobody. An EMPTY line answers true with an empty
    // string -- return pressed is a line, M12's distinction consumed here.
    bool typed_line(std::string *line);

    // Wake the thread through the pipe and join it. Idempotent, safe with
    // the thread never started, and the console's shutdown calls it -- the
    // fifth step, after the printer's four.
    void stop();

    // What is queued, for a test to assert on. Not for the language.
    size_t lines_waiting() const;

private:
    Reader() = default;
    ~Reader();

    Reader(const Reader &) = delete;
    Reader &operator=(const Reader &) = delete;

    void start_held();
    void loop();
    void take_bytes(const char *bytes, size_t count);

    mutable std::mutex mutex_;
    std::condition_variable arrived_;

    // Whole lines, terminators stripped -- the unit pushed is the unit asked
    // for, the queue's own atomicity argument run in reverse.
    std::vector<std::string> lines_;

    // Bytes after the last newline. A final line the input ends without
    // terminating is still a line -- getline's rule, kept so `printf x |
    // satl ...` answers "x" and not the end of input.
    std::string partial_;

    std::thread thread_;
    int pipe_read_ = -1;
    int pipe_write_ = -1;
    bool started_ = false;
    bool eof_ = false;
};

} // namespace satellite::console
