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
// A MACHINE SETTING TAKES A NUMBER OR THE MACHINE'S OWN ANSWER, AND THE FILE
// SAYS WHICH. `CORE_COUNT=12` holds satl to twelve; `CORE_COUNT=arguments.machine.cores`
// says "whatever this machine has", in the spelling a satellite program uses for
// the same fact. DESIGN §7.7 is where that pairing comes from and it is exact:
// "arguments.machine.threads, arguments.machine.cores and arguments.memory.total
// are the same numbers as THREAD_COUNT, CORE_COUNT and MEMORY_MAX in the
// configuration ... Three ways to ask, one place that knows." So the three
// shouted settings are the three facts, one each, and `Fact` below is that table
// in an enum.
//
// THE DIALS DO NOT TAKE ONE, AND THE LINE IS THE SAME LINE AS ABOVE. A machine
// setting is about the machine and has a fact behind it; a dial is the
// language's and has none -- there is no fact called `division_digits` for a
// file to name. Adding one would be inventing M8's meaning, which is the thing
// PLAN M6 forbids in as many words.
//
// AND NOTHING IS READ FROM THE MACHINE UNTIL SOMEBODY ASKS FOR IT, which is why
// `value()` is a function and not a member. Until 2026-08-31 this module filled
// every setting in from the machine at startup and let the file overwrite what
// it named -- "machine first, then file" -- and the cost of that order was
// 0.42 ms on EVERY run of satl, because facts::physical_cores() reads two sysfs
// files per CPU and the only thing that wants the answer is one row of
// `satl --limits`. The order was there because the file had no way to say "the
// machine"; now it has, and there is no order left to get wrong. See limits.cpp.
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
    Default,   // nobody said, so the machine answers
    File,      // satellite_config.ini wrote a number
    Machine,   // satellite_config.ini named the machine's own answer
    Clamped,   // the file asked for more than the machine has, and the machine won
};

// The machine's own answer behind each of the three shouted settings -- DESIGN
// §7.7's "three surfaces, one set of facts", as three cases.
//
// NOT A POINTER TO A READER FUNCTION, which was the first shape and is worse
// for one reason: this enum is stored in a Setting, and a Setting is compared,
// copied and printed. An enum does all three and a function pointer does none
// of them usefully -- and the switch that turns one into a call lives in
// limits.cpp beside the argument for why it is called late.
enum class Fact {
    Threads,       // arguments.machine.threads  1 14 1 1 1 3
    Cores,         // arguments.machine.cores    1 14 1 1 1 1
    MemoryTotal,   // arguments.memory.total     1 14 1 1 2 1
};

std::string_view origin_text(Origin origin);

// One machine setting: what it is, who said so, and which line of the file.
//
// THE LINE IS CARRIED EVEN THOUGH NOTHING RENDERS A CARET FROM IT, and it is
// four bytes for the thing a person actually asks next -- "where do I change
// it?" `satl --limits` prints it as `satellite_config.ini:7`, which is a place
// an editor can jump to.
struct Setting {
    // WHICH MACHINE FACT IS BEHIND THIS SETTING, AND IT NEVER CHANGES. It is
    // not read from the file: the file chooses whether the fact answers, not
    // which fact it is. `Held` below sets all three once.
    Fact fact = Fact::Threads;

    // The number the file wrote, when it wrote one. Meaningless -- and
    // deliberately not cleared -- when it did not: on a Clamped row it is still
    // the over-large figure the file asked for, which is the only place that
    // number survives at all.
    unsigned long long written = 0;

    Origin origin = Origin::Default;
    unsigned line = 0;

    // WHAT SATL IS ACTUALLY HOLDING TO, resolved when it is asked for and not
    // when the file is read. Three of the four origins answer from the machine
    // and one answers from the file, which is the whole of the rule --
    // limits.cpp has it, and the header there has what reading it late is
    // worth.
    unsigned long long value() const;

