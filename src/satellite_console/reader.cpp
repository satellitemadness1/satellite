// The reader thread and its two-descriptor park. See
// satellite_console/reader.hpp for the invariant and the two questions the
// self-pipe answers; what is here is the mechanics, and every line of it is
// about one of three promises:
//
//   1. THE PROGRAM'S THREAD NEVER BLOCKS ON THE TERMINAL. It waits on the
//      queue's condition variable or not at all; only this file's loop()
//      touches descriptor 0.
//   2. A LINE ARRIVES WHOLE OR NOT YET. Bytes accumulate in partial_ under
//      the lock and a line is pushed only when its newline has -- so typed()
//      can never answer half of what somebody is still typing.
//   3. THE THREAD CAN ALWAYS BE WOKEN. poll() on the pipe is exit and
//      Ctrl-C both; a reader that could be neither joined nor interrupted
//      would be v1's raw-mode prompt problem arriving three milestones early.

#include "satellite_console/reader.hpp"

#include "system_facts/interrupt.hpp"

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace satellite::console {

Reader &Reader::the()
{
    // Built on first use, not before main() -- Console::the()'s reason:
    // PLAN §4.3's startup floor is measured every milestone, and
    // `satl --version` reads nothing.
    static Reader one;
    return one;
}

Reader::~Reader()
{
    // The backstop for a process that forgot -- Console::~Console()'s twin.
    // The run arm reaches stop() through Console::shutdown(); this joins the
    // thread for every other caller.
    stop();
}

void Reader::start_held()
{
    if (started_)
        return;

    // O_CLOEXEC on both ends: `satl` hands itself to `satl-term` by exec
    // (DESIGN §10.4), and a control pipe inherited by a window would be a
    // second process able to stop a reader it does not have.
    int ends[2] = {-1, -1};
    if (pipe2(ends, O_CLOEXEC) != 0)
        return; // no pipe, no reader; the next ask tries again
    pipe_read_ = ends[0];
    pipe_write_ = ends[1];

    eof_ = false;
    started_ = true;

    // The handler's alarm clock, registered before the thread exists so no
    // window opens where a Ctrl-C could land between the two.
    set_interrupt_wake_fd(pipe_write_);

    thread_ = std::thread(&Reader::loop, this);
}

void Reader::take_bytes(const char *bytes, size_t count)
{
    // Under the lock, and split on newlines HERE rather than in the asker:
    // promise 2. The terminator is stripped because the unit queued is the
    // answer -- v1's getline semantics, kept whole.
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < count; i++) {
        if (bytes[i] == '\n') {
            lines_.push_back(std::move(partial_));
            partial_.clear();
        } else {
            partial_ += bytes[i];
        }
    }
    arrived_.notify_all();
}

void Reader::loop()
{
    for (;;) {
        pollfd asks[2] = {{STDIN_FILENO, POLLIN, 0},
                          {pipe_read_, POLLIN, 0}};
        if (poll(asks, 2, -1) < 0) {
            // EINTR is a signal that landed on THIS thread; the byte in the
            // pipe is the message and the next lap reads it. Anything else
            // on a poll of two descriptors this thread owns is a program
            // state that cannot be reasoned about, and stdin is treated as
            // ended rather than spun on.
            if (errno == EINTR)
                continue;
            std::lock_guard<std::mutex> lock(mutex_);
            eof_ = true;
            arrived_.notify_all();
            return;
        }

        if (asks[1].revents != 0) {
            char bytes[16];
            const ssize_t got = read(pipe_read_, bytes, sizeof bytes);
            for (ssize_t i = 0; i < got; i++) {
                if (bytes[i] == 'q')
                    return; // stop() joins right behind this
            }
            // A wake ('w', the SIGINT handler's byte): the flag is the
            // message and the pipe only the alarm clock, so notifying the
            // queue's waiters is the whole of what there is to do.
            arrived_.notify_all();
            continue;
        }

        if (asks[0].revents == 0)
            continue;

        char bytes[4096];
        const ssize_t got = read(STDIN_FILENO, bytes, sizeof bytes);
        if (got > 0) {
            take_bytes(bytes, static_cast<size_t>(got));
            continue;
        }
        if (got < 0 && errno == EINTR)
            continue;

        // ZERO IS THE END OF INPUT, AND SO IS EIO -- a pty whose other side
        // hung up answers EIO, and "the terminal went away" and "the input
        // ended" are the same fact to a program asking for a line. A final
        // unterminated line still counts as one: getline's rule, promised in
        // reader.hpp so `printf x | satl` answers "x".
        std::lock_guard<std::mutex> lock(mutex_);
        if (!partial_.empty()) {
            lines_.push_back(std::move(partial_));
            partial_.clear();
        }
        eof_ = true;
        arrived_.notify_all();
        return;
    }
}

Reader::Got Reader::read_line(std::string *line, bool (*interrupted)())
{
    std::unique_lock<std::mutex> lock(mutex_);
    start_held();
    if (!started_)
        return Got::end; // no pipe, no reader -- and a wait with no thread
                         // behind it would be a hang wearing a read's clothes

    arrived_.wait(lock, [&] {
        return !lines_.empty() || eof_ ||
               (interrupted != nullptr && interrupted());
    });

    // INTERRUPTED WINS OVER A LINE THAT ARRIVED WITH IT. DESIGN §10.2 gives
    // the key its meaning at the prompt -- cancel -- and answering a line
    // the person typed BEFORE changing their mind would be acting on an
    // instruction they revoked. The walk stops at the next boundary either
    // way; this only decides what the place or the value holds when it does.
    if (interrupted != nullptr && interrupted())
        return Got::interrupted;

    if (!lines_.empty()) {
        *line = std::move(lines_.front());
        lines_.erase(lines_.begin());
        return Got::line;
    }

    return Got::end;
}

bool Reader::typed_line(std::string *line)
{
    std::lock_guard<std::mutex> lock(mutex_);
    start_held();

    if (lines_.empty())
        return false;

    *line = std::move(lines_.front());
    lines_.erase(lines_.begin());
    return true;
}

void Reader::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_)
            return;
    }

    // Deregister BEFORE anything closes: interrupt.hpp's one rule about the
    // fd, so the handler can never write an fd number the kernel has
    // reassigned.
    set_interrupt_wake_fd(-1);

    const char leave = 'q';
    (void)!write(pipe_write_, &leave, 1);

    if (thread_.joinable())
        thread_.join();

    close(pipe_read_);
    close(pipe_write_);
    pipe_read_ = -1;
    pipe_write_ = -1;

    std::lock_guard<std::mutex> lock(mutex_);
    started_ = false;
}

size_t Reader::lines_waiting() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return lines_.size();
}

} // namespace satellite::console
