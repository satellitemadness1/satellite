#pragma once

// What the machine answers about itself for the arguments object -- PLAN M20,
// DESIGN §7.7. The thirty-two facts under `satellite.library.main.arguments`
// that are neither the command line nor read from the object.
//
// THIS FILE KNOWS NOTHING ABOUT SATELLITE, which is `system_facts/`'s whole
// seam and is why these are plain strings and counts rather than `Value`s.
// LAYOUT.md states it for the directory -- "`system_facts/` reports what the
// machine is" -- and `satellite_arguments/` is the module one layer up that
// maps a registry number onto a field below.
//
// BUILT ONCE, ON FIRST ASK, AND THAT IS THE LAZY HALF PLAN M20 ASKED FOR.
// v1 assembled all of this before `satellite.main` ran: `uname`,
// /etc/os-release, `getpwuid`, `gethostname`, two `readlink`s and three
// `sysconf`s, against satl's 2 ms startup. A program that never reads a fact
// now pays for none of them, and one that reads thirty pays once.
// MILESTONES/M20.md carries both numbers, measured rather than assumed --
// PLAN §9's rule, and the reason this is not simply asserted to be faster.
//
// NOT `system_facts`' NO-CACHING RULE, AND THE DIFFERENCE IS THE SUBJECT.
// facts.hpp caches nothing "because the question is what this program is using
// NOW" -- a loop printing `\memused` must see the number move. Every fact here
// is fixed for the life of the process: uname, the distribution file, the
// macros baked into this binary, the pid, the page size, the environment
// satellite has no way to write. THE ONE THAT IS NOT is `getcwd()`, because
// `satellite.directory.change` `1 18 1` has been built since M19 -- so the
// starting directory is captured with the command line instead, in
// satellite_value/value_arguments.hpp, and is not in this file at all.

#include <string>

namespace satellite::facts {

// The thirty-two, laid out as the numbering nests them. TEXT AND COUNTS ARE
// SEPARATE FIELDS rather than everything rendered to a string, because seven
// of these are numbers in the language -- DESIGN §7.7 says `cores` and
// `threads` are "a satellite number" -- and a count stored as text and parsed
// back would be a conversion the language spent M8 not having.
struct MachineAnswers {
    // --- machine `1 14 1 1 1` -----------------------------------------------
    std::string cpu;            // /proc/cpuinfo's model name
    std::string architecture;   // uname.machine -- a DIFFERENT fact from cpu
    std::string byte_order;
    unsigned long long cores = 0;
    unsigned long long threads = 0;
    unsigned long long page_size = 0;
    unsigned long long pointer_bits = 0;

    // --- memory `1 14 1 1 2` ------------------------------------------------
    // MEGABYTES, WHOLE, AND IT IS THE SAME NUMBER AS LIVE CODE 98 BY
    // CONSTRUCTION -- one call to facts::mem_total_mb(), which is what the
    // renderer calls. DESIGN §7.7's "they must not be allowed to disagree" is
    // delivered by there being one reader, not by two readers agreeing.
    unsigned long long memory_total_mb = 0;

    // --- username `1 14 1 1 3` ----------------------------------------------
    std::string username;

    // --- system `1 14 1 1 4` ------------------------------------------------
    std::string name;
    std::string kernel;
    std::string kernel_version;
    std::string distribution;
    std::string distribution_id;
    std::string distribution_version;
    std::string hostname;

    // --- build `1 14 1 1 5` -------------------------------------------------
    // What compiled THIS BINARY, baked in by make_support/020-version.mk, so
    // these stay true after the compiler is upgraded or removed -- which is
    // the whole reason they are worth carrying.
    std::string compiler;
    std::string compiler_version;
    std::string standard;
    std::string flags;
    std::string make;
    std::string standard_library;
    std::string c_library;
    std::string built;

    // --- interpreter `1 14 1 1 6` -------------------------------------------
    // The bare word is the PATH, which is PLAN M20's shape table and the one
    // group word that is a fact. `library_path` and `library_path_source` are
    // M25's and are not here; arguments_rows.cpp says so where it refuses.
    std::string interpreter;
    std::string version;

    // --- process `1 14 1 1 7` -----------------------------------------------
    unsigned long long process_id = 0;
    unsigned long long parent_process_id = 0;

    // --- session `1 14 1 1 8` -----------------------------------------------
    // Four of the five. `directory` is the object's -- see the header note.
    std::string shell;
    std::string terminal;
    std::string language;
    std::string home;
};

// The answers, assembled on the first call and kept. A function-local static,
// so the assembly is thread-safe without a mutex anybody has to remember.
const MachineAnswers &machine_answers();

// THE WORD A MISSING FACT ANSWERS WITH, in one place. A blank value would let
// a program believe the machine had been asked and had said nothing, which is
// DESIGN §1.1's "behind their back" with a fact as the stake.
inline constexpr const char *kUnrecorded = "unrecorded";

} // namespace satellite::facts
