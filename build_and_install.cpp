// build_and_install -- one command that takes a working tree to an installed
// satellite: uninstall what is there, clean, build, install.
//
// It exists because the four steps have to happen in that order and because
// skipping the uninstall is how a stale binary survives a reinstall and gets
// blamed for a bug that was fixed. Doing it by hand is four commands and one of
// them is easy to forget.
//
//     c++ -std=c++20 -Wall -Wextra -O2
//         -o enterprise_satellite_builder build_and_install.cpp
//
// EVERY STEP IS CHECKED, and that is the whole of what this adds over typing
// the four commands. std::system hands back a wait status, not an exit code,
// and a program that ignores it will cheerfully install a tree whose `make`
// failed -- which is worse than not building at all, because the binary that
// ends up on PATH is the previous one and nothing says so.

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <sys/wait.h>

namespace {

struct Step {
    const char *command;
    const char *what;
    bool        may_fail;   // an uninstall with nothing installed is not an error
};

const std::vector<Step> kSteps = {
    { "./install.sh --uninstall", "remove any previously installed satellite", true  },
    { "make clean",               "discard every object and binary",            false },
    { "make",                     "build satl and satl-term",                   false },
    { "./install.sh",             "install them, and the icons and mime type",  false },
};

void usage()
{
    std::cout <<
"build_and_install -- build satellite from a clean tree and install it\n"
"\n"
"usage:  ./enterprise_satellite_builder [--help] [--dry-run]\n"
"\n"
"  --help, -h     this text\n"
"  --dry-run, -n  print the steps and run none of them\n"
"\n"
"It runs these, in order, from the directory it is invoked in:\n";
    for (const Step &step : kSteps)
        std::cout << "  " << step.command << "\n        " << step.what
                  << (step.may_fail ? "  (failure here is not fatal)" : "") << "\n";
    std::cout <<
"\n"
"It STOPS at the first step that fails, except the uninstall -- there is\n"
"nothing to remove on a first install, and that is not an error. A build that\n"
"failed must never reach the install: the binary left on PATH would be the\n"
"previous one, and nothing would say so.\n"
"\n"
"Run it from the top of the satellite source tree. It needs no root: both\n"
"install.sh and the Makefile install to $HOME/.local for an ordinary user and\n"
"to /usr/local for root, so `sudo` decides where it lands and neither needs\n"
"to be told.\n";
}

} // namespace

int main(int argc, char *argv[])
{
    bool dry_run = false;

    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            usage();
            return 0;
        }
        if (arg == "--dry-run" || arg == "-n") {
            dry_run = true;
            continue;
        }
        std::cerr << "build_and_install: unknown option " << arg << "\n"
                  << "try --help\n";
        return 2;
    }

    for (size_t i = 0; i < kSteps.size(); i++) {
        const Step &step = kSteps[i];

        std::cout << "[" << (i + 1) << "/" << kSteps.size() << "] "
                  << step.command << "\n";
        std::cout.flush();

        if (dry_run)
            continue;

        const int status = std::system(step.command);

        // Three distinct failures wearing one return value, and they need
        // different messages: the shell never started, the command was killed
        // by a signal, or it ran and exited nonzero.
        if (status == -1) {
            std::cerr << "build_and_install: could not run a shell\n";
            return 1;
        }
        if (WIFSIGNALED(status)) {
            std::cerr << "build_and_install: `" << step.command
                      << "` was killed by signal " << WTERMSIG(status) << "\n";
            return 1;
        }

        const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (code != 0) {
            if (step.may_fail) {
                std::cout << "      (exit " << code
                          << ", carrying on -- nothing was installed yet)\n";
                continue;
            }
            std::cerr << "build_and_install: `" << step.command
                      << "` exited " << code << "\n"
                      << "stopping here rather than installing a tree that did "
                         "not build\n";
            return code;
        }
    }

    std::cout << (dry_run ? "\ndry run: nothing was done\n"
                          : "\nbuild and install completed\n");
    return 0;
}
