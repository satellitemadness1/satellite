#pragma once

// What `satl --version` prints, and what the opening information names itself
// with.
//
// TWO NUMBERS, and they move at different rates. The VERSION (003) is the
// LANGUAGE; it changes rarely and deliberately. The REVISION (03) is this build
// of it and goes up as work lands. 001 was the version the first satellite
// carried while its design was being written and 002 is what it became; 003 is
// the second satellite, and the number moved because the language is being
// rebuilt rather than because this build is newer than that one.
//
// The values arrive as -D from make_support/020-version.mk and are DELIBERATELY
// NOT in CXXFLAGS. CXXFLAGS is what .cxxflags-stamp records, and a build stamp
// that changes every second would put a different string in that stamp on every
// invocation -- so `make` would recompile the whole tree, every time, forever,
// and the stamp that exists to catch a real flag change would never again be
// quiet. They sit on the two recipes that need them instead.
//
// The defaults below exist so this header compiles for anyone who builds a file
// by hand without the Makefile's defines. They say "unrecorded" rather than
// inventing a plausible number, because a build that cannot say when it was
// built should not answer as though it could.

#include <string>

#ifndef SATELLITE_VERSION
#define SATELLITE_VERSION "003"
#endif
#ifndef SATELLITE_REVISION
#define SATELLITE_REVISION "03"
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

// "003 revision 03" -- the one line that names this build.
//
// ONE FUNCTION, read by --version and by the opening information both, so the
// two cannot disagree about what is running. The first satellite had them as
// separate literals and they drifted: the prompt still said 0.1 long after the
// language said 002, and nothing in the build had any way to notice.
inline std::string version_line()
{
    return std::string(SATELLITE_VERSION) + " revision " + SATELLITE_REVISION;
}

// The whole answer. `program` is the command the user actually typed, so satl
// and satl-term each name themselves rather than both claiming to be the
// language.
//
// THE COMPILER COMES FROM __VERSION__, which the compiler defines itself, and
// not from the Makefile's $(CXX). The two are different facts and both are
// printed: $(CXX) is the path make invoked, and __VERSION__ is what that path
// turned out to be. This project has already been bitten once by the
// difference -- LLVM_BIN pointed at a directory that did not exist, `c++`
// answered instead, and every measurement attributed to clang was GCC's with
// nothing anywhere saying so.
inline std::string version_text(const char *program)
{
    return std::string(program) + " " + version_line() + "\n"
           "  built     " SATELLITE_BUILT "\n"
           "  compiler  " __VERSION__ "\n"
           "  invoked   " SATELLITE_BUILD_CXX "\n"
           "  flags     " SATELLITE_BUILD_FLAGS "\n";
}

} // namespace satellite