    // The same rule, for a caller that has ALREADY asked the machine.
    //
    // TWO ENTRY POINTS AND ONE RULE, which is the point of the pair rather than
    // a convenience. `satl --limits` prints the machine's own answers beside the
    // settings, so it reads every fact once and then needs to decide three rows
    // against what it read -- and the version of that code which re-derived
    // "file or machine" for itself was a second copy of this line, free to stop
    // agreeing with it. dump.cpp calls this; everything else calls value().
    unsigned long long value_given(unsigned long long machine_says) const
    {
        return origin == Origin::File ? written : machine_says;
    }
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
//
// AND min_free_mb IS UNSET TOO, WHICH IS A CORRECTION TO v1 RATHER THAN A PORT
// OF IT. v1 defaulted it to 4096 MB and compared the MACHINE's available memory
// against it once a second -- so on any machine with less than 4 GB free, satl
// kills itself one second after starting, every time, having done nothing
// wrong. That is not a conservative default; it is a machine-sized assumption
// written as a number, and this tree has 61.9 GiB so it would never have been
// noticed here. Unset means the machine's free memory is not watched, MEMORY_MAX
// carries the promise on its own, and somebody who wants v1's check writes one
// line.
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

    // THE FACT IS SET HERE AND NOWHERE ELSE, so a default-constructed Held is
    // already a complete answer: every row reads from the machine, which is
    // exactly what satl holds to when there is no file -- and no file is the
    // ordinary case. Nothing has to be filled in before the file is opened.
    Setting thread_count{Fact::Threads};       // threads satl may use
    Setting core_count{Fact::Cores};           // physical cores this run can see
    Setting memory_max{Fact::MemoryTotal};     // BYTES satl may occupy

    Dial dials[kDialCount];

    // THE STACK, BEFORE AND AFTER satl ASKED FOR MORE. Both are kept because
    // `satl --limits` prints every value satl is holding to AND where it came
    // from, and "8.0 GiB" alone would hide the whole fact -- which is that the
    // 8 MiB everybody assumes is a kernel wall is a shell's default with an
    // unlimited hard limit behind it. kStackLimitUnknown in either means the
    // machine would not say.
    unsigned long long stack_before = 0;
    unsigned long long stack_now = 0;

    const Dial &dial(DialId id) const
    {
        return dials[static_cast<size_t>(id)];
    }
};

// WHAT satl ASKS THE KERNEL FOR, IN BYTES. 8 GiB.
//
// A NUMBER AND NOT A CEILING, AND THE DIFFERENCE IS THE WHOLE ARGUMENT.
// DESIGN §7.5's rule is that the language has no depth limit; this does not
// deliver that and is not pretending to -- it is the same C++ stack with a
// bigger default, and the thing that delivers the rule is a walker keeping its
// own stack on the heap. What it buys is that every depth a person could
// plausibly reach stops being reachable: measured 2026-08-31, a recursion that
// died before 100,000 frames at the 8 MiB default ran past 2,600,000 at this
// setting, which is 1,300x the depth the first satellite refused at.
//
// AND IT COSTS NOTHING UNTIL IT IS USED. A stack is lazily committed, so this
// is address space and not memory: reserving 8 GiB moved VmSize by 0.0 MiB.
// 040-sources.mk carries what it did to startup.
//
// 8 GiB RATHER THAN "unlimited" ON PURPOSE. RLIM_INFINITY makes the main
// thread's stack grow until it collides with the next mapping, which is a wall
// in a place nobody chose and reports itself as a segfault; a number is a number
// the machine can honour and `satl --limits` can print. facts.hpp's
// kStackLimitUnknown already refuses to read the word as unbounded and this is
// the same care from the writing side.
// AND IT IS A CONSTANT ON A MACHINE THIS PROGRAM CAN MEASURE, WHICH IS THE NEXT
// THING TO FIX HERE. `facts::mem_total_bytes()` is read a few lines further down
// this same startup; 8 GiB is a quarter of a 32 GiB laptop and a four-hundredth
// of a 4 TiB machine, and asking for a share rather than a number would be right
// on both. **It costs nothing to ask for more** -- the reservation is address
// space and a stack is lazily committed, so a terabyte machine could be handed a
// terabyte-shaped request for the same zero bytes of RSS this one costs.
//
// TWO THINGS HAVE TO BE DECIDED FIRST AND NEITHER IS HARD. What the share is
// (memory total, or `MEMORY_MAX` -- which is the number satl is actually allowed
// and is read AFTER this, so the order would have to change); and what the floor
// is, because a share of a small machine must not come out below the 8 MiB it
// would have had. Recorded rather than done, because it is a policy with a
// number in it and this file is where those get argued.
inline constexpr unsigned long long kWantedStackBytes = 8ULL * 1024 * 1024 * 1024;

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
