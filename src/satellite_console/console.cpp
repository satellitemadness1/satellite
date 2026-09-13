// The printer thread and the barrier. See satellite_console/console.hpp for
// what this is for and what it refuses to be.
//
// THE FILE IS SMALL AND EVERY LINE OF IT IS ABOUT ONE OF THREE PROMISES:
//
//   1. A LINE IS ATOMIC. The unit queued is a whole string, so two threads
//      printing at once interleave lines and never characters (DESIGN §10.1).
//   2. A DRAIN MEANS WRITTEN. When drain() returns, everything queued before it
//      has reached the fd and been flushed -- which is what lets M14 print a
//      prompt with no newline and then wait for a key.
//   3. NOTHING BLOCKS THE PROGRAM. `display` takes a lock, moves a string and
//      signals; it never waits for a write.

#include "satellite_console/console.hpp"

#include "satellite_console/reader.hpp"

#include <cstdio>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>

namespace satellite::console {

Console &Console::the()
{
    // BUILT ON FIRST USE, NOT BEFORE main(). See the header: PLAN §4.3's
    // startup floor is measured every milestone, and `satl --version` reaches
    // no line of this file.
    static Console one;
    return one;
}

Console::~Console()
{
    // A PROCESS THAT FORGOT STILL JOINS. `satl`'s run arm calls shutdown()
    // itself so the four steps happen where they can be seen -- this is the
    // backstop for every other caller, and it is why shutdown() is written to
    // be safe twice. The watchdog's `_exit` runs no destructor and that is the
    // documented cost, not an oversight of this line.
    shutdown();
}

void Console::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    start_held();
}

void Console::start_held()
{
    // THE THREAD IS SPAWNED WITH THE LOCK HELD, which is safe and is worth one
    // line: the new thread's first act is to take this mutex, so it parks until
    // the caller lets go. Nothing joins here, so there is no way round for a
    // deadlock to come.
    if (started_)
        return;
    closed_ = false;
    writing_ = false;
    started_ = true;
    printer_ = std::thread(&Console::printer, this);
}

void Console::push(std::string text)
{
    // ONE MECHANISM WITH TWO CALLERS, WHICH IS THE POINT OF start_held(). The
    // run arm calls start() so the cost is paid where PLAN §9 can attribute it;
    // this pays it if nobody did, so no path can queue into a console with no
    // printer behind it. A lazy start and an eager one written separately would
    // be two answers to when the thread exists.
    std::lock_guard<std::mutex> lock(mutex_);
    start_held();
    queue_.push_back(std::move(text));
    arrived_.notify_one();
}

void Console::display(std::string line)
{
    line += '\n';
    push(std::move(line));
}

void Console::write(std::string text) { push(std::move(text)); }

bool Console::take_mid_line()
{
    std::lock_guard<std::mutex> lock(mutex_);
    const bool was = mid_line_;
    mid_line_ = false;
    return was;
}

void Console::drain()
{
    // A CONSOLE NOTHING HAS PRINTED THROUGH RETURNS AT ONCE, AND THERE IS NO
    // GUARD FOR IT. The predicate is already true -- an empty queue and no
    // batch in flight -- so `wait` does not wait, which is the exit path of
    // every program that prints nothing and so the commonest case there is.
    //
    // AN `if (!started_) return;` STOOD HERE UNTIL A MUTATION TEST FOUND IT WAS
    // DEAD. It was written for a deadlock that cannot happen: push() starts the
    // printer, so there is no way to have work queued and no thread, and with
    // nothing queued the wait falls straight through. Removing it changed no
    // fixture's answer -- which is what a guard against an impossible state
    // does, and it made the reason harder to see rather than safer.
    std::unique_lock<std::mutex> lock(mutex_);
    emptied_.wait(lock, [this] { return queue_.empty() && !writing_; });
}

