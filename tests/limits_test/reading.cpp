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
#include "system_facts/facts.hpp"

#include <string>
#include <vector>

namespace limits_test {

using satellite::errors::Code;
using satellite::limits::DialId;
using satellite::limits::Held;
using satellite::limits::Origin;
using satellite::limits::Setting;

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

    check(reading_of("THREAD_COUNT=4\n").thread_count.value() == 4,
          "THREAD_COUNT is what the file says");
    check(reading_of("THREAD_COUNT=4\n").thread_count.origin == Origin::File,
          "and it says so: a value from the file is marked as from the file");
    check(reading_of("THREAD_COUNT=4\n").thread_count.line == 1,
          "with the line it came from, so `satl --limits` can name it");

    // A FILE THAT SETS ONE THING CHANGES ONE THING, and since 2026-08-31 that
    // falls out of the shape rather than being arranged by limits.cpp filling
    // the machine's answers in first. A row nobody wrote a line for is still
    // `Origin::Default`, which IS "ask the machine".
    check(reading_of("THREAD_COUNT=4\n").core_count.origin == Origin::Default,
          "a file that names one setting leaves the others alone");

    // --- the machine's own answer, named ----------------------------------
    //
    // DESIGN §7.7's PAIRING, CHECKED. "arguments.machine.threads,
    // arguments.machine.cores and arguments.memory.total are the same numbers
    // as THREAD_COUNT, CORE_COUNT and MEMORY_MAX in the configuration." Before
    // 2026-08-31 the file could not say any of it and satl read all three from
    // the machine on every run to make up for it -- 0.42 ms of sysfs per run
    // for a number only `satl --limits` prints.
    check(reading_of("CORE_COUNT=arguments.machine.cores\n").core_count.origin ==
              Origin::Machine,
          "a setting may name the machine's own answer instead of a number, and "
          "the row says which -- `the machine, named by the file`");
    check(reading_of("CORE_COUNT=arguments.machine.cores\n").core_count.line == 1,
          "with the line, the same as a number: the question `where do I change "
          "it` has the same answer either way");
    check(reading_of("CORE_COUNT=arguments.machine.cores\n").core_count.value() ==
              satellite::facts::physical_cores(),
          "and it answers what the machine answers, which is the whole of what "
          "naming it means");
    check(reading_of("THREAD_COUNT=arguments.machine.threads\n")
                  .thread_count.value() == satellite::facts::hardware_threads(),
          "the same for THREAD_COUNT, whose fact is a different one");
    check(reading_of("MEMORY_MAX=arguments.memory.total\n").memory_max.value() ==
              satellite::facts::mem_total_bytes(),
          "and for MEMORY_MAX, which is the one that takes a unit when it is a "
          "number and takes none when it is a fact");

    // ALL SIX SPELLINGS, WHICH IS NOT A LIST THIS MODULE KEEPS. config.cpp
    // walks the real trie, so the aliases work here for the same reason they
    // work inside a program -- words.def's five SAT_ALIAS rows plus the node's
    // own text. A string comparison would have needed its own copy of that
    // list, which is the drift the whole module is arranged to avoid.
    for (const std::string spelling :
         {"arg", "args", "argz", "argument", "arguments", "argumentz"})
        check(reading_of("CORE_COUNT=" + spelling + ".machine.cores\n")
                      .core_count.origin == Origin::Machine,
              "`" + spelling + ".machine.cores` is the same node, because "
              "DESIGN §7.7 gives the special variable six names");

    // A DEFAULT ROW AND A NAMED ROW ANSWER THE SAME AND ARE NOT THE SAME THING,
    // which is why Origin has four values and not three. Both read the machine;
    // only one of them was asked for, and `satl --limits` prints the difference
    // because "what is satl holding to" and "who decided that" are different
    // questions.
    check(Setting{}.origin == Origin::Default,
          "a setting nobody has said anything about reads from the machine");
    check(reading_of("CORE_COUNT=arguments.machine.cores\n").core_count.value() ==
              Held().core_count.value(),
          "so a named fact and an absent line hold satl to the same number");
    check(reading_of("CORE_COUNT=arguments.machine.cores\n").core_count.origin !=
              Held().core_count.origin,
          "and are still told apart, because one of them is a decision somebody "
          "wrote down");

