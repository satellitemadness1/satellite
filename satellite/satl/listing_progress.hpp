#pragma once
// satellite/satl/listing_progress.hpp -- the line that says how far a long listing has
// got and how long it has left (the author, 2026-09-25: "when directory is ran on the
// root directory, we need to create a special thread for it that tries to guess how
// much time is left").
//
// HOW THE TIME LEFT IS KNOWN. A walk cannot know how long it will take, but a
// filesystem knows how many names it holds -- statvfs's used inodes, the count df -i
// gives, answered at once. So when the listed directory is the top of its filesystem
// (/ is, and so is the top of every drive), the walk has its total before it starts:
// the names on that filesystem. The walk adds one for each name it passes; this
// thread wakes four times a second, and the time left is the time so far, times what
// is left, over what is done. A directory partway down a filesystem has no total
// anyone can give, so its line says how many and how fast instead.
//
// ONE THREAD, AND ONLY WHILE A TABLE IS COUNTED. It says nothing for the first second,
// so a quick listing shows nothing; after that it redraws one line in place, and
// takes the line off the screen before the table is drawn. Only at a terminal: in a
// pipe, a line redrawn with \r would be noise in a file. Every signal is blocked on
// it, so Ctrl-C always lands on the thread doing the walk.

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace satellite004 {

class ListingProgress {
public:
    // `total`: the names the walk will pass on the listed directory's filesystem, or 0
    // when nobody knows. The thread starts only when `shown`.
    ListingProgress(bool shown, unsigned long long int total);
    ~ListingProgress();   // stops the thread, and takes its line off the screen
    ListingProgress(const ListingProgress &) = delete;
    ListingProgress &operator=(const ListingProgress &) = delete;

    std::atomic<unsigned long long int> on_the_filesystem{0};   // names counted where `total` counts
    std::atomic<unsigned long long int> elsewhere{0};           // names counted on filesystems below it

private:
    void run();
    void draw(double seconds);

    const unsigned long long int total_;
    bool drawn_ = false;
    bool finished_ = false;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::thread thread_;   // last, so everything it reads exists before it starts
};

} // namespace satellite004
