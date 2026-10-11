// The progress line under a long listing. See listing_progress.hpp.

#include "listing_progress.hpp"

#include "listing.hpp"
#include "../display/printing_satellite.hpp"

#include <chrono>
#include <iostream>
#include <csignal>
#include <pthread.h>
#include <string>
#include <unistd.h>

namespace satellite004 {

namespace {

// 3 s, 2 min 10 s, 1 h 4 min -- whole seconds, since the guess is no finer.
std::string how_long(double seconds)
{
    const unsigned long long int whole = static_cast<unsigned long long int>(seconds + 0.5);
    if (whole < 60)
        return std::to_string(whole) + " s";
    if (whole < 3600)
        return std::to_string(whole / 60) + " min " + std::to_string(whole % 60) + " s";
    return std::to_string(whole / 3600) + " h " + std::to_string(whole % 3600 / 60) + " min";
}

void write_all(const std::string &text)
{
    std::size_t at = 0;
    while (at < text.size()) {
        const ssize_t wrote = ::write(STDOUT_FILENO, text.data() + at, text.size() - at);
        if (wrote <= 0)
            break;
        at += static_cast<std::size_t>(wrote);
    }
    the_pty_was_written_directly();   // in satl's own console, the listing's lines must not overtake it
}

} // namespace

ListingProgress::ListingProgress(bool shown, unsigned long long int total) : total_(total)
{
    if (!shown)
        return;
    // EVERYTHING DISPLAYED BEFORE THE LISTING IS ON THE SCREEN FIRST: this line is written straight
    // to the terminal from its own thread, and the printing satellite may still hold lines the
    // program displayed before it (display/printing_satellite.hpp).
    std::cout.flush();
    sigset_t all, before;
    sigfillset(&all);
    ::pthread_sigmask(SIG_BLOCK, &all, &before);   // the new thread takes this mask
    try {
        thread_ = std::thread(&ListingProgress::run, this);
    } catch (...) {
        // No thread, no line: the listing is the same without it.
    }
    ::pthread_sigmask(SIG_SETMASK, &before, nullptr);
}

ListingProgress::~ListingProgress()
{
    if (!thread_.joinable())
        return;
    {
        std::lock_guard<std::mutex> hold(mutex_);
        finished_ = true;
    }
    wake_.notify_all();
    thread_.join();
}

void ListingProgress::run()
{
    const auto start = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> hold(mutex_);
    const auto done = [this] { return finished_; };
    if (!wake_.wait_for(hold, std::chrono::seconds(1), done)) {
        do
            draw(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
        while (!wake_.wait_for(hold, std::chrono::milliseconds(250), done));
    }
    if (drawn_)
        write_all("\r\033[K");
}

// counted 412,334 of about 762,515 names (54%), about 3 s left
// counted 45,210 names, 38,114 a second
void ListingProgress::draw(double seconds)
{
    const unsigned long long int here = on_the_filesystem.load(std::memory_order_relaxed);
    const unsigned long long int there = elsewhere.load(std::memory_order_relaxed);
    std::string line;
    if (total_ > 0) {
        line = "counted " + with_commas(here) + " of about " + with_commas(total_) + " names";
        if (here >= total_)
            line += " -- nearly done";
        else if (here == 0)
            line += " (0%)";
        else
            line += " (" + std::to_string(here * 100 / total_) + "%), about " +
                    how_long(seconds * static_cast<double>(total_ - here) / static_cast<double>(here)) + " left";
    } else {
        const unsigned long long int all = here + there;
        line = "counted " + with_commas(all) + " names, " +
               with_commas(static_cast<unsigned long long int>(static_cast<double>(all) / seconds)) + " a second";
    }
    write_all("\r\033[K" + line);
    drawn_ = true;
}

} // namespace satellite004
