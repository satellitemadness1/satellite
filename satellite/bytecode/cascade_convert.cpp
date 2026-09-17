// satellite/bytecode/cascade_convert.cpp -- the header says what the two halves
// are for and why the cascade is bounded.

#include "cascade_convert.hpp"

#include "bytecode_registry.hpp"

#include <algorithm>
#include <atomic>
#include <thread>

namespace satellite004 {
namespace {

// What every thread in the cascade is handed. One struct because a thread's
// arguments are copied and a reference to a local would outlive its frame.
struct Cascade {
    const std::vector<std::string_view> *lines;
    std::vector<std::vector<std::bitset<16>>> *pieces;
    std::atomic<std::size_t> *done;
    std::size_t depth;   // the last line the cascade itself takes
};

// ONE THREAD, ONE LINE, AND THEN THE NEXT THREAD. Converting comes first: a
// thread start is the expensive part, so putting it after the conversion means
// line `i` is ready without ever waiting on line `i + 1`'s thread.
void cascade_line(Cascade run, std::size_t i)
{
    for (;;) {
        tokenise_one_line((*run.lines)[i], (*run.pieces)[i]);
        run.done->fetch_add(1, std::memory_order_release);

        const std::size_t next = i + 1;
        if (next >= run.depth)
            return;

        // The author's shape: this thread starts the thread that takes the next
        // line. Detached, because nothing joins it -- `done` is what says the
        // work is finished, and it is what main waits on.
        //
        // THE LAST ONE DOES NOT SPAWN, IT CONTINUES. A thread that spawned its
        // successor and then immediately returned would pay a thread start to
        // save itself one loop turn, so the tail of the cascade is a loop here.
        // That is the one place this differs from "a thread a line", and it
        // costs nothing: the threads already started are still running.
        std::thread(cascade_line, run, next).detach();
        return;
    }
}

} // namespace

void cascade_convert(const std::vector<std::string_view> &lines,
                     std::vector<std::bitset<16>> &file,
                     StartupThreads &threads,
                     std::size_t cascade_depth,
                     unsigned long long int batches)
{
    if (lines.empty())
        return;

    // EVERY LINE ITS OWN PIECE, so no two threads write to one vector and the
    // order is the index rather than the order they finished in.
    std::vector<std::vector<std::bitset<16>>> pieces(lines.size());
    std::atomic<std::size_t> done{0};

    const std::size_t depth = std::min(cascade_depth, lines.size());

    // MAIN STARTS ONE THREAD. That one starts the next, and so on down the front
    // of the file.
    if (depth > 0) {
        Cascade run{&lines, &pieces, &done, depth};
        std::thread(cascade_line, run, static_cast<std::size_t>(0)).detach();
    }

    // THEN THE 256 WARM THREADS CONTINUE (the author). Everything the cascade
    // did not take, in batches, because they are already parked and a batch
    // costs a recall rather than a thread start.
    if (depth < lines.size()) {
        const std::size_t rest = lines.size() - depth;
        const unsigned long long int jobs =
            std::max<unsigned long long int>(1, std::min<unsigned long long int>(batches, rest));
        const std::size_t per = (rest + jobs - 1) / jobs;
        for (unsigned long long int job = 0; job < jobs; ++job) {
            const std::size_t from = depth + static_cast<std::size_t>(job) * per;
            if (from >= lines.size())
                break;
            const std::size_t stop = std::min(lines.size(), from + per);
            threads.submit([&lines, &pieces, &done, from, stop] {
                for (std::size_t i = from; i < stop; ++i)
                    tokenise_one_line(lines[i], pieces[i]);
                done.fetch_add(stop - from, std::memory_order_release);
            });
        }
    }

    // UNTIL THE ENTIRE PROGRAM IS CONVERTED (the author). Main waits here, and
    // this is the seam where it will one day start RUNNING line 1 instead --
    // `done` is already the watermark that would say how far it may go.
    while (done.load(std::memory_order_acquire) < lines.size())
        std::this_thread::yield();

    std::size_t codes = 1;
    for (const std::vector<std::bitset<16>> &piece : pieces)
        codes += piece.size();
    file.reserve(file.size() + codes);
    for (const std::vector<std::bitset<16>> &piece : pieces)
        file.insert(file.end(), piece.begin(), piece.end());
}

} // namespace satellite004
