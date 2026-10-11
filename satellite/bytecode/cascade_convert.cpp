// satellite/bytecode/cascade_convert.cpp -- the header says whose design this is.

#include "cascade_convert.hpp"

#include "bytecode_registry.hpp"

#include <algorithm>
#include <condition_variable>
#include <mutex>

namespace satellite004 {

void convert_every_line(const std::vector<std::string> &lines,
                        std::vector<std::bitset<16>> &file,
                        StartupThreads &threads,
                        unsigned long long int thread_count)
{
    if (lines.empty())
        return;

    // One chunk a thread, never more chunks than lines.
    const std::size_t chunks = static_cast<std::size_t>(
        std::max<unsigned long long int>(1, std::min<unsigned long long int>(thread_count, lines.size())));
    const std::size_t per = (lines.size() + chunks - 1) / chunks;

    std::vector<std::vector<std::bitset<16>>> pieces(chunks);
    std::mutex mutex;
    std::condition_variable finished;
    std::size_t left = chunks;

    for (std::size_t chunk = 0; chunk < chunks; ++chunk) {
        const std::size_t from = chunk * per;
        const std::size_t stop = std::min(lines.size(), from + per);
        threads.submit([&, chunk, from, stop] {
            std::vector<std::bitset<16>> &piece = pieces[chunk];
            for (std::size_t i = from; i < stop; ++i)
                tokenise_one_line(lines[i], piece);
            std::lock_guard<std::mutex> lock(mutex);
            if (--left == 0)
                finished.notify_one();
        });
    }

    // EVERY LINE BEFORE A SINGLE LINE RUNS (the author).
    {
        std::unique_lock<std::mutex> lock(mutex);
        finished.wait(lock, [&] { return left == 0; });
    }

    std::size_t codes = 1;
    for (const std::vector<std::bitset<16>> &piece : pieces)
        codes += piece.size();
    file.reserve(file.size() + codes);
    for (const std::vector<std::bitset<16>> &piece : pieces)
        file.insert(file.end(), piece.begin(), piece.end());
}

} // namespace satellite004
