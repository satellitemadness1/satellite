// satl — the interpreter.
//
// This binary links NO GUI: the window lives in satl-term, which spawns
// this one into a PTY. The split is measured, not tidy-minded — linking gtk4
// and vte here pulled 119 shared objects that the dynamic linker loaded before
// main() on every `--run`, costing 23.4 ms against an interpreter whose own
// share of hello world is 0.3 ms. See window.cpp.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "console_output/console.hpp"
#include "system_facts/version.hpp"
#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"
#include "satellite_string/satellite_string.hpp"
#include "system_facts/system.hpp"

// ---------------------------------------------------------------------------
// REPL side (runs inside the terminal)
// ---------------------------------------------------------------------------


#include "programs/main_internal.hpp"

// satl --where — the resolved library directory and which of the three
// candidates in system.cpp produced it.
//
// It exists to make an install falsifiable. `make install` can then be checked
// without running a program, and someone whose install is broken has one
// command to run instead of a guess to make about which tier answered: the
// difference between "found it next to the binary" and "fell through to the
// compiled-in default" is the whole diagnosis, and the path alone does not say
// which happened.
int where()
{
    satellite::LibraryPathSource source = satellite::LibraryPathSource::None;
    std::string path = satellite::library_path(&source);

    const char *from = "";
    switch (source) {
    case satellite::LibraryPathSource::Environment:
        from = "$SATELLITE_PATH (development override)";
        break;
    case satellite::LibraryPathSource::Relative:
        from = "alongside the binary (relocatable install)";
        break;
    case satellite::LibraryPathSource::Compiled:
        from = "compiled-in SATELLITE_LIB_DIR (packaged install)";
        break;
    case satellite::LibraryPathSource::None:
        from = "nothing — no candidate exists";
        break;
    }

    printf("library: %s\nfrom:    %s\n",
           path.empty() ? "(not found)" : path.c_str(), from);
    return 0;
}


void usage()
{
    fprintf(stderr,
            "usage: satl                             repl on stdin/stdout\n"
            "       satl --repl                      the same, spelled out\n"
            "       satl --run <file> [args]         run a file on stdout\n"
            "       satl <file> [args]               same as --run\n"
            "       satl --where                     resolved library path\n"
            "\n"
            "at the prompt: run <file> [args]  (also spelled interpret, --run)\n"
            "the gui terminal is a separate binary: satl-term\n"
            "  satl --version            what this build is, and what built it\n");
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() > 1 && args[1] == "--repl")
        return run_repl();
    if (args.size() > 1 && args[1] == "--where")
        return where();
    // Before --help and before the bare-filename arm, so a file that happens
    // to be called --version cannot shadow the flag. Exit 0: asking a program
    // what it is, is not an error, and a packaging script that greps this is
    // entitled to a zero.
    if (args.size() > 1 && (args[1] == "--version" || args[1] == "-V")) {
        fputs(satellite::version_text("satl").c_str(), stdout);
        return 0;
    }
    if (args.size() > 1 && (args[1] == "-h" || args[1] == "--help")) {
        usage();
        return 2;
    }

    std::string file;
    std::vector<std::string> rest;

    if (args.size() > 1 && args[1] == "--run") {
        if (args.size() < 3) {
            usage();
            return 2;
        }
        file = args[2];
        rest.assign(args.begin() + 3, args.end());
    } else if (args.size() > 1 && !args[1].empty() && args[1][0] != '-') {
        file = args[1];
        rest.assign(args.begin() + 2, args.end());
    } else if (args.size() > 1) {
        fprintf(stderr, "satellite: unknown option %s\n", args[1].c_str());
        usage();
        return 2;
    }

    // No file means the repl. It used to mean the window, which is now a
    // separate binary that this one knows nothing about.
    if (file.empty())
        return run_repl();

    return run_file_mode(file, rest);
}
