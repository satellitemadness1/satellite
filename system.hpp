#pragma once

#include <string>

namespace satellite {

// Live machine facts, used by satellite_string's system chars and the repl.
std::string username();
std::string home_dir();
std::string cwd();
unsigned hardware_threads();
unsigned long mem_total_mb();
unsigned long mem_used_mb();      // total - available
unsigned long mem_available_mb();

// Which of library_path()'s three candidates answered. --where prints it,
// because "where is the library" and "why there" are different questions, and
// only the second one is any use to someone whose install is broken.
enum class LibraryPathSource {
    None,           // nothing resolved
    Environment,    // $SATELLITE_PATH
    Relative,       // ../share/satellite/lib, found from /proc/self/exe
    Compiled,       // -DSATELLITE_LIB_DIR
};

// Where the satellite-language library (.satl) lives, resolved per run. Three
// candidates, first one that exists as a directory wins:
//
//   1. $SATELLITE_PATH
//   2. dirname(/proc/self/exe)/../share/satellite/lib
//   3. the compiled-in SATELLITE_LIB_DIR, baked from the Makefile's `prefix`
//
// The order is the design, not an accident of how it was written. An install
// that only works once the user exports a variable is a broken install, so
// tiers 2 and 3 must each be sufficient alone -- and the environment variable
// is therefore not a configuration mechanism at all. It is a development
// escape hatch for running a tree that was never installed, and it goes FIRST
// precisely because that is the one job it has: overriding a system install
// without uninstalling one.
//
// Tier 2 is what makes a tarball relocatable. It asks the binary where it
// actually is rather than where it was configured to be, so unpacking the same
// tree under /opt or ~/.local works with no rebuild: bin/ and share/ move
// together because they were only ever one directory apart.
//
// Tier 3 is the answer for a distribution package, where the prefix is settled
// at build time and nothing moves afterwards.
//
// Every candidate is stat'd before it is accepted, so a candidate that names
// nothing cannot shadow a later one that names a real directory. That case is
// not hypothetical: it is every run of the binary straight out of the build
// directory, where ../share does not exist and tier 3 has to get the call.
//
// Returns "" if none of the three resolves; what to say about that belongs to
// the caller, since --where, a --run and the REPL each owe the user something
// different.
std::string library_path(LibraryPathSource *source = nullptr);

// Background guard: once a second, if available memory drops below
// satellite.library.system.min_free_mb (default 4096), the program
// prints a message and shuts down.
void start_memory_watchdog();

} // namespace satellite
