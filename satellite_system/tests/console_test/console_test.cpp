// The console queue and its printer thread: ordering, line atomicity under
// concurrent producers, drain, and the wait on an empty queue.
//
// Every test here captures std::cout by swapping its streambuf, because what
// the Console is FOR is the bytes reaching stdout — asserting on the queue
// would test the thing that was easy to reach rather than the thing that was
// promised.

#include "console_output/console.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

// Runs `body` with std::cout redirected, and returns everything it printed.
// The Console is destroyed inside the capture, so its destructor's final drain
// is part of what gets measured.
template <typename Body>
static std::string captured(Body body)
{
    std::ostringstream sink;
    std::streambuf *saved = std::cout.rdbuf(sink.rdbuf());
    body();
    std::cout.rdbuf(saved);
    return sink.str();
}

static std::vector<std::string> lines_of(const std::string &text)
{
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
        lines.push_back(line);
    return lines;
}

int main()
{
    // One producer, in order. The queue is FIFO from a single writer, which is
    // what a single-threaded program's output depends on absolutely.
    {
        std::string out = captured([] {
            Console console;
            console.write("one\n");
            console.write("two\n");
            console.write("three\n");
        });
        check(out == "one\ntwo\nthree\n", "single producer keeps order");
    }

    // drain() means the bytes are OUT, not merely dequeued. Read immediately
    // after it returns, with the Console still alive and unflushed by any
    // destructor.
    {
        std::ostringstream sink;
        std::streambuf *saved = std::cout.rdbuf(sink.rdbuf());
        {
            Console console;
            console.write("before drain\n");
            console.drain();
            const bool arrived = sink.str() == "before drain\n";
            std::cout.rdbuf(saved);
            check(arrived, "drain waits for the write to reach stdout");
            std::cout.rdbuf(sink.rdbuf());
        }
        std::cout.rdbuf(saved);
    }

    // drain() on an idle Console returns rather than blocking forever. The
    // predicate is "empty and not printing", and an untouched queue satisfies
    // it — if it did not, every program that displayed nothing would hang.
    {
        Console console;
        console.drain();
        check(true, "drain on an empty console returns");
    }

    // The destructor prints what is still queued. A program's last line must
    // not be lost to a shutdown that beat the printer to it.
    {
        std::string out = captured([] {
            Console console;
            for (int i = 0; i < 500; i++)
                console.write("tail " + std::to_string(i) + "\n");
        });
        check(lines_of(out).size() == 500, "destructor drains what is queued");
    }

    // Line atomicity under concurrent producers — the property the whole
    // design exists for. Eight threads write 250 lines each; every line must
    // arrive WHOLE. A shared std::string with += shreds these, which is what
    // the vector-of-strings prevents.
    {
        const int threads = 8;
        const int per_thread = 250;
        std::string out = captured([&] {
            Console console;
            std::vector<std::thread> writers;
            for (int t = 0; t < threads; t++) {
                writers.emplace_back([&console, t] {
                    for (int i = 0; i < per_thread; i++)
                        console.write("thread " + std::to_string(t) +
                                      " line " + std::to_string(i) + "\n");
                });
            }
            for (std::thread &writer : writers)
                writer.join();
        });

        std::vector<std::string> lines = lines_of(out);
        check(lines.size() == static_cast<size_t>(threads * per_thread),
              "every line from every thread arrives exactly once");

        // Whole, and each one exactly the line some thread wrote. A torn line
        // would not be in this set.
        std::set<std::string> expected;
        for (int t = 0; t < threads; t++)
            for (int i = 0; i < per_thread; i++)
                expected.insert("thread " + std::to_string(t) + " line " +
                                std::to_string(i));
        bool all_whole = true;
        for (const std::string &line : lines)
            if (!expected.count(line))
                all_whole = false;
        check(all_whole, "no line is torn by a concurrent write");

        // Per-thread ORDER survives even though inter-thread order does not.
        // A thread's own writes are pushed under the lock in the order it made
        // them, so they can be separated but never reordered.
        bool per_thread_ordered = true;
        std::vector<int> next(threads, 0);
        for (const std::string &line : lines) {
            int t = 0, i = 0;
            if (sscanf(line.c_str(), "thread %d line %d", &t, &i) == 2) {
                if (t < 0 || t >= threads || i != next[t])
                    per_thread_ordered = false;
                else
                    next[t]++;
            }
        }
        check(per_thread_ordered, "each thread's own lines stay in order");
    }

    // An empty queue WAITS rather than spinning, and a write after a long
    // quiet period still wakes it. This is the "occasionally there will not be
    // any output" case: the printer sleeps on a condition variable, and the
    // only evidence available from outside is that the late line still lands.
    {
        std::string out = captured([] {
            Console console;
            console.write("early\n");
            console.drain();
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            console.write("late\n");
            console.drain();
        });
        check(out == "early\nlate\n", "a write after a quiet period wakes the printer");
    }

    // --- the pace, satellite.console.display(100ms) -------------------------
    //
    // Timed rather than structural, because a pause that does not take time is
    // not a pause. 40 ms a line is small enough to keep the suite quick and far
    // enough above scheduler noise to assert on.
    {
        const auto start = std::chrono::steady_clock::now();
        std::string out = captured([] {
            Console console;
            console.pace(40 * 1000 * 1000);
            console.write("one\n");
            console.write("two\n");
            console.write("three\n");
        });
        const double ms = std::chrono::duration<double, std::milli>(
                              std::chrono::steady_clock::now() - start)
                              .count();

        // n-1 pauses for n lines: the first line waits for nothing. Three lines
        // is therefore ~80 ms, and an upper bound is asserted too, because a
        // pause BEFORE the first line or AFTER the last would still be ~120 ms
        // and would otherwise pass unnoticed.
        check(out == "one\ntwo\nthree\n", "a paced console prints the same bytes");
        check(ms >= 75, "three lines at 40 ms take at least two pauses");
        check(ms < 115, "and not three: no pause before the first or after the last");
    }

    // The producer never waits for the pace. This is the property the whole
    // arrangement is for: the walk queues its output and carries on, and only
    // the printer sleeps.
    {
        double queue_ms = 0;
        std::string out = captured([&] {
            Console console;
            console.pace(30 * 1000 * 1000);
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < 5; i++)
                console.write("line\n");
            queue_ms = std::chrono::duration<double, std::milli>(
                           std::chrono::steady_clock::now() - start)
                           .count();
            console.drain();
        });
        check(lines_of(out).size() == 5, "all five paced lines arrive");
        check(queue_ms < 20,
              "queueing five lines at a 30 ms pace does not wait for 120 ms of it");
    }

    // The pace travels WITH the line. A program that paces two lines and then
    // turns the pacing off has queued all three calls before the printer has
    // written the first, so a printer that read the current pace would find the
    // zero and print all three at once -- which is exactly what the first
    // version of this did. The two paced lines must still be spaced.
    {
        const auto start = std::chrono::steady_clock::now();
        std::string out = captured([] {
            Console console;
            console.pace(50 * 1000 * 1000);
            console.write("paced one\n");
            console.write("paced two\n");
            console.pace(0);
            console.write("fast\n");
        });
        const double ms = std::chrono::duration<double, std::milli>(
                              std::chrono::steady_clock::now() - start)
                              .count();
        check(out == "paced one\npaced two\nfast\n", "order is unaffected by the pace");
        check(ms >= 45, "the line written while paced keeps its pause");
        check(ms < 90, "and the line written after 0ms does not get one");
    }

    // A minimum GAP, not a fixed delay. A line that is already later than the
    // pace waits for nothing -- which is the difference between spacing a
    // program's output and lagging a prompt, where a line typed after five idle
    // seconds must not wait another 1.5 s to appear.
    {
        const auto start = std::chrono::steady_clock::now();
        std::string out = captured([] {
            Console console;
            console.pace(100 * 1000 * 1000);
            console.write("first\n");
            console.drain();
            // Longer than the pace, and spent OUTSIDE the console: by the time
            // the second line is written, the gap the pace asks for has already
            // happened.
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            console.write("second\n");
        });
        const double ms = std::chrono::duration<double, std::milli>(
                              std::chrono::steady_clock::now() - start)
                              .count();
        check(out == "first\nsecond\n", "both lines arrive");
        check(ms < 230, "time already elapsed counts toward the pace");
    }

    // Zero is full speed, and negative is zero rather than an error: pace() is
    // reached from a language that cannot write a negative duration, so the
    // clamp is a floor on a value nobody can send rather than a policy.
    {
        Console console;
        check(console.pace() == 0, "a fresh console is unpaced");
        console.pace(1234);
        check(console.pace() == 1234, "pace() reads back what was set");
        console.pace(-5);
        check(console.pace() == 0, "a negative pace is no pace");
    }

    // A producer must not block behind the printer. Measured rather than
    // asserted structurally: 20,000 writes should cost far less than what
    // 20,000 flushes to a stream would, because write() only takes the mutex
    // long enough to move a string.
    double write_ms = 0;
    {
        std::string out = captured([&] {
            Console console;
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < 20000; i++)
                console.write("x\n");
            const auto stop = std::chrono::steady_clock::now();
            write_ms = std::chrono::duration<double, std::milli>(stop - start)
                           .count();
        });
        check(lines_of(out).size() == 20000, "20000 writes all arrive");
    }

    if (failures)
        return 1;
    printf("PASS: console (order from one producer, whole lines from eight, "
           "per-thread order kept, drain waits for the flush, destructor "
           "drains, an idle queue sleeps and wakes; a pace spaces n lines with "
           "n-1 pauses, is a minimum gap rather than a fixed delay, travels "
           "with the line and never blocks the producer; "
           "20000 writes queued in "
           "%.2f ms)\n",
           write_ms);
    return 0;
}
