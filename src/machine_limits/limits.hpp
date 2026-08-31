#pragma once

// The machine limits, and the file that holds them -- PLAN M6, against PLAN
// §4.5. The one door over this module.
//
// WHAT THIS IS. satl reads a `satellite_config.ini` and holds itself to what it
// says: how many threads it may use, how many cores the machine has, and how
// much memory it may occupy before it stops itself. Nothing in the language
// exists yet to read any of it -- running a program lands at M10 -- so this
// milestone builds the READERS and the POLICY over them, and PLAN M6 draws that
// seam in one sentence: "the readers are here, and satellite.system's
// twenty-eight paths are M20."
//
// TWO KINDS OF SETTING, AND THE SPELLING IS WHAT TELLS THEM APART.
//
//     THREAD_COUNT   CORE_COUNT   MEMORY_MAX      machine settings, SHOUTED
//     division_digits  max_depth  min_free_mb  float_digits    dials, lower case
//
// The upper-case three are §4.5's own spelling and are **not in the language**:
// no `satellite.` path names them, nothing numbers them, and a program cannot
// read them. The lower-case four are `satellite.library.system.*` --
// `1 14 2 1` through `1 14 2 4` -- which a running program will read and retune
// from M8 onward. The case difference is not decoration: it is the difference
// between a setting the machine has and a setting the language has, and reading
// one line of the file tells you which you are looking at.
//
// §4.5.3's QUESTION IS ANSWERED YES, AND THIS IS WHERE. It asked whether the
// file seeds the namespace, and said "not yet decided". It does: the file is
// read once at startup, its values land in the dials below, and from that moment
// the DIALS are the single authority at run time -- the file is never read
// again and nothing consults it. That is the arrangement §4.5.3 called obvious,
// and what it was missing is the half above: the file may set a dial the
// language owns, and it may set three settings the language does not, and only
// the first kind is in the namespace at all.
//
// ONLY min_free_mb HAS A MEANING AT M6, WHICH IS PLAN M6'S OWN RULE: "the node
// and the storage land here; min_free_mb is the only one whose meaning is this
// milestone's." So all four are stored, all four can be set from the file, all
// four are printed by `satl --limits` -- and three of them are printed with the
// milestone that will read them, because a milestone that quietly gave
// `division_digits` a default would be taking M8's decision about what a
// division does. They arrive here as storage and leave here as storage.

#include "error_reporter/report.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::limits {

// Where a value came from. `satl --limits` prints this beside every row,
// because "what is satl holding to" and "who decided that" are different
// questions and only the second one is any use to somebody whose machine is
// behaving unexpectedly. It is the same argument v1's LibraryPathSource makes
// about --where.
enum class Origin {
    Default,   // nobody said, so the OS or a built-in answered
    File,      // satellite_config.ini said so
    Clamped,   // the file asked for more than the machine has, and the machine won
};

std::string_view origin_text(Origin origin);

// One machine setting: what it is, who said so, and which line of the file.
//
// THE LINE IS CARRIED EVEN THOUGH NOTHING RENDERS A CARET FROM IT, and it is
// four bytes for the thing a person actually asks next -- "where do I change
// it?" `satl --limits` prints it as `satellite_config.ini:7`, which is a place
// an editor can jump to.
struct Setting {
    unsigned long long value = 0;
    Origin origin = Origin::Default;
    unsigned line = 0;
};

// The four dials of `satellite.library.system`, in words.def order, so
// `kDialNodes[static_cast<size_t>(id)]` is the node and the two lists cannot
// drift. WORD_NUMBERS §2.2 numbers them `1 14 2 1` .. `1 14 2 4`.
enum class DialId : size_t {
    DivisionDigits = 0,
    MaxDepth,
    MinFreeMb,
    FloatDigits,
    Count_
};

inline constexpr size_t kDialCount = static_cast<size_t>(DialId::Count_);

inline constexpr words::NodeId kDialNodes[kDialCount] = {
    words::NodeId::LIBRARY_SYSTEM_DIVISION_DIGITS,
    words::NodeId::LIBRARY_SYSTEM_MAX_DEPTH,
    words::NodeId::LIBRARY_SYSTEM_MIN_FREE_MB,
    words::NodeId::LIBRARY_SYSTEM_FLOAT_DIGITS,
};

