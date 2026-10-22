// satellite/machine/as_another_account.cpp -- see as_another_account.hpp.

#include "as_another_account.hpp"

#include "stack_share.hpp"
#include "stop_flag.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace satellite004 {
namespace {

constexpr std::size_t kMostKept = 64 * 1024;   // a program's words are kept up to this, for `why`

bool asked_to_stop()
{
    const volatile sig_atomic_t *flag = stop_flag();
    return flag != nullptr && *flag != 0;
}

// THE CHILD'S HALF, between fork and exec: only calls a signal handler may make, because the other threads
// of satl were copied mid-step. `terminal` is the pty for su, made its controlling terminal; for sudo it is
// -1 and `in` and `out` are the pipes.
[[noreturn]] void become(const char *program, char *const argv[], char *const envp[], int terminal, int in, int out)
{
    sigset_t none;
    sigemptyset(&none);
    sigprocmask(SIG_SETMASK, &none, nullptr);
    struct sigaction plain{};
    plain.sa_handler = SIG_DFL;
    for (int signal = 1; signal < NSIG; ++signal)
        sigaction(signal, &plain, nullptr);
    hand_a_child_the_stack_satl_was_given();   // machine/stack_share.hpp
    setsid();
    if (terminal >= 0) {
        ioctl(terminal, TIOCSCTTY, 0);
        in = out = terminal;
    }
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);
    dup2(out, STDERR_FILENO);
    close_range(3, ~0U, 0);
    execve(program, argv, envp);
    static const char failed[] = "could not be started\n";
    (void)!write(STDERR_FILENO, failed, sizeof failed - 1);
    _exit(127);
}

struct Child {
    pid_t pid = -1;
    std::string why;   // why it could not start
};

Child start(const std::vector<std::string> &words, int terminal, int in, int out)
{
    std::vector<std::string> text = words;
    std::vector<char *> argv;
    for (std::string &word : text)
        argv.push_back(word.data());
    argv.push_back(nullptr);
    static char path[] = "PATH=/usr/bin:/bin", language[] = "LC_ALL=C", term[] = "TERM=dumb";
    char *envp[] = {path, language, term, nullptr};
    Child child;
    child.pid = ::fork();
    if (child.pid == 0)
        become(words.front().c_str(), argv.data(), envp, terminal, in, out);
    if (child.pid < 0)
        child.why = words.front() + " could not be started: " + std::strerror(errno);
    return child;
}

int ended_with(pid_t pid)
{
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

// READS WHAT IS THERE, waiting at most `ms`: false once the other end is closed (EIO on a pty whose child has
// gone, 0 on a pipe).
bool read_some(int fd, std::string &kept, int ms)
{
    pollfd waiting{fd, POLLIN, 0};
    const int ready = ::poll(&waiting, 1, ms);
    if (ready < 0)
        return errno == EINTR;
    if (ready == 0)
        return true;
    char buffer[4096];
    const ssize_t got = ::read(fd, buffer, sizeof buffer);
    if (got < 0)
        return errno == EINTR || errno == EAGAIN;
    if (got == 0)
        return false;
    if (kept.size() < kMostKept)
        kept.append(buffer, static_cast<std::size_t>(got));
    return true;
}

void write_and_wipe(int fd, const std::string &password)
{
    std::string line = password + "\n";
    std::size_t at = 0;
    while (at < line.size()) {
        const ssize_t put = ::write(fd, line.data() + at, line.size() - at);
        if (put < 0 && errno == EINTR)
            continue;
        if (put <= 0)
            break;
        at += static_cast<std::size_t>(put);
    }
    explicit_bzero(line.data(), line.size());
}

// THE LAST THING THE PROGRAM SAID, on one line: \r dropped (a pty writes \r\n), blank lines passed over.
std::string last_line(const std::string &said)
{
    std::string line, last;
    for (const char c : said) {
        if (c == '\r')
            continue;
        if (c == '\n') {
            if (line.find_first_not_of(" \t") != std::string::npos)
                last = line;
            line.clear();
        } else {
            line += c;
        }
    }
    if (line.find_first_not_of(" \t") != std::string::npos)
        last = line;
    return last;
}

// A LINE OF su'S OR sudo'S OWN, which starts with the words -- never a line of the command's, which may quote a
// file named "Sorry, try again" (FD-2's reader).
bool says(const std::string &said, const char *words)
{
    const std::string wanted(words);
    std::size_t at = 0;
    while (at < said.size()) {
        while (at < said.size() && (said[at] == '\r' || said[at] == '\n'))
            ++at;
        if (said.compare(at, wanted.size(), wanted) == 0)
            return true;
        at = said.find('\n', at);
    }
    return false;
}

// A LINE OF sudo'S ENDING IN THESE WORDS: "madness is not in the sudoers file." -- a line, never a part of one.
bool ends_a_line(const std::string &said, const char *words)
{
    const std::string wanted(words);
    for (std::size_t at = said.find(wanted); at != std::string::npos; at = said.find(wanted, at + 1)) {
        const std::size_t after = at + wanted.size();
        if (after == said.size() || said[after] == '\r' || said[after] == '\n')
            return true;
    }
    return false;
}

// su ASKING SOMETHING MORE once the password is in -- an expired password's "Current password:", a code: what
// it said last ends in ": " with no newline, ECHO is off, and nothing more has come for a second. Nobody here
// can answer it, so su is ended rather than waited on for ever (FD-2's reader).
bool asking_again(const std::string &said, std::size_t typed_at, int master, int quiet_ms)
{
    if (said.size() <= typed_at || quiet_ms < 1000)
        return false;
    const std::size_t end = said.find_last_not_of(' ');
    if (end == std::string::npos || said[end] != ':' || end < typed_at)
        return false;
    termios now{};
    return ::tcgetattr(master, &now) == 0 && (now.c_lflag & ECHO) == 0;
}

} // namespace