    // --- the units --------------------------------------------------------
    //
    // §4.5.4 ASKED WHAT UNIT MEMORY_MAX IS IN AND THE ANSWER IS "the one the
    // file writes down". These are the two families with their real meanings,
    // and the pair on the last line is the whole argument for requiring a unit
    // at all: 1 GiB and 1 GB are different numbers of bytes and neither is
    // wrong.
    check(reading_of("MEMORY_MAX=1B\n").memory_max.value() == 1, "B is a byte");
    check(reading_of("MEMORY_MAX=1KiB\n").memory_max.value() == 1024, "KiB is 1024");
    check(reading_of("MEMORY_MAX=1MiB\n").memory_max.value() == 1048576, "MiB is 1024^2");
    check(reading_of("MEMORY_MAX=1GiB\n").memory_max.value() == 1073741824ULL,
          "GiB is 1024^3");
    check(reading_of("MEMORY_MAX=1TiB\n").memory_max.value() == 1099511627776ULL,
          "TiB is 1024^4");
    check(reading_of("MEMORY_MAX=1KB\n").memory_max.value() == 1000, "KB is 1000");
    check(reading_of("MEMORY_MAX=1MB\n").memory_max.value() == 1000000, "MB is 1000^2");
    check(reading_of("MEMORY_MAX=1GB\n").memory_max.value() == 1000000000ULL,
          "GB is 1000^3");
    check(reading_of("MEMORY_MAX=1TB\n").memory_max.value() == 1000000000000ULL,
          "TB is 1000^4");
    check(reading_of("MEMORY_MAX=1GiB\n").memory_max.value() !=
              reading_of("MEMORY_MAX=1GB\n").memory_max.value(),
          "GiB and GB are different amounts of memory, which is why the file "
          "has to say which one it means");

    check(reading_of("MEMORY_MAX=48gib\n").memory_max.value() ==
              reading_of("MEMORY_MAX=48GiB\n").memory_max.value(),
          "a unit is matched without regard to case -- somebody typing a "
          "config is not choosing between `GiB` and `gib`");
    check(reading_of("MEMORY_MAX=48 GiB\n").memory_max.value() ==
              reading_of("MEMORY_MAX=48GiB\n").memory_max.value(),
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

    // A DIAL IS BOUNDED ONLY ONCE ITS MEANING EXISTS, and on 2026-08-31 exactly
    // one of the four had one. M8 gave `division_digits` a meaning, so the two
    // things a count of digits cannot be became knowable and are now refused
    // where a caret can land under them -- which is the whole of what
    // MILESTONES/M8.md §6 left open, arriving at the file rather than at M11.
    check(reading_of("division_digits=50000\n")
                  .dial(DialId::DivisionDigits)
                  .value == 50000,
          "a wide division_digits is ACCEPTED -- there is no ceiling on what a "
          "division may spend, and satl computes all 50,000 digits");
    check(raises("division_digits=0\n", Code::CONFIG_TOO_SMALL),
          "S0807: keeping no digits is not an answer, and it used to become 34 "
          "inside limits::division_digits() with nothing said");
    check(raises("division_digits=99999999999\n", Code::CONFIG_TOO_LARGE),
          "S0808: and a count too wide to hold used to become UINT_MAX the "
          "same silent way");

    // TWO DIALS ARE STILL UNBOUNDED AND TWO NOW HAVE RANGES -- this check
    // said "the other three are unbounded" until M15 gave `float_digits` its
    // reader, its floor, and its INT_MAX ceiling (limits.hpp carries why the
    // ceiling is a representation fact, not a bound on the language), and
    // this suite noticed on the day, which is exactly the rot it was written
    // to catch. `max_depth` and `min_free_mb` keep their no-range reasons:
    // any number of bytes is a number of bytes.
    check(raises("float_digits=0\n", Code::CONFIG_TOO_SMALL),
          "S0807: keeping no fractional places is not an answer, "
          "division_digits' argument at the fourth dial");
    check(raises("float_digits=99999999999\n", Code::CONFIG_TOO_LARGE),
          "S0808: a right half longer than a Number's exponent can place has "
          "no Number to be rounded in");
    check(!raises("max_depth=0\n", Code::CONFIG_TOO_SMALL) &&
              !raises("min_free_mb=0\n", Code::CONFIG_TOO_SMALL),
          "a dial with a meaning and no bound takes any whole number -- zero "
          "bytes of stack refuses the first push and says so, which is a "
          "working answer");

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
    check(raises("max_depth=twelve\n", Code::CONFIG_NOT_A_NUMBER),
          "S0804: a dial is a whole number, and a dial is the only thing S0804 "
          "is about since 2026-08-31 -- a MACHINE setting may also be the "
          "machine's own answer, so `CORE_COUNT=twelve` gets S0810's sentence, "
          "which names both of the things it could have been");
    check(raises("CORE_COUNT=twelve\n", Code::CONFIG_NOT_A_FACT),
          "S0810: a machine setting is a whole number or the one path that "
          "names its fact");
    check(raises("CORE_COUNT=arguments.memory.total\n", Code::CONFIG_NOT_A_FACT),
          "S0810: including a real path that is the WRONG fact -- both walk to "
          "a node, so a shape check could not tell them apart and DESIGN §7.7's "
          "pairing has to be enforced one setting at a time");
    check(raises("CORE_COUNT=satellite.library.main.arguments.machine.cores\n",
                 Code::CONFIG_NOT_A_FACT),
          "S0810: and the ROOTED spelling, which WORD_NUMBERS.md writes and "
          "`satl --words` answers to -- it walks, it walks to the right node, "
          "and it is still not what a program writes");
    check(raises("CORE_COUNT=\n", Code::CONFIG_NOT_A_FACT),
          "S0810: nothing after the `=` is answered by the sentence that lists "
          "everything the value could have been, which for a machine setting is "
          "two things and not one");
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
