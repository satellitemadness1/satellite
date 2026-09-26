#pragma once

#include "satellite_value/value.hpp"

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

// The same facts in BYTES, which is what satellite.system.memory.* answers
// from. Bytes rather than megabytes because every unit the language offers is
// a power of 1024 away from a byte, and a division by a power of two
// TERMINATES -- so every conversion is exact and §8.1's promise that there is
// one answer survives being asked in terabytes. Converting from an already
// rounded megabyte figure would have thrown that away before the language saw
// it.
unsigned long long mem_total_bytes();
unsigned long long mem_used_bytes();
unsigned long long mem_available_bytes();
// This process's own resident set, right now -- what satellite.system.memory.
// main() answers. RESIDENT and not virtual: virtual size counts address space
// the program reserved and never touched, which on a tree walker is most of it,
// and a number that goes up when nothing was allocated is a number nobody can
// act on.
unsigned long long process_memory_bytes();

// This THREAD's stack: what it is using, and how big it is allowed to get.
// False when the platform will not say.
//
// The stack is the only memory a thread has of its own. Linux accounts an
// address space per PROCESS, and every thread shares it -- so a per-thread
// "used memory" that meant heap would be the same number for all of them, and
// /proc/self/task/<tid>/statm reports exactly that process-wide figure. The
// stack is the real per-thread quantity, and it is also the one that decides
// how deep this interpreter may recurse, which is why stack_limit_bytes()
// below already existed for the depth guard.
bool thread_stack_bytes(unsigned long long *used, unsigned long long *total);

unsigned long long swap_total_bytes();
unsigned long long swap_used_bytes();

// From SMBIOS/DMI table type 17, the Memory Device record. BOTH answer 0 for
// "this machine would not say", which is the ordinary case: /sys/firmware/dmi/
// entries is mode 0400 root, and there is no other source on Linux for either
// number -- lshw, dmidecode and inxi all read the same root-only table. A run
// as root gets real figures; a run as anybody else gets 0, which is why 0 is a
// documented answer here and not a failure.
unsigned long mem_frequency_mhz();
unsigned mem_width_bits();

// The stack this process may actually grow, in bytes — RLIMIT_STACK's soft
// limit. It is what bounds how deep the tree walk can recurse, because a
// satellite capsule activation is a real chain of C++ calls (src/evaluator/helpers.cpp
// measures the cost of one), so the depth guard's ceiling is derived from this
// rather than fixed at a number that was only ever right for one stack size.
//
// `ulimit -s` is therefore the knob for how deep a program may recurse, and it
// is deliberately outside the language: asking for a 64 GB stack is a thing to
// do on purpose, from the shell, and not something a program can talk itself
// into partway through a run.
//
// RLIM_INFINITY and a failed getrlimit both answer STACK_LIMIT_UNKNOWN, which
// the caller reads as "assume the ordinary 8 MB". Unlimited is not treated as
// unbounded on purpose: the main thread's stack still stops where the next
// mapping begins, so believing "infinity" would put the guard past the cliff
// again, which is the exact failure it exists to prevent.
constexpr unsigned long long STACK_LIMIT_UNKNOWN = 0;
unsigned long long stack_limit_bytes();

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

// Every named entry of the value satellite.main is handed, built once, at
// startup, from the command line and from this machine.
//
// Takes the command line as satl already assembled it -- a List of string
// Values, since run_entry() has already made them (interp.hpp: argv[0] is the
// script, so a program sees its own name first, as it would in C). Those
// handles are REUSED rather than re-encoded, so building the arguments object
// costs no copy of the command line.
//
// It lives in system_facts because it is the ONE place that touches uname,
// /etc/os-release, getpwuid and sysconf; the evaluator sees named strings and
// nothing else. Every fact is either true or says in words that it is not:
// `unrecorded` for a build fact the Makefile did not pass, `unavailable` for a
// runtime fact the kernel would not answer. An entry is never silently absent,
// because .has() has to be able to tell "there is no such name" from "there is
// no such answer on this machine".
//
// $PATH is deliberately not among them: unbounded, and the most likely of all
// of them to hold something private in an object people will paste into bug
// reports.
Arguments arguments_for(const List &command_line);

// Background guard: once a second, if available memory drops below
// satellite.library.system.min_free_mb (default 4096), the program
// prints a message and shuts down.
void start_memory_watchdog();

} // namespace satellite
