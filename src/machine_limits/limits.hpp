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
// ONE THING ASKS ANYWAY, AND IT IS NOT A SETTING. The stack satl raises itself
// to is a share of total memory as of 2026-08-31, so begin() reads
// /proc/meminfo once on every run before it looks at anything else. That is one
// small file against physical_cores()' 48, and it is the exception rather than
// the order coming back: no SETTING is read early, and begin() has the cost.
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
#include "system_facts/facts.hpp"

#include <climits>
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

// WHAT satl ASKS THE KERNEL FOR: 32 KiB OF STACK FOR EVERY 1 MiB OF MEMORY THE
// MACHINE HAS, never less than 128 MiB, and with no ceiling over it. That is
// 1.9 GiB on this 61.9 GiB machine, 32 GiB on a terabyte one and 128 GiB on a
// four -- a thirty-second of the machine, written in the author's own units
// because a rate is a sum a reader can check without a calculator, and "3.125%
// of memory, floored" is not.
//
// A SHARE AND NOT A CONSTANT, DECIDED 2026-08-31, and it is the whole of what
// SCRATCH.md/NO_LIMITS.md §4.1.2 asked for. This shipped that morning as a flat
// 8 GiB: a quarter of a 32 GiB laptop and a four-hundredth of a 4 TiB machine,
// and no one number is right on both. "We will be totally geared towards the
// terabytes of ram that are coming out in the future." limits.cpp carries the
// two decisions the share needed and why each went the way it did.
//
// SO IT ASKS FOR LESS THAN 8 GiB ON ANY MACHINE UNDER 256 GiB, WHICH IS THE
// TRADE AND WAS MADE WITH THE NUMBERS IN FRONT OF IT. Here that is 1.9 GiB
// where the morning's constant asked for 8.0 -- and 500,000 nested brackets
// still check and unparse at it, measured 2026-08-31, on a machine whose
// largest real satellite program nests at brace depth SIX (MILESTONES/M7.md).
// A rate generous enough to beat a constant chosen for a laptop would have to
// be absurd on the machine this rate is aimed at, and the constant is the thing
// being replaced.
//
// A NUMBER AND NOT A CEILING, AND THE DIFFERENCE IS THE WHOLE ARGUMENT.
// DESIGN §7.5's rule is that the language has no depth limit; this does not
// deliver that and is not pretending to -- it is the same C++ stack with a
// bigger default, and the thing that delivers the rule is a walker keeping its
// own stack on the heap.
//
// AND IT COSTS NOTHING UNTIL IT IS USED. A stack is lazily committed, so this
// is address space and not memory: the 8 GiB reservation moved VmSize by
// 0.0 MiB and a terabyte machine's 32 GiB costs the same nothing.
// 040-sources.mk carries what it did to startup, including the one
// /proc/meminfo read the share adds to every run of satl.
//
// A NUMBER RATHER THAN "unlimited" ON PURPOSE. RLIM_INFINITY makes the main
// thread's stack grow until it collides with the next mapping, which is a wall
// in a place nobody chose and reports itself as a segfault; a number is one the
// machine can honour and `satl --limits` can print. facts.hpp's
// kStackLimitUnknown already refuses to read the word as unbounded and this is
// the same care from the writing side.
inline constexpr unsigned long long kStackPerMegabyte = 32ULL * 1024;

// AND THE FLOOR, WHICH IS THE ONE PLACE A NUMBER STILL DECIDES ANYTHING HERE.
// 128 MiB, reached at 4 GiB of memory, so it is the answer for a small
// container and for a machine that would not say what it has -- and it is
// sixteen times the 8 MiB a login shell hands out, because a machine being
// small is not a reason for its programs to be shallow.
inline constexpr unsigned long long kStackFloorBytes = 128ULL * 1024 * 1024;

// The share, taken from the machine's own total -- and the same rule for a
// caller that has ALREADY read it. TWO ENTRY POINTS AND ONE RULE, which is the
// pair Setting::value_given() above is the other half of and is here for the
// same reason: `satl --limits` reads every machine fact once, and a second
// /proc/meminfo read to print a number it can already compute would be this
// command reporting the machine from two different instants.
unsigned long long wanted_stack_bytes();
unsigned long long wanted_stack_bytes_given(unsigned long long memory_total);

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

// --- the dials that have a meaning -----------------------------------------

