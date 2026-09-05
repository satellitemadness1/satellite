#pragma once

// The pty harness M14's terminal section drives the real `satl` with --
// forkpty(3), a screen the test reads the way a person would, and the
// child's CPU time for the no-spin clause. Split from terminal.cpp so each
// file holds one subject: this one knows terminals, that one knows clauses.
//
// gather() DRAINS UNTIL QUIET AND collect_for() READS FOR A FIXED WINDOW,
// and the difference was found the expensive way: a quiet-based read never
// returns against a program printing faster than its quiet window, so a
// clause that compares screen growth ACROSS a pause must measure the pause
// with a deadline, never with silence.

#include <cstdio>
#include <cstring>
#include <poll.h>
#include <pty.h>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace console_test {

inline std::string fixture(const char *name, const std::string &body)
{
    std::string path = std::string("/tmp/satl_m14_") + name + ".satl";
    FILE *out = fopen(path.c_str(), "w");
    if (out == nullptr)
        return path;
    const std::string program = "satellite.include(satellite)\n"
                                "satellite.capsule satellite.main()\n{\n" +
                                body + "}\n";
    fwrite(program.data(), 1, program.size(), out);
    fclose(out);
    return path;
}

struct Terminal {
    pid_t pid = -1;
    int master = -1;
    std::string screen;

    // Read whatever the screen gained within `wait_ms` of quiet.
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

    // Read for a FIXED window of wall time, however busy the screen is. The
    // quiet-based gather() above never returns against a program that prints
    // faster than its quiet window -- found by this file's own first run --
    // so the counter clauses, which compare screen growth ACROSS a pause,
    // measure the pause with a deadline rather than with silence.
    void collect_for(int window_ms)
    {
        const int slice = 20;
        for (int spent = 0; spent < window_ms; spent += slice) {
            pollfd ask{master, POLLIN, 0};
            if (poll(&ask, 1, slice) <= 0)
                continue;
            char bytes[4096];
            const ssize_t got = read(master, bytes, sizeof bytes);
            if (got > 0)
                screen.append(bytes, static_cast<size_t>(got));
        }
    }

    // Wait until the screen holds `needle`, up to `patience_ms`.
    bool wait_for(const std::string &needle, int patience_ms)
    {
        for (int spent = 0; spent < patience_ms; spent += 50) {
            if (screen.find(needle) != std::string::npos)
                return true;
            gather(50);
        }
        return screen.find(needle) != std::string::npos;
    }

    void type(const char *bytes) { (void)!write(master, bytes, strlen(bytes)); }

    // Reap, with the child's CPU time answered for clause 2.
    int finish(long *cpu_ms = nullptr)
    {
        gather(200);
        int status = 0;
        rusage used{};
        wait4(pid, &status, 0, &used);
        if (cpu_ms != nullptr)
            *cpu_ms = used.ru_utime.tv_sec * 1000 + used.ru_utime.tv_usec / 1000 +
                      used.ru_stime.tv_sec * 1000 + used.ru_stime.tv_usec / 1000;
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
    }
};

inline Terminal run_on_a_terminal(const std::string &program, unsigned short columns,
                           unsigned short rows)
{
    Terminal t;
    winsize size{};
    size.ws_col = columns;
    size.ws_row = rows;
    t.pid = forkpty(&t.master, nullptr, nullptr, &size);
    if (t.pid == 0) {
        setenv("SATL_NO_WINDOW", "1", 1);
        execl("./satl", "./satl", program.c_str(), nullptr);
        _exit(127);
    }
    return t;
}


} // namespace console_test
