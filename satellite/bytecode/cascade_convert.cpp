// satellite/bytecode/cascade_convert.cpp -- the header says what the two halves
// are for and why the cascade is bounded.
//
// THE ROW IS LAID PROGRESSIVELY, IN ORDER, AND THAT IS WHAT THE WATERMARK IS FOR.
// The first version filled every piece and then laid them all out at the end, so
// nothing could read a code until the last line of the file was converted. Now a
// LAYER walks the pieces in order and appends each one the moment it is ready,
// publishing how many codes are final. A reader below that number is reading
// codes nobody will touch again.
//
// (the author, 2026-09-16) "the main thread will be converting the first line of
// code into 16-bits, then running it, after it has run the next line, the thread
// that gets line 2 is handing it to main". The watermark is that handing.
//
// THE ROW IS RESERVED BEFORE ANYTHING IS APPENDED, and that is not an
// optimisation -- it is what makes reading it while it grows SAFE. A vector that
// reallocates moves every code it holds, and a reader holding an index into the
// old memory would be reading freed memory. Reserving an upper bound up front
// means the storage never moves, so an index below the watermark stays valid.
// The bound is the same one the single-threaded path already used: the file's
// characters plus eight codes a line.

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
    std::atomic<std::size_t> *ready;   // pieces[i] is final for every i < ready
    std::size_t depth;                 // the last line the cascade itself takes
};

// ONE THREAD, ONE LINE, AND THEN THE NEXT THREAD. Converting comes first: a
// thread start is the expensive part, so putting it after the conversion means
// line `i` is ready without ever waiting on line `i + 1`'s thread.
void cascade_line(Cascade run, std::size_t i)
{
    tokenise_one_line((*run.lines)[i], (*run.pieces)[i]);

    // `ready` ONLY EVER COUNTS UP IN ORDER, which is what lets the layer below
    // trust it: the cascade is a chain, so line i is finished before line i + 1
    // is started, and this is the only writer for the cascade's range.
    run.ready->store(i + 1, std::memory_order_release);

    const std::size_t next = i + 1;
    if (next >= run.depth)
        return;

    // The author's shape: this thread starts the thread that takes the next
    // line. Detached, because nothing joins it -- the watermark is what says the
    // work is finished, and it is what the layer waits on.
    std::thread(cascade_line, run, next).detach();
}

} // namespace

void cascade_convert(const std::vector<std::string_view> &lines,
                     std::vector<std::bitset<16>> &file,
                     StartupThreads &threads,
                     std::size_t cascade_depth,
                     unsigned long long int batches,
                     std::atomic<std::size_t> *codes_final)
{
    if (lines.empty())
        return;

    // THE STORAGE NEVER MOVES AFTER THIS, so an index below the watermark is
    // safe to read while later lines are still being appended.
    std::size_t room = 1;
    for (const std::string_view &line : lines) room += line.size() + 8;
    file.reserve(file.size() + room);

    std::vector<std::vector<std::bitset<16>>> pieces(lines.size());
    std::atomic<std::size_t> cascade_ready{0};
    std::atomic<std::size_t> pool_done{0};

    const std::size_t depth = std::min(cascade_depth, lines.size());

    // MAIN STARTS ONE THREAD. That one starts the next, down the front of the file.
    if (depth > 0) {
        Cascade run{&lines, &pieces, &cascade_ready, depth};
        std::thread(cascade_line, run, static_cast<std::size_t>(0)).detach();
    }

    // THEN THE 256 WARM THREADS CONTINUE (the author). Everything the cascade did
    // not take, in batches, because they are already parked and a batch costs a
    // recall rather than a thread start.
    unsigned long long int jobs = 0;
    if (depth < lines.size()) {
        const std::size_t rest = lines.size() - depth;
        jobs = std::max<unsigned long long int>(1, std::min<unsigned long long int>(batches, rest));
        const std::size_t per = (rest + jobs - 1) / jobs;
        for (unsigned long long int job = 0; job < jobs; ++job) {
            const std::size_t from = depth + static_cast<std::size_t>(job) * per;
            if (from >= lines.size())
                break;
            const std::size_t stop = std::min(lines.size(), from + per);
            threads.submit([&lines, &pieces, &pool_done, from, stop] {
                for (std::size_t i = from; i < stop; ++i)
                    tokenise_one_line(lines[i], pieces[i]);
                pool_done.fetch_add(stop - from, std::memory_order_release);
            });
        }
    }

    // THE LAYER. Walks the pieces in order and appends each as it becomes ready,
    // publishing the watermark after every one. This is the thread that hands
    // main its lines, and it runs ON main -- there is nothing for main to do
    // until a code is final anyway, and one fewer thread is one fewer recall.
    const std::size_t rest_from = depth;
    const std::size_t rest_count = lines.size() - depth;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i < depth) {
            while (cascade_ready.load(std::memory_order_acquire) <= i)
                std::this_thread::yield();
        } else if (rest_count > 0) {
            // The pool finishes a BATCH at a time and not in order, so the
            // watermark cannot pass the first unfinished batch. Waiting for all
            // of them once is simpler and costs nothing: by here the cascade's
            // 256 lines are already laid and main has had them for a while.
            while (pool_done.load(std::memory_order_acquire) < rest_count)
                std::this_thread::yield();
        }
        const std::vector<std::bitset<16>> &piece = pieces[i];
        file.insert(file.end(), piece.begin(), piece.end());
        if (codes_final != nullptr)
            codes_final->store(file.size(), std::memory_order_release);
    }
    (void)rest_from;
}

} // namespace satellite004
