// satellite/bytecode/cascade_convert.cpp -- the header says whose design this is
// and what 200,000 threads cost.

#include "cascade_convert.hpp"

#include "bytecode_registry.hpp"

#include <atomic>
#include <thread>
#include <vector>

namespace satellite004 {

std::size_t lines_in(const ProgramText &text)
{
    std::size_t count = 0;
    for (const std::vector<std::string> &file : text)
        count += file.size();
    return count;
}

void convert_a_thread_for_each_line(const std::vector<std::string> &lines,
                                    std::vector<std::bitset<16>> &file,
                                    StartupThreads &threads)
{
    if (lines.empty())
        return;

    // ONE PIECE A LINE. Sized before any thread starts, so no thread ever grows
    // this vector and no two threads ever touch the same element -- which is
    // what makes the whole thing need no lock at all.
    std::vector<std::vector<std::bitset<16>>> pieces(lines.size());

    // ONE THREAD A LINE (the author). Held so they can be joined: the author's
    // own words are "join all of them", and a detached thread would leave main
    // with nothing to wait on but a counter.
    std::vector<std::thread> made;
    made.reserve(lines.size());

    std::atomic<std::size_t> refused{0};
    for (std::size_t i = 0; i < lines.size(); ++i) {
        try {
            made.emplace_back([&lines, &pieces, i] { tokenise_one_line(lines[i], pieces[i]); });
        } catch (const std::system_error &) {
            // THE MACHINE REFUSING A THREAD MUST NOT LOSE A LINE. At 200,000
            // lines this is a real possibility and the answer is not to fail:
            // the line goes to the warm pool, which is already parked and needs
            // no new thread at all.
            refused.fetch_add(1);
            threads.submit([&lines, &pieces, i] { tokenise_one_line(lines[i], pieces[i]); });
        }
    }

    // JOIN ALL OF THEM (the author: "we will just make this simpler than it has
    // to be hard, so we wait for all 32 threads to return").
    for (std::thread &one : made)
        one.join();

    // The refused lines went to the pool, which is not joined -- submitting a
    // job and waiting for it is what `warm()` cannot tell us, so a second pass
    // over the pieces is the honest wait. Empty only while its line is unread.
    if (refused.load() != 0) {
        bool waiting = true;
        while (waiting) {
            waiting = false;
            for (std::size_t i = 0; i < lines.size() && !waiting; ++i)
                if (pieces[i].empty() && !lines[i].empty())
                    waiting = true;
            if (waiting)
                std::this_thread::yield();
        }
    }

    // LAID BY INDEX, so the order is the program's and never the order the
    // threads happened to finish in.
    std::size_t codes = 1;
    for (const std::vector<std::bitset<16>> &piece : pieces)
        codes += piece.size();
    file.reserve(file.size() + codes);
    for (const std::vector<std::bitset<16>> &piece : pieces)
        file.insert(file.end(), piece.begin(), piece.end());
}

} // namespace satellite004
