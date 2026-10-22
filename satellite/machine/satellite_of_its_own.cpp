// satellite/machine/satellite_of_its_own.cpp -- see satellite_of_its_own.hpp.

#include "satellite_of_its_own.hpp"

#include "shown.hpp"
#include "stack_share.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

// Linux 5.11's flag, spelled out where the headers are older than that.
#ifndef CLOSE_RANGE_CLOEXEC
#define CLOSE_RANGE_CLOEXEC (1U << 2)
#endif

namespace satellite004 {
namespace {

// A DESCRIPTOR AT 3 OR ABOVE, close-on-exec, program_spawn.cpp's rule: a satl started with 0, 1 or 2
// closed gets one of them back from open() or pipe(), and the child's own dup2s onto 0, 1 and 2 would
// write over it before it was used.
int above_two(int fd)
{
    if (fd < 0 || fd > 2)
        return fd;
    const int moved = fcntl(fd, F_DUPFD_CLOEXEC, 3);
    close(fd);
    return moved;
}

} // namespace

bool start_a_satellite_of_its_own(const std::vector<std::string> &words, std::string &why, const std::string &called)
{
    // EVERYTHING THE CHILD USES IS MADE HERE, BEFORE THE FORK: its path, its words, its three streams and
    // the pipe it answers on. After the fork it only hands them to the kernel.
    char self[4096];
    const ssize_t length = readlink("/proc/self/exe", self, sizeof self - 1);
    if (length <= 0) {
        why = "satl cannot read its own path (/proc/self/exe)";
        return false;
    }
    self[length] = '\0';
    std::vector<std::string> given{"--console"};
    given.insert(given.end(), words.begin(), words.end());
    std::string name = called.empty() ? std::string(self) : called;
    std::vector<char *> argv{name.data()};
    for (std::string &word : given)
        argv.push_back(word.data());
    argv.push_back(nullptr);

    // WHERE THIS satl WAS STARTED is the C stderr's descriptor; one that cannot be had is /dev/null --
    // what the new satl says there is lost, and still never written on this console.
    const int nothing = above_two(open("/dev/null", O_RDWR | O_CLOEXEC));
    int started = fcntl(fileno(stderr), F_DUPFD_CLOEXEC, 3);
    if (started < 0 && nothing >= 0)
        started = fcntl(nothing, F_DUPFD_CLOEXEC, 3);
    int report[2] = {-1, -1};
    if (pipe2(report, O_CLOEXEC) == 0) {
        report[0] = above_two(report[0]);
        report[1] = above_two(report[1]);
    }
    if (nothing < 0 || started < 0 || report[0] < 0 || report[1] < 0) {
        why = std::strerror(errno);
        for (const int fd : {nothing, started, report[0], report[1]})
            if (fd >= 0)
                close(fd);
        return false;
    }
    sigset_t none;
    sigemptyset(&none);

    const pid_t middle = fork();
    if (middle == 0) {
        // THE MIDDLE ONE starts the new satl and ends at once, so the new satl is never this satl's child.
        const pid_t program = _Fork();
        if (program > 0)
            _exit(0);
        if (program == 0) {
            setsid();
            // GLib's five, which the menu's satl was started with until this day; and nothing blocked.
            for (const int each : {SIGPIPE, SIGINT, SIGTERM, SIGHUP, SIGCHLD})
                signal(each, SIG_DFL);
            sigprocmask(SIG_SETMASK, &none, nullptr);
            if (dup2(nothing, STDIN_FILENO) == STDIN_FILENO && dup2(started, STDOUT_FILENO) == STDOUT_FILENO &&
                dup2(started, STDERR_FILENO) == STDERR_FILENO) {
                // EVERY OTHER FILE THIS satl HAS OPEN STAYS THIS satl's -- its pty, a program's file, the
                // console's store -- closed by the exec, and the pipe with them, which is how the parent
                // learns the exec happened. A kernel before 5.11 leaves them as the fork did.
#ifdef SYS_close_range
                syscall(SYS_close_range, 3U, ~0U, CLOSE_RANGE_CLOEXEC);
#endif
                hand_a_child_the_stack_satl_was_given();   // stack_share.hpp
                execv(self, argv.data());
            }
        }
        // THE SECOND FORK, A dup2 OR THE EXEC FAILED: its errno goes back up the pipe.
        const int failed = errno;
        const ssize_t wrote = write(report[1], &failed, sizeof failed);
        (void)wrote;
        _exit(127);
    }
    const int forked = errno;
    close(report[1]);
    close(nothing);
    close(started);
    if (middle < 0) {
        close(report[0]);
        why = std::strerror(forked);
        return false;
    }
    // THE ONE WAIT, for the middle one, which ends as soon as it has forked.
    int status = 0;
    while (waitpid(middle, &status, 0) < 0 && errno == EINTR) {}
    // AND THE PIPE: closed with nothing in it once the new satl's exec has closed it, an errno when it failed.
    int failed = 0;
    ssize_t got;
    while ((got = read(report[0], &failed, sizeof failed)) < 0 && errno == EINTR) {}
    close(report[0]);
    if (got == static_cast<ssize_t>(sizeof failed)) {
        why = shown(self) + ": " + std::strerror(failed);
        return false;
    }
    // AND A MIDDLE ONE THAT DID NOT END WITH 0, the one end it has once it has forked: a signal ended it
    // first -- satl's own hangup handler in a fork of satl ends it with 129 -- and it closed the pipe empty
    // with no new satl started. GLib's start said so too, for want of the new one's pid.
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        why = "the start was ended before satl could run";
        return false;
    }
    return true;
}

} // namespace satellite004
