// satl -- handing over to the window when there is no console. See the header
// for what this is for; this file is the six reasons not to do it.

#include "programs/window_handover.hpp"

#include <string>
#include <vector>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite {
namespace {

// Whether this process has a controlling terminal.
//
// THIS IS THE WHOLE DESIGN AND isatty() IS THE TRAP. The obvious test is
// isatty(STDOUT_FILENO) -- "am I writing to a terminal" -- and it is wrong in
// the direction that breaks working commands: `satl --words | grep console`
// has a pipe on stdout, answers no, and would open a window instead of feeding
// the pipe. Every shell pipeline in every script would do the same.
//
// The question that actually distinguishes a launcher from a terminal is
// whether the process has a CONTROLLING terminal, and /dev/tty is the file that
// answers it: it is the controlling terminal of whoever opens it, and opening
// it fails with ENXIO when there is none. A pipeline started from a shell still
// has one, because it inherited the shell's. A program started by a file
// manager, a .desktop entry or a desktop menu has none at all.
//
// O_NOCTTY, so that asking the question cannot answer it: without it a process
// with no controlling terminal that opens a terminal ACQUIRES it as its own on
// some systems, and a test that changes what it tests is worse than no test.
bool has_a_controlling_terminal()
{
    const int fd = open("/dev/tty", O_RDONLY | O_NOCTTY);
    if (fd < 0)
        return false;
    close(fd);
    return true;
}

// Whether something is plainly capturing our output.
//
// The controlling-terminal test above is necessary and not sufficient. A
// program run by a build system, a cron job or a systemd unit also has no
// controlling terminal, and popping a window at one of those would be exactly
// the behind-their-back failure this file exists to avoid. So: if stdout is a
// PIPE or a REGULAR FILE, somebody arranged to read what we print, and we print
// it. `satl --version > version.txt` from a desktop launcher still writes the
// file.
//
// A character device (/dev/null, and a tty, though a tty cannot reach here) and
// a socket (the session journal, which is where a desktop launch usually lands)
// are the two cases where nothing is meaningfully reading, and those are the
// ones that get a window.
bool something_is_reading_our_output()
{
    struct stat st;
    if (fstat(STDOUT_FILENO, &st) != 0)
        return false;
    return S_ISFIFO(st.st_mode) || S_ISREG(st.st_mode);
}

// Is there a display to put a window on?
//
// Asked of the environment rather than by trying to connect, because trying
// costs a round trip to a display server on every single run of satl, and this
// function is on the startup path that PLAN sec 4.3 measures at 0.01 ms of our
// own. Two variables and not one: Wayland sessions set WAYLAND_DISPLAY, X11
// sets DISPLAY, and an Xwayland session sets both.
bool a_display_exists()
{
    const char *const wayland = getenv("WAYLAND_DISPLAY");
    if (wayland != nullptr && wayland[0] != '\0')
        return true;
    const char *const x11 = getenv("DISPLAY");
    return x11 != nullptr && x11[0] != '\0';
}

// The satl-term sitting NEXT TO this binary.
//
// The same rule terminal.cpp uses in the other direction, and for the same
// reason: /proc/self/exe rather than argv[0], because argv[0] is whatever the
// caller chose to say, and the binary beside us rather than whatever PATH
// resolves, because a satl-term from another install would spawn ITS satl and
// the user would end up in a different interpreter than the one they ran.
//
// Empty when it cannot be worked out, which the caller treats as a refusal.
std::string satl_term_beside_us()
{
    char self[4096];
    const ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n <= 0 || static_cast<size_t>(n) >= sizeof self - 1)
        return std::string();
    self[n] = '\0';

    const std::string path(self);
    const std::string::size_type slash = path.rfind('/');
    const std::string directory =
        (slash == std::string::npos) ? std::string() : path.substr(0, slash + 1);
    return directory + "satl-term";
}

} // namespace

void hand_over_to_the_window(char **argv)
{
    // ONE: the user said not to. An environment variable and not only a flag,
    // because the thing that starts satl without a console is often a launcher
    // whose command line somebody else wrote -- a .desktop entry, a file
    // association -- and a variable is the only lever available there. Any
    // non-empty value counts; there is nothing to be gained by parsing it.
    const char *const off = getenv("SATL_NO_WINDOW");
    if (off != nullptr && off[0] != '\0')
        return;

    // TWO: there is a console, so satl is where the person is looking. This is
    // every run from a terminal, pipeline or not.
    if (has_a_controlling_terminal())
        return;

    // THREE: no console, but our output is going somewhere on purpose.
    if (something_is_reading_our_output())
        return;

    // FOUR: no display, so there is no window to hand over to. An ssh session
    // with no tty, a container, a cron job. satl runs and prints into whatever
    // it was given, which is all it could ever have done here.
    if (!a_display_exists())
        return;

    // FIVE: no satl-term beside us. It is a conditional binary -- 047-window.mk
    // drops it when gtk4 and vte are not installed -- so this is an ordinary
    // outcome on a headless build and not a broken install.
    const std::string window = satl_term_beside_us();
    if (window.empty() || access(window.c_str(), X_OK) != 0)
        return;

    // --hold, and it is the difference between a window and a flash of one.
    // satl-term closes on a clean exit and holds on a failure (PLAN M11.A), and
    // everything satl does today -- the opening text, --version, --words --
    // exits cleanly in milliseconds. Without --hold the person who clicked the
    // icon sees a window appear and vanish, which is a worse answer than the
    // silence this whole file exists to fix.
    std::vector<char *> spawn;
    std::string program = window;
    std::string hold = "--hold";
    spawn.push_back(&program[0]);
    spawn.push_back(&hold[0]);
    for (char **arg = argv + 1; *arg != nullptr; ++arg)
        spawn.push_back(*arg);
    spawn.push_back(nullptr);

    // SIX: exec failed, so carry on here. Nothing is printed about it: this
    // process has already established that it has nowhere to print, which is
    // the only reason it was trying to leave.
    execv(window.c_str(), spawn.data());
}

} // namespace satellite
