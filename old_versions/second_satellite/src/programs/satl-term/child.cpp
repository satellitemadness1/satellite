// The `satl` on the other side of the pty. See child.hpp for the split.

#include "programs/satl-term/child.hpp"

#include <cstdio>
#include <sys/resource.h>
#include <unistd.h>

namespace satellite {
namespace {

// The `satl` sitting NEXT TO this binary -- not this binary again, and not
// whatever PATH happens to resolve.
//
// /proc/self/exe rather than argv[0], because argv[0] is whatever the caller
// chose to put there and a desktop launcher's is not a path at all. An empty
// return means the question could not be answered, which the caller reports
// rather than papering over with a guess.
std::string interpreter_beside_me()
{
    char self[4096];
    const ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n < 0)
        return std::string();
    self[n] = '\0';

    const std::string path(self);
    const size_t slash = path.rfind('/');
    return (slash == std::string::npos ? std::string()
                                       : path.substr(0, slash + 1)) + "satl";
}

// HOW NICE THE INTERPRETER RUNS, and 19 -- the lowest priority there is -- is
// the default rather than an option nobody sets.
//
// THE REASON IS THAT NOTHING ELSE CAN REACH IT. A person who nices `satl` in a
// shell alias has not niced the one THIS program spawns: a launcher runs
// `Exec=satl-term %f` through the desktop shell, and a shell alias is expanded
// by bash on a line a person types, so neither ever sees the child. Measured
// 2026-09-12 on a 24-thread, 12-core machine: an interpreter running 23 compute
// threads at priority 0 took every core and the desktop stopped answering, and
// it looked exactly like running out of memory when it was not.
//
// IT COSTS AN IDLE MACHINE NOTHING. Niceness only decides who yields when two
// things want the same core; with nothing else asking, a niced process still
// gets all of them. What it buys is that the window this interpreter is
// printing into keeps repainting while it works.
//
// SET FROM --nice, AND THE WHOLE RANGE IS ALLOWED including negatives, which
// the kernel will refuse without privilege -- refuse, and leave the child at
// what it inherited, which is the same as not having asked.
int spawn_niceness = 19;

// RUNS IN THE CHILD, between fork and exec -- GSpawn's own hook for exactly
// this. setpriority() and not nice(): nice() adds to whatever the process
// already has, so a window that was itself niced would compound it, and this
// wants to arrive at a value rather than move by one.
//
// A FAILURE IS NOT REPORTED AND CANNOT BE. There is no safe way to say
// anything from between a fork and an exec -- stdio in a forked child of a
// threaded process is not async-signal-safe -- and the consequence of the
// refusal is a child at the priority it inherited, which is what would have
// happened anyway. setpriority itself is a syscall and is safe to call here.
void lower_the_child(gpointer)
{
    setpriority(PRIO_PROCESS, 0, spawn_niceness);
}

} // namespace

void child_set_nice(int niceness)
{
    spawn_niceness = niceness;
}

bool child_spawn(VteTerminal *terminal,
                 const std::string &file,
                 const std::vector<std::string> &args,
                 VteTerminalSpawnAsyncCallback done,
                 gpointer for_done)
{
    const std::string interpreter = interpreter_beside_me();
    if (interpreter.empty()) {
        fprintf(stderr, "satl-term: cannot find myself on disk, so I cannot "
                        "find satl beside me\n");
        return false;
    }

    std::vector<std::string> child{ interpreter };
    if (file.empty()) {
        child.push_back("--repl");
    } else {
        child.push_back("--run");
        child.push_back(file);
        child.insert(child.end(), args.begin(), args.end());
    }

    std::vector<char *> child_argv;
    child_argv.reserve(child.size() + 1);
    for (std::string &word : child)
        child_argv.push_back(word.data());
    child_argv.push_back(nullptr);

    // The child is told it is in THIS window, and that is the whole content of
    // the message: the interpreter's prompt picks colors against a background,
    // and this process is the only one that knows what the background is --
    // terminal.cpp's apply_colors painted it. Without this, satl has to guess,
    // and a guess of white-on-light-blue is an invisible prompt.
    //
    // The environment is COPIED rather than replaced, so the child still
    // inherits everything else it had -- PATH, HOME, TERM and the rest.
    gchar **child_env = g_environ_setenv(g_get_environ(), "SATL_TERM", "1", TRUE);

    vte_terminal_spawn_async(terminal,
                             VTE_PTY_DEFAULT,
                             nullptr,          // inherit the working directory
                             child_argv.data(),
                             child_env,
                             G_SPAWN_DEFAULT,
                             lower_the_child, nullptr, nullptr,
                             -1,               // default timeout
                             nullptr,          // no cancellable
                             done,
                             for_done);

    // spawn_async copies what it needs; this side owns the array.
    g_strfreev(child_env);
    return true;
}

} // namespace satellite
