#pragma once

// Private to src/system_facts/. Nothing outside this directory may include it —
// the public surface is src/system_facts/system.hpp, and that has not changed.
//
// system.cpp was 653 lines. Split along the families system.hpp already lists:
//
//   host_facts.cpp       the user and the machine -- username, home, cwd,
//                        hardware threads
//   memory_facts.cpp     /proc/meminfo, this process's resident set, swap, the
//                        DMI memory device, and the watchdog that reads them
//   stack_facts.cpp      this thread's stack, and the ceiling it may grow to
//   arguments_facts.cpp  the one-fact-each readers arguments_for() is built from
//   system.cpp           library_path() and arguments_for() themselves
//
// Those last two stay in system.cpp because system.o is the ONE object the
// Makefile compiles with -DSATELLITE_LIB_DIR and VERSION_DEFS (the
// $(SYSTEM)/system.o recipe). A sibling file is built by the generic rule and
// gets neither, so it would silently answer from the #ifndef fallbacks at the
// top of system.cpp: `unrecorded` for a build fact that WAS recorded, and
// /usr/local for a prefix nobody chose. Whatever reads those macros therefore
// cannot move, and only what does not read them did.
//
// The helpers below lived in an anonymous namespace when this was one
// translation unit. They are declared here and defined once, in
// arguments_facts.cpp, because internal linkage cannot cross a file boundary.
// That is the only semantic change the split makes; every function body moved
// verbatim.
//
// The include block is the one system.cpp had, carried whole rather than
// pruned per file, for the same reason: it is what these sources were compiled
// against, so nothing here depends on a judgement about what each piece still
// needs.

#include "system_facts/system.hpp"
#include "satellite_library/library.hpp"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include <dirent.h>
#include <sys/utsname.h>

#include <pthread.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace satellite {

// --- arguments_facts.cpp ---------------------------------------------------
//
// The reason each of these is written the way it is stays with its definition.

// The two words that stand in for a fact this machine will not give up. They
// are distinct on purpose: `unrecorded` means the build never passed it,
// `unavailable` means the running system declined to answer. Debugging a
// report that says one is a different job from debugging one that says the
// other.
constexpr const char *UNRECORDED  = "unrecorded";
constexpr const char *UNAVAILABLE = "unavailable";

// $NAME, or "" if unset or empty.
std::string env_or_empty(const char *name);

// One VAR=value out of /etc/os-release, with the surrounding quotes taken off.
std::string os_release(const char *key);

// What compiled the interpreter: the standard, the standard library, the C
// library. All three answer from a macro the compiler defines, so all three
// describe THIS binary rather than whatever is installed now.
const char *cxx_standard_name();
std::string standard_library_name();
std::string c_library_name();

// Append one entry, keeping `index` in step. The default argument lives here
// and not on the definition, which is where a declaration in a header puts it.
void add(Arguments &args, std::string name, std::string value,
         const char *missing = UNAVAILABLE);

} // namespace satellite