// WHAT A NON-TERMINATING DIVISION KEEPS, in significant digits.
//
// THE FIRST DIAL TO BE GIVEN A MEANING, AND M6 LEFT THE HOLE FOR IT ON PURPOSE.
// The Dial note above says an unset dial exists because "three of the four have
// no reader at M6 and inventing a default for them would be inventing their
// meaning ... the milestone that will read it decides what no answer means".
// M8 is that milestone for `satellite.library.system.division_digits`
// `1 14 2 1`, and this function is the whole of the decision: unset means 34.
//
// 34 IS decimal128's PRECISION, and the first satellite chose it with the
// argument that still holds -- wide enough that ordinary arithmetic never
// notices, narrow enough that 1/3 is readable. It lives HERE, beside the dial,
// rather than in Number: v1 kept a DEFAULT_DIVISION_DIGITS in its bignum header
// because its evaluator read the library namespace itself, and in this tree the
// namespace is this module. Number::divide takes a count and never invents one.
//
// THERE IS NO CEILING ANY MORE AND THAT IS THE CORRECTION THIS BLOCK CARRIES.
// Until 2026-08-31 Number::kMaxDivisionDigits capped a division at 10,000
// significant digits, so a file setting `division_digits=50000` was clamped
// inside divide() and nothing told the program. DESIGN §7.5 does not allow the
// cap and DESIGN §1.1 does not allow the silence, and the second was only ever
// a symptom of the first: with the cap gone there is nothing to say, because
// 50,000 digits is 50,000 digits. What ends a runaway is M6's watchdog at
// MEMORY_MAX -- the same answer errors.def's S06xx block gives for why there is
// no overflow row.
//
// WHAT IS BOUNDED IS WHAT THE FILE MAY SAY, AND THAT IS A DIFFERENT CLAIM.
// A count of digits below one keeps nothing, and a count satl cannot hold in a
// machine word is a count it cannot act on. Both are refused where a caret can
// land under them -- S0807 and S0808, in machine_limits/config.cpp, on the line
// of the satellite_config.ini that wrote them -- rather than substituted for
// here, which is what this function did to a zero until 2026-08-31. That is the
// whole difference between a bound on the LANGUAGE, which §7.5 forbids, and a
// bound on what a configuration FILE is allowed to contain, which every other
// setting in this module has had since M6.
inline constexpr unsigned kDivisionDigitsDefault = 34;
inline constexpr unsigned long long kDivisionDigitsLeast = 1;
inline constexpr unsigned long long kDivisionDigitsMost = UINT_MAX;

unsigned division_digits();

// DESIGN §8.6's DEFAULT length of a float's right half, for a value that does
// not state one -- §13 redefined the dial into that default on 2026-08-27,
// and M15 is the milestone that gave it a reader. 34 BESIDE division_digits'
// 34 DELIBERATELY: both are "the default width of an inexact result", and two
// different arbitrary constants would be two facts where the language has one
// (evaluator/machine.hpp's Policy row is the other half of this note).
//
// THE CEILING IS INT_MAX AND NOT UINT_MAX, AND IT IS A REPRESENTATION FACT
// RATHER THAN A BOUND ON THE LANGUAGE. A fractional place lives in a Number's
// base-10 exponent, which is an int (bignum_number.hpp's small form); a right
// half longer than INT_MAX places has no Number to be rounded IN, so a file
// asking for one is refused at its own line -- S0808's job -- instead of
// wrapping into a wrong answer somewhere under float_arith.cpp.
inline constexpr unsigned kFloatDigitsDefault = 34;
inline constexpr unsigned long long kFloatDigitsLeast = 1;
inline constexpr unsigned long long kFloatDigitsMost = INT_MAX;

unsigned float_digits();

// A MEMORY CEILING ON THE EVALUATOR'S CONTROL STACK, IN BYTES.
//
// `satellite.library.system.max_depth` `1 14 2 2`, the author's decision of
// 2026-09-01, and PLAN §8's M9 entry carries the whole argument. A numbered path
// cannot be deleted -- WORD_NUMBERS §1.2 is "never renumber, never reuse" -- so
// the dial had to mean something, and of the three readings on the table this is
// the one that keeps its NAME honest: a depth measured in what depth actually
// costs. It is a sibling of MEMORY_MAX rather than of `ulimit -s`.
//
// WHAT IT BUYS IS THE SENTENCE. M6's watchdog already stops a runaway recursion
// -- touched stack pages are resident memory and it counts them -- but it can
// only say the run is using N and MEMORY_MAX is M.
// SCRATCH.md/NO_LIMITS.md §8's first question is exactly that gap, and a ceiling
// on the control stack knows it IS the control stack, so evaluator/machine.cpp
// can tell a runaway recursion about recursion. It is also cheaper than a frame
// count: the check happens when the stack GROWS, not on every push.
//
// UNSET MEANS MEMORY_MAX, WHICH IS THE MACHINE, AND THAT IS ONE DECISION AND
// NOT TWO. PLAN §8's M9 entry says "the default is the machine", citing §4.5.4's
// answer for MEMORY_MAX -- "the default is the WHOLE MACHINE, because a fraction
// is a number satl would have invented about a program it has never seen". This
// reads that number off the sibling that already holds it rather than opening
// /proc/meminfo a second time, and the two are the same value whenever
// MEMORY_MAX is unset.
//
// WHERE THEY DIFFER, READING THE SIBLING IS THE ANSWER THAT WORKS. A user who
// sets MEMORY_MAX=1GiB has said satl may have a gigabyte; a control-stack
// ceiling above that could never be reached, because the watchdog would stop the
// run first -- with the sentence this dial exists to improve on. So the ceiling
// follows what satl was actually given, and the recursion is caught one layer in
// where the growing thing has a name.
//
// AND IT HAS NO RANGE, WHICH IS THE ONE THING PLAN §8 EXPECTED AND DID NOT GET.
// That entry says "THE RANGE AND THE READER LAND TOGETHER, AT M9". The reader
// landed; there is no range, for the reason config_internal.hpp already writes
// out two rows away about `min_free_mb` -- "a meaning with no bound ever claimed
// for it". Any number of bytes is a number of bytes: zero refuses the first push
// and says so, and a number wider than any machine means the machine. Neither is
// a value satl cannot act on, and S0807 and S0808 exist for the ones that are.
unsigned long long max_depth_bytes();

// human_bytes() MOVED TO system_facts/facts.hpp AT M9, and this note is here
// because eleven call sites in this module still read it. It was declared here
// while every caller was in this module; the evaluator's S0701 is the second
// consumer and must not include this header -- evaluator/machine.hpp's Policy
// note says why. facts.hpp carries the argument and the function.
using facts::human_bytes;

} // namespace satellite::limits
