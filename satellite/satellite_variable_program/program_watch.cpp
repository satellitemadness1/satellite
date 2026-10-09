// satellite/satellite_variable_program/program_watch.cpp -- the header says what the watcher is.

#include "program_watch.hpp"

#include "../display/printing_satellite.hpp"
#include "../machine/console_lock.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace satellite004 {
namespace {

constexpr int kQuietBeforeAHalfLine = 50;              // ms: a prompt with no end yet is shown then
constexpr std::size_t kLongestLineHeld = 64 * 1024;    // a longer line is shown in pieces this size
constexpr int kLookForTheEnd = 100;                    // ms between looks, on a kernel with no pidfd
constexpr std::size_t kInputKeptWritten = 1024 * 1024; // written input dropped from the front past this

// A PIECE OF THE PROGRAM'S OUTPUT, onto the screen in turn with satl's own lines -- through the
// printing satellite's ring, as satl's own words go (display/printing_satellite.hpp).
void to_the_screen(std::string &piece)
{
    if (piece.empty())
        return;
    const ConsoleHold one_piece;
    display_bytes(std::move(piece));
    piece.clear();
}

// WHAT HAS COME, ONTO THE SCREEN UP TO ITS LAST END OF LINE. Only the new bytes are searched, so a
// line with no end costs what its bytes cost (the review: an 80 MB line took 20.8 s searched from
// its start every time), and one longer than kLongestLineHeld goes in pieces of that size.
void new_output(std::string &waiting, const char *bytes, std::size_t size)
{
    const void *last = memrchr(bytes, '\n', size);
    if (last == nullptr) {
        waiting.append(bytes, size);
    } else {
        const std::size_t through = static_cast<std::size_t>(static_cast<const char *>(last) - bytes) + 1;
        waiting.append(bytes, through);
        to_the_screen(waiting);
        waiting.assign(bytes + through, size - through);
    }
    if (waiting.size() >= kLongestLineHeld)
        to_the_screen(waiting);
}

// IT HAS ENDED. What is in the output pipe NOW is read -- exactly that much, so a writer it left
// behind cannot keep this here (the review: a drain read forty pipes' worth after sh had gone).
void read_what_is_there(int out, std::string &waiting, std::string &chunk)
{
    int in_the_pipe = 0;
    if (ioctl(out, FIONREAD, &in_the_pipe) != 0)
        return;
    while (in_the_pipe > 0) {
        const std::size_t want = std::min(chunk.size(), static_cast<std::size_t>(in_the_pipe));
        const ssize_t got = read(out, chunk.data(), want);
        if (got < 0 && errno == EINTR)
            continue;
        if (got <= 0)
            return;
        new_output(waiting, chunk.data(), static_cast<std::size_t>(got));
        in_the_pipe -= static_cast<int>(got);
    }
}

// WHAT pass() TYPED IN, written as the program takes it and never waited on: a program that does not
// read cannot hold satl in pass(). Under program.lock, which a write to a full pipe does not keep --
// it answers at once. The input closes once everything is written and join() or end() was reached,
// or when the program closed its own.
//
// TAKEN FROM THE FRONT BY AN INDEX, input_from, and the written part dropped only once it is over a
// mebibyte and half of what is held: erasing the front after every write of a pipe's 64 KiB made
// passing 80 MB take 8.1 s (the review of steps 2-5), each erase moving everything behind it.
void write_what_was_passed(satellite_program &program, int &in)
{
    const std::lock_guard<std::mutex> hold(program.lock);
    std::string &waiting = program.input_waiting;
    while (in >= 0 && program.input_from < waiting.size()) {
        const ssize_t wrote = write(in, waiting.data() + program.input_from, waiting.size() - program.input_from);
        if (wrote > 0) {
            program.input_from += static_cast<std::size_t>(wrote);
            continue;
        }
        if (wrote < 0 && errno == EINTR)
            continue;
        if (wrote < 0 && errno == EAGAIN)
            break;    // full: POLLOUT says when there is room
        // THE PROGRAM CLOSED ITS INPUT (EPIPE): what was passed can go nowhere now.
        close(in);
        in = -1;
        program.input_fd = -1;
        waiting.clear();
        program.input_from = 0;
        program.input_gone = true;
        return;
    }
    if (program.input_from == waiting.size()) {
        waiting.clear();
        program.input_from = 0;
    } else if (program.input_from > kInputKeptWritten && program.input_from * 2 > waiting.size()) {
        waiting.erase(0, program.input_from);
        program.input_from = 0;
    }
    if (in >= 0 && program.input_closing && waiting.empty()) {
        close(in);
        in = -1;
        program.input_fd = -1;
        program.input_gone = true;
    }
}

bool something_to_write(satellite_program &program, int in)
{
    const std::lock_guard<std::mutex> hold(program.lock);
    return in >= 0 && program.input_from < program.input_waiting.size();
}

// THE RUN, UNTIL THE PROGRAM HAS ENDED: its output to the screen, what was passed to its input, and
// the end itself -- found, never reaped, here: the reaping is done under the program's lock. Answers
// whether the output pipe is still open after it -- something the program started may hold it for
// longer than the program lives.
bool watch_while_it_runs(Watch &watch, std::string &waiting, std::string &chunk)
{
    satellite_program &program = *watch.program;
    bool out_open = watch.out >= 0;
    for (;;) {
        write_what_was_passed(program, watch.in);
        pollfd ready[4];
        nfds_t count = 0;
        int at_out = -1, at_end = -1, at_wake = -1;
        if (out_open) {
            at_out = static_cast<int>(count);
            ready[count++] = {watch.out, POLLIN, 0};
        }
        if (watch.pidfd >= 0) {
            at_end = static_cast<int>(count);
            ready[count++] = {watch.pidfd, POLLIN, 0};
        }
        if (something_to_write(program, watch.in))
            ready[count++] = {watch.in, POLLOUT, 0};   // room again: the top of the loop writes
        if (watch.wake >= 0) {
            at_wake = static_cast<int>(count);
            ready[count++] = {watch.wake, POLLIN, 0};
        }
        int timeout = waiting.empty() ? -1 : kQuietBeforeAHalfLine;
        if (watch.pidfd < 0)
            timeout = timeout < 0 ? kLookForTheEnd : std::min(timeout, kLookForTheEnd);
        const int seen = poll(ready, count, timeout);
        if (seen < 0) {
            if (errno == EINTR)
                continue;
            return out_open;   // poll itself failed: the end is waited for after this, with nothing more shown
        }
        // NO PIDFD (a kernel before 5.3): the end is looked for every tenth of a second -- WNOWAIT, so
        // it is seen and left to be reaped under the lock.
        siginfo_t ended{};
        if (watch.pidfd < 0 && waitid(P_PID, static_cast<id_t>(watch.pid), &ended, WEXITED | WNOHANG | WNOWAIT) == 0 &&
            ended.si_pid == watch.pid) {
            if (out_open)
                read_what_is_there(watch.out, waiting, chunk);
            return out_open;
        }
        if (seen == 0) {
            to_the_screen(waiting);   // quiet, with a line half written: a prompt waiting for an answer
            continue;
        }
        if (at_wake >= 0 && ready[at_wake].revents != 0) {
            std::uint64_t said = 0;   // pass() or join() said look again: the top of the loop does
            [[maybe_unused]] const ssize_t taken = read(watch.wake, &said, sizeof said);
        }
        if (at_out >= 0 && ready[at_out].revents != 0) {
            const ssize_t got = read(watch.out, chunk.data(), chunk.size());
            if (got > 0)
                new_output(waiting, chunk.data(), static_cast<std::size_t>(got));
            else if (got == 0 || (errno != EINTR && errno != EAGAIN))
                out_open = false;   // every writer has closed it; the program may still be running
        }
        if (at_end >= 0 && (ready[at_end].revents & POLLIN) != 0) {
            if (out_open)
                read_what_is_there(watch.out, waiting, chunk);
            return out_open;
        }
    }
}

// WHAT THE PROGRAM LEFT RUNNING WRITES AFTER IT HAS ENDED is shown as a terminal would show it,
// until the pipe closes or the run does (the review: closing the pipe at the program's end killed
// what it left behind with SIGPIPE, and lost its last lines).
void read_what_it_left(const Watch &watch, std::string &waiting, std::string &chunk)
{
    for (;;) {
        pollfd ready[2] = {{watch.out, POLLIN, 0}, {closing_fd(), POLLIN, 0}};
        const int seen = poll(ready, 2, waiting.empty() ? -1 : kQuietBeforeAHalfLine);
        if (seen < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        if (seen == 0) {
            to_the_screen(waiting);
            continue;
        }
        if (ready[0].revents != 0) {
            const ssize_t got = read(watch.out, chunk.data(), chunk.size());
            if (got > 0)
                new_output(waiting, chunk.data(), static_cast<std::size_t>(got));
            else if (got == 0 || (errno != EINTR && errno != EAGAIN))
                return;
        }
        if (ready[1].revents != 0)
            return;
    }
}

std::string signal_named(int signal)
{
    const char *short_name = sigabbrev_np(signal);
    return std::to_string(signal) + (short_name != nullptr ? std::string(" (SIG") + short_name + ")" : std::string());
}

} // namespace

int closing_fd()
{
    static const int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    return fd;
}

void *watch_the_program(void *given)
{
    std::unique_ptr<Watch> watch(static_cast<Watch *>(given));
    satellite_program &program = *watch->program;
    std::string waiting;
    std::string chunk(64 * 1024, '\0');
    const bool still_open = watch_while_it_runs(*watch, waiting, chunk);
    to_the_screen(waiting);   // its last line, without the end it never wrote
    // WAITED FOR WITHOUT BEING REAPED, then reaped under the lock: while program.pid names it, the pid is
    // the program's or its zombie's, never one the machine has handed to another process (the review of
    // steps 2-5: stop_it read a pid the moment after it was reaped here, outside the lock).
    siginfo_t ended{};
    while (waitid(P_PID, static_cast<id_t>(watch->pid), &ended, WEXITED | WNOWAIT) < 0 && errno == EINTR) {
    }
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        int status = 0;
        while (waitpid(watch->pid, &status, 0) < 0 && errno == EINTR) {
        }
        long long code = 0;
        std::string why;
        if (WIFSIGNALED(status)) {
            code = 128 + WTERMSIG(status);
            why = "it was ended by signal " + signal_named(WTERMSIG(status));
        } else {
            code = WEXITSTATUS(status);
            why = code == 0 ? std::string() : "it ended with code " + std::to_string(code);
        }
        // ITS INPUT AND ITS WAKE GO WITH IT, under the lock pass() writes the wake under -- so pass()
        // never writes to a descriptor this has closed, or one the machine handed out again since.
        if (watch->in >= 0)
            close(watch->in);
        program.input_fd = -1;
        program.input_waiting.clear();
        program.input_from = 0;
        program.input_gone = true;
        if (watch->wake >= 0)
            close(watch->wake);
        program.input_wake = -1;
        if (watch->pidfd >= 0)
            close(watch->pidfd);
        program.pidfd = -1;
        program.pid = 0;
        program.run_ended(watch->run, code, std::move(why));
    }
    program.ended_signal.notify_all();
    if (still_open && closing_fd() >= 0) {
        read_what_it_left(*watch, waiting, chunk);
        to_the_screen(waiting);
    }
    if (watch->out >= 0)
        close(watch->out);
    watch->finished->store(true, std::memory_order_release);
    // ONE WRITER FEWER, AFTER ITS LAST HAND-OFF (machine/console_lock.hpp's programs_watched) -- a
    // hidden run, with no output to hand, was never counted.
    if (watch->out >= 0)
        programs_watched().fetch_sub(1, std::memory_order_acq_rel);
    return nullptr;
}

} // namespace satellite004
