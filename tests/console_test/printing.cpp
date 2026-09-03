// The queue, the printer thread and the four-step shutdown. See
// tests/console_test/console_test.hpp for what is being proved.

#include "console_test.hpp"

#include "satellite_console/console.hpp"

#include <string>
#include <thread>
#include <vector>

namespace console_test {

using satellite::console::Console;

namespace {

// A line long enough that a torn write would be visible.
//
// SIXTY-FOUR CHARACTERS AND NOT FIVE, because the failure this is looking for
// is a producer's string being split across a syscall. Two threads writing
// "ab" can interleave into "aabb" and can also, by luck, not -- over ten
// thousand writes of sixty-four characters, luck does not hold.
const std::string kLine(64, 'x');

} // namespace

void section_printing()
{
    // --- everything queued is written, in order ------------------------------
    //
    // THE PRINTER THREAD IS JOINED AND NOT DETACHED, which is what this
    // actually proves. A detached printer and a process that exits produce
    // exactly this test's setup with some of the output missing, and the
    // amount missing depends on timing -- so a smaller count would pass on a
    // fast machine and fail on a loaded one. Ten thousand is enough that the
    // queue is never empty while the producer runs.
    {
        const std::string out = capture([] {
            Console &console = Console::the();
            console.start();
            for (int i = 0; i < 10000; i++)
                console.display(std::to_string(i));
        });

        size_t lines = 0;
        bool in_order = true;
        size_t at = 0;
        while (at < out.size()) {
            const size_t end = out.find('\n', at);
            if (end == std::string::npos) {
                in_order = false;
                break;
            }
            if (out.compare(at, end - at, std::to_string(lines)) != 0)
                in_order = false;
            lines++;
            at = end + 1;
        }
        check(lines == 10000, "every line queued before the shutdown is written");
        check(in_order, "and they are written in the order they were queued");
    }

    // --- a line stays atomic across threads ----------------------------------
    //
    // DESIGN §10.1's ONE SENTENCE, AND THE ONLY WAY TO SEE IT. "A line stays
    // atomic because the unit queued is a whole string." Eight threads, each
    // queueing the same 64-character line: if the queue held characters, or if
    // the printer wrote a string in two calls with the lock released between,
    // some line in the output would be 128 characters long or 32.
    //
    // AND IT IS THE PROPERTY M23 DEPENDS ON. Threads are twelve milestones
    // away; the console is what they will print through, and the guarantee is
    // cheaper to establish while there is one producer than to retrofit when
    // there are eight.
    {
        const std::string out = capture([] {
            Console &console = Console::the();
            console.start();

            std::vector<std::thread> writers;
            for (int t = 0; t < 8; t++)
                writers.emplace_back([&console] {
                    for (int i = 0; i < 500; i++)
                        console.display(kLine);
                });
            for (std::thread &writer : writers)
                writer.join();
        });

        size_t lines = 0;
        check(every_line_is(out, kLine, &lines),
              "DESIGN §10.1: every line is whole -- eight threads interleave "
              "lines and never characters, because the unit queued is a whole "
              "string");
        check(lines == 8 * 500, "and all four thousand of them arrived");
    }

    // --- the un-newlined form writes no newline ------------------------------
    //
    // PLAN §8's M10 entry calls this "display's un-newlined form, which lives
    // under `1 5 1`" and this milestone's. NO PROGRAM CAN REACH IT YET and that
    // is the grammar rather than an omission -- v1 spelled it `display(text,
    // end="")` and DESIGN §6's `args := expression { "," expression }` has no
    // named arguments at all -- so this fixture is its only caller until M14's
    // `satellite.console.input(prompt)` `1 5 3` becomes the real one.
    {
        const std::string out = capture([] {
            Console &console = Console::the();
            console.start();
            console.write("prompt> ");
        });
        check(out == "prompt> ",
              "write() queues exactly what it was given, with no terminator -- "
              "DESIGN §10.1's \"a prompt written with no trailing newline\"");
    }

    // --- drain() means written AND flushed -----------------------------------
    //
    // THE BARRIER, AND IT IS THE FIRST STEP OF THE SHUTDOWN RATHER THAN A
    // SECOND MECHANISM (the author, 2026-09-02). What makes it worth its own
    // fixture is that the queue being empty is not the claim: the claim is that
    // the BYTES ARE OUT, which is why console.cpp flushes inside the critical
    // section that signals the waiter. A drain that returned with the string
    // sitting in glibc's buffer would pass an "is the queue empty" test and
    // fail the thing the barrier exists for -- a prompt appearing after the
    // program has already blocked waiting for the answer to it.
    {
        std::string expected;
        for (int i = 0; i < 2000; i++)
            expected += "line\n";
        expected += "prompt> ";

        std::string seen_at_the_barrier;
        const std::string out = capture([&seen_at_the_barrier] {
            Console &console = Console::the();
            console.start();

            // TWO THOUSAND LINES AND NOT ONE, so that the barrier is asked
            // about a real backlog rather than about a queue with one string
            // in it.
            //
            // AND THIS DOES NOT FORCE THE `writing_` HALF OF THE WAIT, which is
            // worth saying where somebody would otherwise assume it did.
            // console.cpp's drain waits for `queue_.empty() && !writing_`; the
            // second half closes the window where the printer has swapped the
            // queue out and not yet written it. Measured 2026-09-02: dropping
            // `!writing_` still passes this fixture at 2,000 lines and at
            // 50,000, because the printer is never behind -- an fwrite to a
            // file in /tmp is a memcpy into stdio's buffer and the PRODUCER,
            // which allocates a string per line, is the slower of the two. What
            // would force it is a sink that blocks, and M14's `pty.fork()`
            // fixture is where one arrives. MILESTONES/M10.md §6 carries it as
            // a line no test holds.
            for (int i = 0; i < 2000; i++)
                console.display("line");
            console.write("prompt> ");
            console.drain();

            // READ THE DESCRIPTOR WHILE THE CONSOLE IS STILL RUNNING, AND
            // FLUSH NOTHING HERE. That is the whole fixture: an `fflush` on
            // this side would make the bytes appear whatever drain() did, and
            // the test would pass against a barrier that only waits for the
            // queue to empty. A second open of fd 1 reads from offset zero.
            FILE *back = fopen("/proc/self/fd/1", "rb");
            if (back != nullptr) {
                char buffer[4096];
                size_t got = 0;
                while ((got = fread(buffer, 1, sizeof buffer, back)) > 0)
                    seen_at_the_barrier.append(buffer, got);
                fclose(back);
            }
        });

        check(seen_at_the_barrier == expected,
              "drain() returns only when everything queued before it has "
              "reached the descriptor -- including a prompt with no newline, "
              "which is the case line buffering would hold back");
        check(out == expected,
              "and the shutdown after it adds nothing and loses nothing");
    }

    // --- a shutdown is safe twice, and a console can start again -------------
    //
    // BOTH HALVES ARE REACHED BY `satl` ITSELF. run_command.cpp shuts the
    // console down before printing diagnostics, and ~Console() shuts it down
    // again at exit; M22 runs many programs in one process, so the same
    // singleton has to be able to print for a second run.
    {
        const std::string out = capture([] {
            Console &console = Console::the();
            console.display("first run");
            console.shutdown();
            console.shutdown();
            console.display("second run");
            console.shutdown();
        });
        check(out == "first run\nsecond run\n",
              "shutdown() is safe twice and a console that has stopped starts "
              "again on the next line queued -- which is what M22 needs");
    }

    // --- a drain with nothing queued returns -------------------------------
    //
    // THE COMMONEST CASE AT M10 AND THE EASIEST TO DEADLOCK ON: a program that
    // prints nothing still takes the exit path, and a barrier that waited for a
    // printer nobody started would hang every one of them.
    {
        Console &console = Console::the();
        console.shutdown();
        console.drain();
        check(true, "drain() on a console nothing has printed through returns");
    }
}

} // namespace console_test
