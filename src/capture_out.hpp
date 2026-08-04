// Capturing what compiled C++ prints.
//
// Both paths that run C++ inside satellite -- the JIT and the g++/dlopen bridge
// -- need this, and they need it done the same way, so it lives in one place.
//
// It is in src/ rather than include/ deliberately: everything under include/ is
// preprocessed by every satellite.cxx block at runtime, and a block has no
// business seeing the machinery that watches it.
#pragma once

#include <cstdio>
#include <string>

#include <fcntl.h>
#include <unistd.h>

namespace satellite {
namespace detail {

// Redirects file descriptor 1 into a pipe for as long as it is alive.
//
// FILE DESCRIPTOR level, not a std::cout rdbuf swap.  Code compiled into
// another module -- a .so loaded with dlopen, or a module handed to ORC -- has
// its OWN iostreams state, so replacing this process's streambuf would not
// touch a thing it writes.  The descriptor is the one piece both halves
// genuinely share.
class CaptureStdout {
public:
    CaptureStdout() {
        saved_ = ::dup(1);
        int fds[2];
        if (saved_ < 0 || ::pipe(fds) != 0) return;

        // The WRITE end is what has to be non-blocking.  A pipe holds ~64 KB
        // and nothing drains it until stop() runs, so a block that prints more
        // than that would block inside the compiled code -- forever, with the
        // console frozen behind it.  Non-blocking turns a deadlock into a short
        // write: heavy output is truncated, which is survivable.
        ::fcntl(fds[1], F_SETFL, ::fcntl(fds[1], F_GETFL, 0) | O_NONBLOCK);
        ::fflush(stdout);
        ::dup2(fds[1], 1);
        ::close(fds[1]);
        read_ = fds[0];
        on_   = true;
    }

    ~CaptureStdout() { stop(); }

    CaptureStdout(const CaptureStdout&)            = delete;
    CaptureStdout& operator=(const CaptureStdout&) = delete;

    // Restores stdout and returns everything written while it was redirected.
    // Safe to call twice; the second call returns an empty string.
    std::string stop() {
        if (!on_) return "";
        on_ = false;

        std::fflush(stdout);
        ::dup2(saved_, 1);
        ::close(saved_);
        saved_ = -1;

        // fd 1 is restored and nothing else holds the write end, so read()
        // reaches EOF and the loop ends on its own.
        std::string out;
        char        buf[4096];
        ssize_t     n;
        while ((n = ::read(read_, buf, sizeof buf)) > 0)
            out.append(buf, (size_t)n);
        ::close(read_);
        read_ = -1;
        return out;
    }

    // False when the pipe could not be set up -- the process is then printing
    // to its real stdout, which is the right thing to fall back to.
    bool active() const { return on_; }

private:
    int  saved_ = -1;
    int  read_  = -1;
    bool on_    = false;
};

}  // namespace detail
}  // namespace satellite
