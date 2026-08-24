#pragma once

// What `satl --version` and `satl-term --version` print.
//
// TWO NUMBERS, and they move at different rates. The VERSION (002) is the
// language; it changes rarely and deliberately. The REVISION (01) is this
// build of it and goes up as work lands. 001 was the version this project
// carried while §1 through §19 were being written; 002 is what it became once
// it had blown past that.
//
// The values arrive as -D from the Makefile and are DELIBERATELY NOT in
// CXXFLAGS, for exactly the reason -DSATELLITE_LIB_DIR is not: they sit on the
// two recipes that need them, main.o and window.o. CXXFLAGS is what
// .cxxflags-stamp records, and a build stamp that changes every second would
// put a different string in that stamp on every invocation -- so `make` would
// rebuild all forty-three objects, every time, forever, and the stamp that
// exists to catch a real flag change would never again be quiet.
//
// The defaults below exist so this header compiles for anyone who builds a
// file by hand without the Makefile's defines. They say "unrecorded" rather
// than inventing a plausible number, because a build that cannot say when it
// was built should not answer as though it could.

#include <string>

#ifndef SATELLITE_VERSION
#define SATELLITE_VERSION "002"
#endif
#ifndef SATELLITE_REVISION
#define SATELLITE_REVISION "01"
#endif
#ifndef SATELLITE_BUILT
#define SATELLITE_BUILT "unrecorded"
#endif
#ifndef SATELLITE_BUILD_CXX
#define SATELLITE_BUILD_CXX "unrecorded"
#endif
#ifndef SATELLITE_BUILD_FLAGS
#define SATELLITE_BUILD_FLAGS "unrecorded"
#endif

namespace satellite {

// "002 revision 01" -- the one line that names this build, used by --version
// and by the prompt banner so the two can never disagree about what is running.
inline std::string version_line()
{
    return std::string(SATELLITE_VERSION) + " revision " + SATELLITE_REVISION;
}

// The whole answer. `program` is the command the user actually typed, so satl
// and satl-term each name themselves rather than both claiming to be the
// language.
//
// The COMPILER comes from __VERSION__, which the compiler defines itself, and
// not from the Makefile's $(CXX). That is on purpose and the two are different
// facts: $(CXX) is the path make invoked, and __VERSION__ is what that path
// turned out to be. Both are printed, because a wrong answer here is exactly
// the failure this project already had once -- LLVM_BIN pointed at a directory
// that did not exist, `c++` answered instead, and every measurement attributed
// to clang was GCC's, with nothing anywhere saying so.
inline std::string version_text(const char *program)
{
    return std::string(program) + " " + version_line() + "\n"
           "  built     " SATELLITE_BUILT "\n"
           "  compiler  " __VERSION__ "\n"
           "  invoked   " SATELLITE_BUILD_CXX "\n"
           "  flags     " SATELLITE_BUILD_FLAGS "\n";
}

} // namespace satellite