AccountRun as_owner(const std::string &account, const std::string &password, const std::vector<std::string> &words,
                    const std::string &su)
{
    AccountRun run;
    int master = -1, terminal = -1;
    if (::openpty(&master, &terminal, nullptr, nullptr, nullptr) != 0) {
        run.why = std::string("could not make a terminal for su: ") + std::strerror(errno);
        return run;
    }
    ::fcntl(master, F_SETFD, FD_CLOEXEC);
    std::vector<std::string> argv{su, "-c", "exec \"$0\" \"$@\"", "--", account};
    argv.insert(argv.end(), words.begin(), words.end());
    const Child child = start(argv, terminal, -1, -1);
    ::close(terminal);
    if (child.pid < 0) {
        ::close(master);
        run.why = child.why;
        return run;
    }

    // THE PASSWORD GOES IN WHEN su HAS ASKED FOR IT AND TURNED ECHO OFF -- the pty's settings are the same from
    // either end, so the master can see it.
    std::string said;
    bool typed = false, asked_more = false;
    std::size_t typed_at = 0, heard = 0;
    int quiet_ms = 0;
    while (read_some(master, said, 100)) {
        quiet_ms = said.size() == heard ? quiet_ms + 100 : 0;
        heard = said.size();
        if (asked_to_stop())
            ::kill(child.pid, SIGTERM);
        if (typed) {
            if (!asked_more && asking_again(said, typed_at, master, quiet_ms)) {
                asked_more = true;
                ::kill(child.pid, SIGTERM);
            }
            continue;
        }
        if (said.find("assword") == std::string::npos)
            continue;
        termios now{};
        if (::tcgetattr(master, &now) == 0 && (now.c_lflag & ECHO) == 0) {
            write_and_wipe(master, password);
            typed = true;
            typed_at = said.size();
        }
    }
    ::close(master);
    const int code = ended_with(child.pid);
    run.done = code == 0;
    if (!run.done) {
        run.wrong_password = says(said, "su: Authentication failure");
        run.why = run.wrong_password ? "that password was not " + account + "'s"
                  : asked_more       ? "su asked for more than " + account + "'s password (" + last_line(said) +
                                     "), and was ended"
                                     : last_line(said);
    }
    return run;
}

AccountRun with_sudo(const std::string &password, const std::vector<std::string> &words, const std::string &sudo)
{
    AccountRun run;
    int in[2], out[2];
    if (::pipe2(in, O_CLOEXEC) != 0) {
        run.why = std::string("could not make a pipe for sudo: ") + std::strerror(errno);
        return run;
    }
    if (::pipe2(out, O_CLOEXEC) != 0) {
        run.why = std::string("could not make a pipe for sudo: ") + std::strerror(errno);
        ::close(in[0]);
        ::close(in[1]);
        return run;
    }
    std::vector<std::string> argv{sudo, "-S", "-k", "-p", "", "--"};
    argv.insert(argv.end(), words.begin(), words.end());
    const Child child = start(argv, -1, in[0], out[1]);
    ::close(in[0]);
    ::close(out[1]);
    if (child.pid >= 0)
        write_and_wipe(in[1], password);
    ::close(in[1]);
    std::string said;
    if (child.pid >= 0)
        while (read_some(out[0], said, 100))
            if (asked_to_stop())
                ::kill(child.pid, SIGTERM);
    ::close(out[0]);
    if (child.pid < 0) {
        run.why = child.why;
        return run;
    }
    const int code = ended_with(child.pid);
    run.done = code == 0;
    if (!run.done) {
        run.wrong_password = says(said, "Sorry, try again") || says(said, "sudo: 1 incorrect password") ||
                             says(said, "sudo: no password was provided");
        run.not_allowed = ends_a_line(said, " is not in the sudoers file.") ||
                          ends_a_line(said, " is not in the sudoers file") || says(said, "Sorry, user ");
        run.why = run.wrong_password ? "that password was not right"
                  : run.not_allowed  ? "this account may not use sudo: " + last_line(said)
                                     : last_line(said);
    }
    return run;
}

} // namespace satellite004
