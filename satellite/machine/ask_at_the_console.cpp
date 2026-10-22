// satellite/machine/ask_at_the_console.cpp -- see ask_at_the_console.hpp.

#include "ask_at_the_console.hpp"

#include "stop_flag.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace satellite004 {
namespace {

// THE TERMINAL AS IT WAS, put back when the read ends however it ends.
class TerminalKept {
public:
    explicit TerminalKept(bool hide)
    {
        if (::tcgetattr(STDIN_FILENO, &before_) != 0)
            return;
        kept_ = true;
        termios asking = before_;
        asking.c_lflag |= ICANON;
        if (hide) {
            asking.c_lflag &= ~static_cast<tcflag_t>(ECHO);
            asking.c_lflag |= ECHONL;   // the Enter still moves to the next line
        } else {
            asking.c_lflag |= ECHO;
        }
        ::tcsetattr(STDIN_FILENO, TCSANOW, &asking);
    }
    ~TerminalKept()
    {
        if (kept_)
            ::tcsetattr(STDIN_FILENO, TCSANOW, &before_);
    }
    TerminalKept(const TerminalKept &) = delete;
    TerminalKept &operator=(const TerminalKept &) = delete;

private:
    termios before_{};
    bool kept_ = false;
};

// A Ctrl-C WHILE A QUESTION WAITS IN A FILE RUN, where nothing has caught SIGINT and it would end satl with
// ECHO still off (FD-2's reader): caught here for the question's time, and raised again once the terminal is
// put back, so the run still ends as Ctrl-C always ended it.
volatile sig_atomic_t interrupted_here = 0;

void on_interrupt_here(int)
{
    interrupted_here = 1;
}

bool asked_to_stop()
{
    const volatile sig_atomic_t *flag = stop_flag();
    return interrupted_here != 0 || (flag != nullptr && *flag != 0);
}

// THE LONGEST LINE KEPT: room made once, before the first key, so the line never moves to a bigger block and
// leaves a copy of a password behind in the one it left (FD-2's reader). Keys past it are read and dropped.
constexpr std::size_t kLongestAnswer = 4095;

// ONE LINE, A BYTE AT A TIME, so nothing after the newline is taken from whoever reads next. A poll with a
// tenth of a second's wait, because the session's Ctrl-C is SA_RESTART (session.cpp) and a read would never
// see it: the flag is what says to stop.
bool one_line(std::string &line)
{
    line.clear();
    line.reserve(kLongestAnswer + 1);
    for (;;) {
        if (asked_to_stop())
            return false;
        pollfd waiting{STDIN_FILENO, POLLIN, 0};
        const int ready = ::poll(&waiting, 1, 100);
        if (ready < 0 && errno != EINTR)
            return false;
        if (ready <= 0)
            continue;
        char c = 0;
        const ssize_t got = ::read(STDIN_FILENO, &c, 1);
        if (got < 0 && errno == EINTR)
            continue;
        if (got <= 0)
            return false;
        if (c == '\n')
            return true;
        if (line.size() < kLongestAnswer)
            line += c;
    }
}

bool ask(const std::string &question, bool hide, std::string &answer)
{
    answer.clear();
    if (!a_console_to_ask_at())
        return false;
    // EVERYTHING DISPLAYED BEFORE IS ON THE SCREEN FIRST: a flush of std::cout waits for the printing
    // satellite (display/printing_satellite.hpp).
    std::cout << question << std::flush;
    struct sigaction mine{}, before{};
    const bool caught_here = stop_flag() == nullptr;
    if (caught_here) {
        interrupted_here = 0;
        mine.sa_handler = on_interrupt_here;
        sigemptyset(&mine.sa_mask);
        sigaction(SIGINT, &mine, &before);
    }
    bool answered = false;
    {
        const TerminalKept kept(hide);
        answered = one_line(answer);
    }
    if (!answered && hide)
        std::cout << '\n' << std::flush;   // ECHONL drew no newline for a line never ended
    if (caught_here) {
        sigaction(SIGINT, &before, nullptr);
        if (interrupted_here != 0) {
            interrupted_here = 0;
            ::raise(SIGINT);
        }
    }
    return answered;
}

} // namespace

bool a_console_to_ask_at()
{
    return ::isatty(STDIN_FILENO) == 1;
}

bool hidden_line(const std::string &question, std::string &answer)
{
    return ask(question, true, answer);
}

bool yes_or_no(const std::string &question)
{
    std::string answer;
    if (!ask(question + " [y/N] ", false, answer))
        return false;
    for (char &c : answer)
        c = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    return answer == "y" || answer == "yes";
}

} // namespace satellite004
