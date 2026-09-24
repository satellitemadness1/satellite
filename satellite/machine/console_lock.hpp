#pragma once
// satellite/machine/console_lock.hpp -- ONE LINE AT A TIME, once a program has threads.
//
// Every line satl writes -- a program's display, a report, a notice -- goes through
// std::cout or std::cerr, and satl runs them with sync_with_stdio(false)
// (structured-library.cpp), which makes the two streams unsafe to write from two threads
// at once: a line could land inside another, or worse. 003 queued whole strings for one
// printer thread; 004 holds this lock for the length of one line instead (threads, 2026-09-23).
//
// TAKEN ONLY ONCE A THREAD HAS BEEN STARTED. Until then satl is one walker, and a plain
// display pays one relaxed load and nothing else -- the author races display, and a lock
// on every line of every program that never makes a thread would be a cost for nothing.
// The flag is set by start() BEFORE the new thread exists, and only the thread calling
// start() can be running satl code at that moment, so no line is ever mid-write unlocked
// when the first thread appears.
//
// RECURSIVE, because a library called under it can fail, and the report of that failure
// is printed from the same thread while the lock is still held.

#include <atomic>
#include <mutex>

namespace satellite004 {

inline std::recursive_mutex &console_lock()
{
    static std::recursive_mutex one;
    return one;
}

// Set once, by the first start(), and never cleared.
inline std::atomic<bool> &a_thread_was_started()
{
    static std::atomic<bool> started{false};
    return started;
}

// HELD FOR ONE LINE. Holds nothing while the program has never started a thread.
class ConsoleHold {
public:
    ConsoleHold()
    {
        if (a_thread_was_started().load(std::memory_order_acquire))
            hold_ = std::unique_lock<std::recursive_mutex>(console_lock());
    }

private:
    std::unique_lock<std::recursive_mutex> hold_;
};

} // namespace satellite004
