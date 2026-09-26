// Part of the loader test binary (loader_test): satellite.include("a/path"),
// the quoted form that lets a project be a tree of files rather than one flat
// directory. Down into a subdirectory, up through `..`, with and without the
// .satl extension, and the two ways a quoted path can name nothing at all.
//
// These sections build a directory tree that the later ones read, so they must
// keep running in the order loader_test.cpp calls them in.

#include "loader_test.hpp"

#include "interpreter/interp.hpp"

#include <fstream>
#include <string>
#include <sys/stat.h>

using namespace satellite;

// --- a quoted include path -------------------------------------------------
//
// The form the loader gained so that a project can be a TREE of files
// rather than one flat directory. Every case below is a real shape a
// program writes.
void loader_test_quoted_include_reaches_subdirectory()
{
    const std::string sub = dir() + "/object";
    mkdir(sub.c_str(), 0700);
    std::ofstream(sub + "/helper.satl") << says("from_sub", "in a subdir");

    const std::string path =
        write_ship("quoted", "satellite.include(\"object/helper.satl\")\n\n" +
                                 main_calling("    from_sub()\n"));

    InterpResult r = run_file(path, {});
    check(r.ok, "a quoted include reaches a subdirectory");
    check(contains(r.output, "in a subdir"),
          "and the spaceship it names actually runs");
}

// The extension may be left off, exactly as a bare name leaves it off.
void loader_test_quoted_include_without_extension()
{
    const std::string path =
        write_ship("noext", "satellite.include(\"object/helper\")\n\n" +
                                main_calling("    from_sub()\n"));

    InterpResult r = run_file(path, {});
    check(r.ok, "a quoted include may omit the .satl extension");
}

// UPWARD, with `..`. This is the case a bare name has no spelling for at
// all, and two files in view_forge climb three levels.
void loader_test_quoted_include_climbs_upward()
{
    const std::string deep = dir() + "/object/deeper";
    mkdir(deep.c_str(), 0700);
    std::ofstream(deep + "/climber.satl")
        << "satellite.include(\"../helper.satl\")\n" +
           says("climbed", "climbed back up");

    const std::string path =
        write_ship("upward", "satellite.include(\"object/deeper/climber.satl\")\n\n" +
                                 main_calling("    climbed()\n    from_sub()\n"));

    InterpResult r = run_file(path, {});
    check(r.ok, "a quoted include may climb with ..");
    check(contains(r.output, "climbed back up"),
          "and the file it climbed to is merged");
}

// RELATIVE TO THE INCLUDING SPACESHIP, not to the working directory. The
// whole form turns on this: the same program must mean the same thing
// whatever directory it is run from, and `object/deeper/climber.satl`
// resolving its own `../helper.satl` against the cwd would break the moment
// anyone ran the program from one level up. Proven by the test above
// running with a cwd that is NEITHER of the two directories involved.

// Include-once still holds ACROSS SPELLINGS, because the key is the
// canonical path: `object/helper.satl` from the root and `../helper.satl`
// from object/deeper are one spaceship. Were they two, this program would
// report from_sub as already defined.
void loader_test_include_once_across_spellings()
{
    const std::string path = write_ship(
        "onceacross",
        "satellite.include(\"object/helper.satl\")\n"
        "satellite.include(\"object/deeper/climber.satl\")\n\n" +
            main_calling("    from_sub()\n"));

    InterpResult r = run_file(path, {});
    check(r.ok, "two spellings of one file are still loaded once");
}

void loader_test_quoted_include_of_empty_string()
{
    const std::string path =
        write_ship("emptypath", "satellite.include(\"\")\n\n" +
                                    main_calling(""));

    InterpResult r = run_file(path, {});
    check(!r.ok, "an include of the empty string is an error");
    check(contains(r.output, "names no spaceship"),
          "and says so rather than reporting an unnamed missing file");
}

void loader_test_quoted_include_that_names_nothing()
{
    const std::string path =
        write_ship("missingpath",
                   "satellite.include(\"object/nope.satl\")\n\n" +
                       main_calling(""));

    InterpResult r = run_file(path, {});
    check(!r.ok, "a quoted include that names nothing is an error");
    check(contains(r.output, "\"object/nope.satl\""),
          "and quotes the path back as the program wrote it");
    check(contains(r.output, "looked in"), "and says where it looked");
}
