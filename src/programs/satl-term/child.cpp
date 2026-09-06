// The `satl` on the other side of the pty. See child.hpp for the split.

#include "programs/satl-term/child.hpp"

#include <cstdio>
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

} // namespace

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
                             nullptr, nullptr, nullptr,   // no child setup
                             -1,               // default timeout
                             nullptr,          // no cancellable
                             done,
                             for_done);

    // spawn_async copies what it needs; this side owns the array.
    g_strfreev(child_env);
    return true;
}

} // namespace satellite