void Console::shutdown()
{
    // STEP 1 -- DRAIN. It is the same call `satellite.console.input` makes on
    // its own, which is the whole of PLAN §8's "the barrier is a prefix of the
    // shutdown and not a second mechanism".
    drain();

    // STEP 2 -- FLUSH. Normally a no-op, because the printer flushes whenever
    // it runs the queue dry (see printer() below), and it stays here anyway:
    // DESIGN §10.1 asks for the flush at the end of a run, and a step that is
    // usually redundant is not the same as one that can be dropped. Anything
    // written to stdout by something OTHER than the printer -- a diagnostic, a
    // future arm -- is sitting in the same buffer and this is what reaches it.
    fflush(stdout);

    {
        // STEP 3 -- STOP. Closing the queue to new work is what lets the
        // printer's wait return with nothing left to do.
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_)
            return;
        closed_ = true;
    }
    arrived_.notify_all();

    // STEP 4 -- JOIN.
    if (printer_.joinable())
        printer_.join();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = false;
        closed_ = false;
    }

    // STEP 5, SINCE M14 -- THE READER. After the printer, so anything the
    // program said is on the terminal before the thread that listens goes;
    // through the pipe, because a thread parked in poll() can always be
    // handed a byte where one parked bare in read() could only be leaked.
    // Idempotent like everything above it, and free when no input was ever
    // asked for -- the common case, and the reason the reader is not started
    // here-and-joined but only joined.
    Reader::the().stop();
}

size_t Console::waiting() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

int Console::width() const
{
    // Asked of the fd the printer writes, fresh on every ask -- console.hpp
    // carries the argument. The fallback is v1's 80, taken when stdout is a
    // pipe or a file and there is no terminal to have a width: a number
    // rather than a refusal, because the one program that asks in a pipeline
    // wants to wrap something, and 80 is the answer every terminal-shaped
    // tool has given that case since before this language.
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        return size.ws_col;
    return 80;
}

int Console::height() const
{
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_row > 0)
        return size.ws_row;
    return 24;
}

bool Console::printing() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return writing_;
}

void Console::printer()
{
    std::vector<std::string> batch;

    for (;;) {
        std::unique_lock<std::mutex> lock(mutex_);
        arrived_.wait(lock, [this] { return !queue_.empty() || closed_; });

        // CLOSED AND EMPTY IS THE ONLY WAY OUT, in that order: a shutdown that
        // arrived while work was queued still writes the work. Step 3 closes
        // the queue AFTER step 1 drained it, so this is a second guarantee of
        // the same thing rather than the only one -- and it is the one that
        // holds if a producer raced the close.
        if (queue_.empty())
            return;

        batch.swap(queue_);
        writing_ = true;
        lock.unlock();

        // THE ONLY PLACE THIS PROGRAM WRITES TO STDOUT. fwrite and not fputs,
        // because a satellite string may hold a NUL -- DESIGN §5's code table
        // is 16-bit and `satellite_value/render.cpp` hands back whatever it
        // decoded, so a length is the only honest way to write it.
        bool mid_line = false;
        bool wrote = false;
        for (const std::string &text : batch) {
            fwrite(text.data(), 1, text.size(), stdout);
            if (!text.empty()) {
                wrote = true;
                mid_line = text.back() != '\n';
            }
        }
        batch.clear();

        lock.lock();
        if (wrote)
            mid_line_ = mid_line;
        writing_ = false;
        if (!queue_.empty())
            continue;

        // THE FLUSH HAPPENS UNDER THE LOCK, AND THAT IS DELIBERATE. It is what
        // makes drain()'s promise "written AND flushed" rather than "handed to
        // stdio": a waiter woken by the signal below cannot be woken before the
        // bytes are out, because the signal is inside the same critical section
        // as the flush. What it costs is a producer waiting for one fflush --
        // and only when the queue is EMPTY, which is precisely when no producer
        // is busy. Under load the queue is never empty and this never runs.
        //
        // AND IT IS ALSO WHY OUTPUT DOES NOT SIT UNWRITTEN. DESIGN §10.1: glibc
        // buffers fully to a pipe or a file, "and output can sit unwritten
        // indefinitely". Flushing when the queue runs dry means a program that
        // prints and then computes for a minute has its line on the pipe now,
        // without a flush per line in the most-called path in the language.
        fflush(stdout);
        emptied_.notify_all();
    }
}

} // namespace satellite::console
