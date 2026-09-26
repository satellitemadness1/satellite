// §16's load phase: several spaceships merged into one Program.
//
// Every test here writes real files and loads them, because every interesting
// property of the loader is about the filesystem — where it looks, what it
// canonicalises, and what it declines to read twice. A loader tested against
// an in-memory stub would confirm the merge and none of the rest.
//
// The load-bearing tests are include-once and the cycle. Both are the same one
// line of code and both are the difference between a working program and a
// pile of duplicate-capsule errors, so both are checked from the outside — by
// running the program and looking at what it printed.

// This file holds the harness the sections share and the main() that runs
// them. The sections themselves are in loader_test_<topic>.cpp, and the
// ORDER main() calls them in is part of the test: the quoted path sections
// build a directory tree that the sections after them read back, so a
// reordering here would break tests that look correct where they are
// written.

#include "loader_test.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// One directory per run, so a stale file from a previous run cannot make a
// test pass. Removed at the end; left behind on a failure, which is when
// someone wants to look at it.
std::string dir()
{
    static std::string path;
    if (path.empty()) {
        path = "/tmp/satellite_loader_test_" + std::to_string(getpid());
        mkdir(path.c_str(), 0700);
    }
    return path;
}

std::string write_ship(const std::string &name, const std::string &body)
{
    const std::string path = dir() + "/" + name + ".satl";
    std::ofstream out(path);
    out << body;
    return path;
}

// A capsule that displays one word, so a test can tell which spaceships ran
// and in what order by reading the output.
std::string says(const std::string &name, const std::string &word)
{
    return "satellite.capsule " + name + "()\n"
           "{\n"
           "    satellite.console.display(\"" + word + "\")\n"
           "    satellite.return(satellite)\n"
           "}\n";
}

std::string main_calling(const std::string &body)
{
    return "satellite.capsule satellite.main("
           "satellite.container.list<satellite.variable.string> argz)\n"
           "{\n" + body +
           "    satellite.return(satellite)\n"
           "}\n";
}

bool contains(const std::string &haystack, const std::string &needle)
{
    return haystack.find(needle) != std::string::npos;
}

int main()
{
    // The merge, and the three include graphs that only terminate
    // because a spaceship is marked seen before it is parsed.
    loader_test_the_merge();
    loader_test_include_once_diamond();
    loader_test_cycle_terminates();
    loader_test_self_include();
    loader_test_included_body_runs_first();
    loader_test_include_satellite_is_ceremony();

    // The quoted include path. These write the directory tree that the
    // sections after them read back, so they run before those.
    loader_test_quoted_include_reaches_subdirectory();
    loader_test_quoted_include_without_extension();
    loader_test_quoted_include_climbs_upward();
    loader_test_include_once_across_spellings();
    loader_test_quoted_include_of_empty_string();
    loader_test_quoted_include_that_names_nothing();

    // What the loader refuses, and what it says while refusing.
    loader_test_bare_include_that_names_nothing();
    loader_test_include_of_a_non_name();
    loader_test_language_owned_include();
    loader_test_parse_error_in_included_spaceship();
    loader_test_name_defined_in_two_spaceships();

    // The SourceMap and the search order, examined directly rather than
    // through a program's output.
    loader_test_source_map_one_entry_per_spaceship();
    loader_test_search_order();

    if (failures) {
        printf("%d loader check(s) failed (files left in %s)\n", failures,
               dir().c_str());
        return 1;
    }

    // Only on success: a failure wants its evidence left on disk.
    const std::vector<std::string> written = {
        "helper",  "entry",     "d",         "b",       "c",     "diamond",
        "cyc_a",   "cyc_b",     "selfie",    "first",   "order", "ceremony",
        "nofile",  "badtarget", "window",    "langowned", "broken",
        "usesbroken", "dup_a",  "dup_b",     "leaf",    "mid",   "root"};
    for (const std::string &name : written)
        std::remove((dir() + "/" + name + ".satl").c_str());
    rmdir(dir().c_str());

    printf("PASS: loader (merge across spaceships, include-once through a "
           "diamond, cycles and self-include terminating, included bodies "
           "first, ceremony preserved, errors named against their own "
           "spaceship, search order)\n");
    return 0;
}
