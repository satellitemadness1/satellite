#pragma once

// ~/.satl/satellite.log -- the warnings a run raised, kept -- THREAD.md T1.
//
// THE AUTHOR'S DECISION OF 2026-09-13, in their words: a second `join()` "prints
// a warning in the interpreter and keeps running", and it is recorded in
// satellite.log, where "it just permanently sits in that file until you or I
// clear it out". Q4's shared file handle is recorded there too. So a warning
// has two destinations and they are not the same promise:
//
//   the log     written THE MOMENT the warning happens, one append per line,
//               and never truncated by satellite. A program that later hangs
//               or is killed still leaves the line behind.
//   the screen  printed by the entry point when the run ends, beside its
//               errors. Mid-run there is no channel for it: the console queues
//               stdout on a printer thread, and a line written straight to
//               stderr would land above output the program produced first.
//
// ONE FILE PER USER, under ~/.satl because that is where satl is installed
// (the author's choice over the program's directory or the current one, which
// would scatter the file). A log that cannot be written is not a reason to stop
// a program, so `warn` falls back to printing -- a warning never disappears.
//
// HEADER-ONLY, because every binary that links a thread or a file handle
// would otherwise need one more line in eleven link lists.

#include "error_reporter/report.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace satellite::errors::log {

namespace detail {

struct State {
    std::mutex lock;
    std::string program;             // what each line names; "" = unknown
    std::string file;                // "" = ~/.satl/satellite.log
    std::vector<Diagnostic> to_print;
};

inline State &state()
{
    static State the;
    return the;
}

// The home directory, from $HOME or the password database -- the order
// user_facts.cpp uses, and for its reason: $HOME is what the person set.
inline std::string home()
{
    if (const char *set = std::getenv("HOME"); set != nullptr && *set != '\0')
        return set;
    if (const passwd *entry = getpwuid(getuid()); entry && entry->pw_dir)
        return entry->pw_dir;
    return {};
}

} // namespace detail

// THE PROGRAM EACH LINE NAMES. Entry points set it before the run.
inline void set_program(std::string path)
{
    // ABSOLUTE WHEN IT IS A FILE, because the log outlives the directory the
    // person was standing in: "prog.satl line 21" names nothing a week later.
    if (char *whole = ::realpath(path.c_str(), nullptr)) {
        path = whole;
        std::free(whole);
    }
    std::lock_guard<std::mutex> held(detail::state().lock);
    detail::state().program = std::move(path);
}

// WHERE THE LOG IS WRITTEN INSTEAD -- tests/eval_test, so a fixture's warnings
// do not sit in the author's log for ever. Empty restores the real one.
inline void redirect(std::string file)
{
    std::lock_guard<std::mutex> held(detail::state().lock);
    detail::state().file = std::move(file);
}

inline std::string path()
{
    std::lock_guard<std::mutex> held(detail::state().lock);
    if (!detail::state().file.empty())
        return detail::state().file;
    const std::string home = detail::home();
    return home.empty() ? std::string() : home + "/.satl/satellite.log";
}

// ONE LINE, APPENDED WHOLE. `O_APPEND` and a single write(2) are what keep two
// satl processes, or two threads, from interleaving inside a line.
//
//     2026-09-13 05:02:11  pid 4121  warning S1404  prog.satl line 21: <sentence>
inline bool append(const Diagnostic &warning)
{
    std::string program;
    bool redirected = false;
    {
        std::lock_guard<std::mutex> held(detail::state().lock);
        program = detail::state().program;
        redirected = !detail::state().file.empty();
    }
    const std::string file = path();
    if (file.empty())
        return false;
    // ~/.satl ITSELF IS MADE IF IT IS MISSING -- one directory, never a tree.
    if (!redirected) {
        const std::string directory = file.substr(0, file.rfind('/'));
        if (::mkdir(directory.c_str(), 0755) != 0 && errno != EEXIST)
            return false;
    }

    char when[32] = "";
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    if (localtime_r(&now, &local) != nullptr)
        std::strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", &local);
    std::string line = std::string(when) + "  pid " + std::to_string(getpid()) +
                       "  warning " + std::string(code_text(warning.code).view()) +
                       "  " + (program.empty() ? "(no file)" : program);
    if (warning.at.somewhere())
        line += " line " + std::to_string(warning.at.line);
    line += ": " + sentence(warning) + "\n";

    const int fd = ::open(file.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC,
                          0644);
    if (fd < 0)
        return false;
    size_t sent = 0;
    bool whole = true;
    while (sent < line.size()) {
        const ssize_t put = ::write(fd, line.data() + sent, line.size() - sent);
        if (put < 0 && errno == EINTR)
            continue;
        if (put <= 0) {
            whole = false;
            break;
        }
        sent += static_cast<size_t>(put);
    }
    ::close(fd);
    return whole;
}

// RECORD A WARNING. `print` is whether the person sees it when the run ends;
// a warning the log could not take is printed whatever `print` says.
inline void warn(const Diagnostic &warning, bool print)
{
    const bool kept = append(warning);
    if (!print && kept)
        return;
    std::lock_guard<std::mutex> held(detail::state().lock);
    detail::state().to_print.push_back(warning);
}

// THE WARNINGS TO PRINT, handed over once. Entry points call this after
// thread::close_all(), when nothing can raise another.
inline std::vector<Diagnostic> take()
{
    std::lock_guard<std::mutex> held(detail::state().lock);
    std::vector<Diagnostic> out;
    out.swap(detail::state().to_print);
    return out;
}

} // namespace satellite::errors::log
