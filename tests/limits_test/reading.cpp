// The file: what a value means, what a unit means, where an answer came from,
// and one assertion per row of errors.def's S08xx block.
//
// ONE ASSERTION PER ROW IS THE CHECK A REGISTRY NEEDS AND BESPOKE STRINGS COULD
// NOT HAVE HAD. MILESTONES/M5.md §5 states the defect it exists for: a site
// raising the WRONG ROW renders perfectly, builds quietly, and describes a
// different problem, and no static_assert can see it. reporter_test does this
// for the parser's nineteen rows; this is the same check for the config
// reader's nine, and both of the rows example/broken_config.ini cannot reach
// are raised here from a string.

#include "limits_test.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"

#include <string>
#include <vector>

namespace limits_test {

using satellite::errors::Code;
using satellite::limits::DialId;
using satellite::limits::Held;
using satellite::limits::Origin;

namespace {

bool raises(const std::string &text, Code wanted)
{
    for (const Code code : codes_in(text))
        if (code == wanted)
            return true;
    return false;
}

// A file that must READ, with what it produced.
Held reading_of(const std::string &text)
{
    Held into;
    std::vector<satellite::errors::Diagnostic> problems;
    check(reads(text, into, problems),
          "this config was meant to read and did not: " + text);
    return into;
}

} // namespace

void section_reading()
{
    // --- the shapes a line may take --------------------------------------

    check(codes_in("").empty(), "an empty file is a legal config");
    check(codes_in("\n\n   \n\t\n").empty(), "blank lines are legal");
    check(codes_in("# a comment\n; and the other kind\n").empty(),
          "`#` and `;` both open a comment, because .ini files in the wild use "
          "either and refusing one would be a rule with nothing behind it");
    check(codes_in("  THREAD_COUNT = 4  \n").empty(),
          "blanks around a name and a value are not part of either");
    check(codes_in("THREAD_COUNT=4").empty(),
          "a last line with no newline after it is still a line");

    // --- values -----------------------------------------------------------

    check(reading_of("THREAD_COUNT=4\n").thread_count.value == 4,
          "THREAD_COUNT is what the file says");
    check(reading_of("THREAD_COUNT=4\n").thread_count.origin == Origin::File,
          "and it says so: a value from the file is marked as from the file");
    check(reading_of("THREAD_COUNT=4\n").thread_count.line == 1,
          "with the line it came from, so `satl --limits` can name it");

    // A FILE THAT SETS ONE THING CHANGES ONE THING, which is the whole reason
    // limits.cpp fills in the machine's answers BEFORE opening the file. This
    // Held starts empty rather than filled, so what is checked here is the
    // narrower half: the reader does not touch what the file does not name.
    check(reading_of("THREAD_COUNT=4\n").core_count.origin == Origin::Default,
          "a file that names one setting leaves the others alone");

    // --- the units --------------------------------------------------------
    //
    // §4.5.4 ASKED WHAT UNIT MEMORY_MAX IS IN AND THE ANSWER IS "the one the
    // file writes down". These are the two families with their real meanings,
    // and the pair on the last line is the whole argument for requiring a unit
    // at all: 1 GiB and 1 GB are different numbers of bytes and neither is
    // wrong.
    check(reading_of("MEMORY_MAX=1B\n").memory_max.value == 1, "B is a byte");
    check(reading_of("MEMORY_MAX=1KiB\n").memory_max.value == 1024, "KiB is 1024");
    check(reading_of("MEMORY_MAX=1MiB\n").memory_max.value == 1048576, "MiB is 1024^2");
    check(reading_of("MEMORY_MAX=1GiB\n").memory_max.value == 1073741824ULL,
          "GiB is 1024^3");
    check(reading_of("MEMORY_MAX=1TiB\n").memory_max.value == 1099511627776ULL,
          "TiB is 1024^4");
    check(reading_of("MEMORY_MAX=1KB\n").memory_max.value == 1000, "KB is 1000");
    check(reading_of("MEMORY_MAX=1MB\n").memory_max.value == 1000000, "MB is 1000^2");
    check(reading_of("MEMORY_MAX=1GB\n").memory_max.value == 1000000000ULL,
          "GB is 1000^3");
    check(reading_of("MEMORY_MAX=1TB\n").memory_max.value == 1000000000000ULL,
          "TB is 1000^4");
    check(reading_of("MEMORY_MAX=1GiB\n").memory_max.value !=
              reading_of("MEMORY_MAX=1GB\n").memory_max.value,
          "GiB and GB are different amounts of memory, which is why the file "
          "has to say which one it means");

    check(reading_of("MEMORY_MAX=48gib\n").memory_max.value ==
              reading_of("MEMORY_MAX=48GiB\n").memory_max.value,
          "a unit is matched without regard to case -- somebody typing a "
          "config is not choosing between `GiB` and `gib`");
    check(reading_of("MEMORY_MAX=48 GiB\n").memory_max.value ==
              reading_of("MEMORY_MAX=48GiB\n").memory_max.value,
          "a space between the amount and its unit is allowed");

    // --- the dials --------------------------------------------------------

    check(!Held().dial(DialId::MinFreeMb).set,
          "a dial nobody has set is UNSET and not zero, because three of the "
          "four have no reader until M8, M9 and M15 and a default would be "
          "those milestones' decision taken early");
    check(reading_of("min_free_mb=512\n").dial(DialId::MinFreeMb).set,
          "a dial the file sets is set");
    check(reading_of("min_free_mb=512\n").dial(DialId::MinFreeMb).value == 512,
          "to what the file said");
    check(reading_of("float_digits=17\n").dial(DialId::FloatDigits).value == 17,
          "including the three nothing reads yet -- storage is what M6 owes "
          "them, and PLAN M6 says so");

    // --- every row of the S08xx block -------------------------------------

    check(raises("[machine]\n", Code::CONFIG_NOT_A_SETTING),
          "S0801: a section header is not a setting");
    check(raises("nonsense\n", Code::CONFIG_NOT_A_SETTING),
          "S0801: a bare word is not a setting");
    check(raises("=4\n", Code::CONFIG_NOT_A_SETTING),
          "S0801: a value with no name is not a setting");
    check(raises("THREAD_COUNTS=4\n", Code::CONFIG_NO_SUCH_SETTING),
          "S0802: this file has seven keys and that is not one of them");
    check(raises("division_digits=1\ndivision_digits=2\n", Code::CONFIG_SET_TWICE),
          "S0803: a setting is set once");
    check(raises("division_digits=1\ndivision_digits=2\n", Code::NOTE_CONFIG_SET_HERE),
          "S0890: and the note points at the first one");
    check(raises("CORE_COUNT=twelve\n", Code::CONFIG_NOT_A_NUMBER),
          "S0804: a count is a whole number");
    check(raises("max_depth=twelve\n", Code::CONFIG_NOT_A_NUMBER),
          "S0804: and so is a dial");
    check(raises("MEMORY_MAX=61.9GiB\n", Code::CONFIG_NOT_A_WHOLE_AMOUNT),
          "S0809: a fraction of a unit is refused, and with its own sentence "
          "rather than S0804's -- the fix is the next unit down and not `write "
          "a number`, and the first version of this reader answered `.9GiB is "
          "not a unit`, which pointed at the wrong half of the value");
    check(raises("MEMORY_MAX=61,9GiB\n", Code::CONFIG_NOT_A_WHOLE_AMOUNT),
          "S0809: including with the comma half the world writes it with");
    check(raises("MEMORY_MAX=48\n", Code::CONFIG_NEEDS_A_UNIT),
          "S0805: a size says how much OF WHAT");
    check(raises("MEMORY_MAX=48 gigs\n", Code::CONFIG_NO_SUCH_UNIT),
          "S0806: and `gigs` is not one of the nine -- the row "
          "example/broken_config.ini cannot reach, because MEMORY_MAX is the "
          "only setting that takes a unit and that file spends it on S0805");
    check(raises("THREAD_COUNT=0\n", Code::CONFIG_TOO_SMALL),
          "S0807: zero threads is not a pool");
    check(raises("THREAD_COUNT=99999\n", Code::CONFIG_TOO_LARGE),
          "S0808: and a quarter of a million would be spawned at startup, on "
          "every run, before the first argument is read");

    // S0891, the second row no file in example/ can reach: it needs
    // twenty-one problems, and a file with twenty-one problems in it
    // demonstrates nothing except this cap.
    std::string many;
    for (int i = 0; i < 30; i++)
        many += "nonsense\n";
    check(raises(many, Code::NOTE_CONFIG_TOO_MANY),
          "S0891: after twenty problems the rest of the file is not read, and "
          "the twenty-first is SAID rather than shown by the output stopping");
    check(codes_in(many).size() <= 21 + 1,
          "and the cap is real: twenty problems and the note, not thirty");

    // --- the numbers that are not overflow --------------------------------
    //
    // A CEILING THAT SILENTLY BECAME A SMALL NUMBER IS A WATCHDOG THAT FIRES ON
    // A HEALTHY PROCESS, which is why both of these are refusals rather than
    // wraps. The first overflows the digits themselves; the second overflows
    // only after the multiplier is applied, and it is the one an obvious
    // implementation gets wrong.
    check(!codes_in("max_depth=99999999999999999999999999\n").empty(),
          "a number too big for the storage is refused and not wrapped");
    check(!codes_in("MEMORY_MAX=99999999999999999999TiB\n").empty(),
          "and so is one that only overflows once its unit is applied");
}

} // namespace limits_test