// One dial. `set` is false when nobody has said anything about it.
//
// UNSET IS A REAL STATE AND NOT ZERO, because three of the four have no reader
// at M6 and inventing a default for them would be inventing their meaning. An
// unset dial prints as `unset` and the milestone that will read it decides what
// no answer means -- v1's read_division_digits() is exactly that decision, made
// where the division is.
struct Dial {
    unsigned long long value = 0;
    bool set = false;
    Origin origin = Origin::Default;
    unsigned line = 0;
};

// Everything satl is holding itself to, and where each piece came from.
struct Held {
    // The file that was read, as it would be typed. Empty when there was none,
    // which is the ordinary case and is not a failure -- every value then comes
    // from the machine.
    std::string config_path;

    Setting thread_count;   // threads satl may use
    Setting core_count;     // physical cores this run can see
    Setting memory_max;     // BYTES satl may occupy before it stops itself

    Dial dials[kDialCount];

    const Dial &dial(DialId id) const
    {
        return dials[static_cast<size_t>(id)];
    }
};

// --- the file ---------------------------------------------------------------

// Read a `satellite_config.ini`'s text. False when it is malformed, with one
// diagnostic per problem -- codes, spans and carets, through M5's reporter.
//
// TEXT AND NOT A PATH, so a test can hand it a string and so the caller decides
// what to say when a file will not open. That is exactly the seam
// programs/source_file.hpp draws and the same reason: getting the bytes and
// deciding what a failure is worth are different jobs.
//
// IT WRITES INTO A Held THAT ALREADY HOLDS THE MACHINE'S ANSWERS, and does not
// clear it. So the order is: fill in the OS fallbacks, then let the file
// overwrite what it mentions -- which is what makes a file that sets one key a
// file that changes one thing.
bool read_config(std::string_view text, Held &into,
                 std::vector<errors::Diagnostic> &problems);

// Every key the file may hold, in the order `satl --limits` prints them. Used
// by the reader for "did you mean", and by the test.
std::vector<std::string_view> config_keys();

// Where satl looks for the file, or "" when there is none.
//
// BESIDE THE BINARY, AND NOT IN THE WORKING DIRECTORY. PLAN §5 fixes the
// install root at `$HOME/.satl` and puts the binary directly in it, so
// `dirname(/proc/self/exe)` IS the install and a file there is that install's
// settings. The working directory is deliberately not searched: a config picked
// up from `.` would mean that running satl in one directory and another gives a
// program different memory, silently, which is DESIGN §1.1's "never anything
// behind their back" exactly. An environment variable is not read either, for
// the same reason and one more -- `satl --limits <file>` is the way to point
// satl at a different file, and it is visible on the command line that used it.
std::string found_config_path();

// --- what satl is holding to ------------------------------------------------

// Read the limits, start the pool, and start the watchdog. Called once, early
// in main(), before any argument is acted on.
//
// `named` IS THE OPERAND OF `--limits` OR `--watchdog`, or empty for the file
// satl finds. It REPLACES the found file rather than adding to it, which is what
// makes `satl --limits good.ini` work on a machine whose installed config is
// malformed -- and that case is not hypothetical, because a malformed installed
// config stops satl doing anything at all.
//
// Returns EXIT_FINE, or reports the file through the reporter on stderr and
// returns EXIT_MALFORMED. It does not exit: opening.hpp exists so that two arms
// cannot disagree about what a failure is worth, and a library that calls exit()
// is an arm nobody can see.
int begin(const std::string &named);

// What satl is holding itself to. Valid after begin(); before it, the machine's
// answers with nothing from any file.
const Held &held();

// A byte count as a person would write it -- `61.9 GiB`, `4.0 MiB`, `512 B`.
//
// BINARY UNITS AND ONE DECIMAL PLACE, and the exact byte count is printed BESIDE
// it everywhere this is used rather than instead of it. §4.5.4's whole question
// was that 61.9 GiB and 64.9 GB are the same memory, so a report that gave only
// the rounded form would have reintroduced the ambiguity the config file was
// made to remove. This is the readable half of a pair and never the whole
// answer.
std::string human_bytes(unsigned long long bytes);

} // namespace satellite::limits
