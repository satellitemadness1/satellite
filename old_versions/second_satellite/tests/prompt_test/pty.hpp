#pragma once

// A terminal with the real `satl --repl` on the far end of it.
//
// NOT SHARED WITH console_test/pty.hpp, AND THE DIFFERENCE IS THE SUBJECT
// RATHER THAN THE CODE. That harness runs `./satl <program>` and reads what a
// program printed; this one runs `./satl --repl`, types AT it, and has to send
// raw control bytes -- 0x03 for Ctrl-C, `ESC [ A` for Up -- which a suite about
// a program's output never needs. What the two share is the argument for using
// a pty at all, and it is worth restating because it is the whole reason this
// file exists: with a pipe on stdin there is no terminal, so raw_mode does
// nothing, ISIG is never turned off, and Ctrl-C would be a SIGNAL rather than a
// byte. A test of the prompt's Ctrl-C run through a pipe tests the opposite of
// what it claims to.

#include <cstring>
#include <poll.h>
#include <pty.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace prompt_test {

struct Prompt {
    pid_t pid = -1;
    int master = -1;
    std::string screen;

    void gather(int wait_ms)
    {
        for (;;) {
            pollfd ask{master, POLLIN, 0};
            if (poll(&ask, 1, wait_ms) <= 0)
                return;
            char bytes[4096];
            const ssize_t got = read(master, bytes, sizeof bytes);
            if (got <= 0)
                return;
            screen.append(bytes, static_cast<size_t>(got));
        }
    }

    bool wait_for(const std::string &needle, int patience_ms)
    {
        for (int spent = 0; spent < patience_ms; spent += 50) {
            if (screen.find(needle) != std::string::npos)
                return true;
            gather(50);
        }
        return screen.find(needle) != std::string::npos;
    }

    void type(const char *bytes)
    {
        (void)!write(master, bytes, strlen(bytes));
        // A SETTLE AFTER EVERY WRITE, because the prompt echoes and redraws:
        // typing the next line before the last has been drawn interleaves two
        // redraws and the screen this test reads stops being the screen a
        // person would see.
        gather(120);
    }

    void line(const std::string &text)
    {
        type((text + "\r").c_str());
        gather(250);
    }

    int finish()
    {
        gather(250);
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
    }
};

inline Prompt start_a_prompt(unsigned short columns = 100,
                             unsigned short rows = 40)
{
    Prompt p;
    winsize size{};
    size.ws_col = columns;
    size.ws_row = rows;
    p.pid = forkpty(&p.master, nullptr, nullptr, &size);
    if (p.pid == 0) {
        // WITHOUT THIS THE CHILD HANDS ITSELF TO satl-term. programs/
        // window_handover.hpp: `satl` started with a terminal on its output
        // re-executes itself inside a window, and a test that let it would be
        // reading an empty pty while a window opened on somebody's desktop.
        setenv("SATL_NO_WINDOW", "1", 1);
        execl("./satl", "./satl", "--repl", nullptr);
        _exit(127);
    }
    return p;
}

} // namespace prompt_test
