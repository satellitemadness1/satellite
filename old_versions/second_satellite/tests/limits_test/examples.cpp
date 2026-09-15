// The two files in example/, read off the disk.
//
// A FILE THAT MUST NOT READ IS AN INPUT WITH THE SAME STANDING AS ONE THAT
// MUST, which is FORMAT/CXX.md §6.2's rule from M4 carried one milestone on:
// "a file that starts parsing is a finding and not a pass." Both files are
// prerequisites of this binary in 065-tests.mk, so correcting either one
// re-runs the test that reads it -- without that line, fixing
// broken_config.ini would leave the assertion that it is broken reporting `ok`
// from a binary built before the fix.

#include "limits_test.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"

#include "system_facts/facts.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace limits_test {

using satellite::errors::Code;
using satellite::limits::DialId;
using satellite::limits::Held;
using satellite::limits::Origin;

namespace {

bool holds(const std::vector<Code> &codes, Code wanted)
{
    for (const Code code : codes)
        if (code == wanted)
            return true;
    return false;
}

} // namespace

void section_examples()
{
    // --- the one that must read -------------------------------------------

    const std::string good = example("satellite_config.ini");
    if (good.empty())
        return;

    Held into;
    std::vector<satellite::errors::Diagnostic> problems;
    check(reads(good, into, problems),
          "example/satellite_config.ini reads, and it is the file M6 means by "
          "done");
    check(problems.empty(),
          "with nothing to say about it -- a config that reads produces no "
          "diagnostics at all, the way `satl --check` produces none for a file "
          "that is fine");

    // THE NUMBERS THIS MACHINE MEASURED, 2026-08-27, WRITTEN INTO THE FILE:
    // PLAN §4.5's `lscpu` and /proc/meminfo reading is 24 hardware threads over
    // 12 physical cores. Checking them here is the same claim words_test makes
    // about WORD_NUMBERS.md -- the document and the code must agree -- with the
    // example file standing in for the document.
    check(into.thread_count.value() == 24, "it asks for 24 threads");
    check(into.thread_count.origin == Origin::File,
          "as a number, which is what makes THREAD_COUNT the demonstration "
          "that a setting and a fact are allowed to differ");
    check(into.core_count.origin == Origin::Machine,
          "and it names the machine's own answer for CORE_COUNT, which is the "
          "other half of the same demonstration -- one file, both spellings");
    check(into.core_count.value() == satellite::facts::physical_cores(),
          "so the core count is whatever this machine has, and on the machine "
          "PLAN §4.5 measured on 2026-08-27 that is 12");
    check(into.memory_max.value() == 48ULL * 1024 * 1024 * 1024,
          "and holds satl to 48 GiB, which is 48 * 1024^3 bytes exactly and "
          "not 48 * 1000^3");
    check(into.dial(DialId::MinFreeMb).set &&
              into.dial(DialId::MinFreeMb).value == 2048,
          "and sets the one dial anything reads at M6");
    check(!into.dial(DialId::DivisionDigits).set &&
              !into.dial(DialId::MaxDepth).set &&
              !into.dial(DialId::FloatDigits).set,
          "and leaves the other three unset, because they are commented out in "
          "that file -- which is what makes them a demonstration of the "
          "comment syntax as well as of the dials");

    for (const satellite::errors::Diagnostic &problem : problems)
        check(false, "example/satellite_config.ini raised " +
                         std::string(satellite::errors::code_text(problem.code).view()));

    // --- the one that must not --------------------------------------------

    const std::string bad = example("broken_config.ini");
    if (bad.empty())
        return;

    const std::vector<Code> raised = codes_in(bad);
    check(!raised.empty(),
          "example/broken_config.ini does NOT read, and a version of it that "
          "started reading would be a finding about the reader rather than a "
          "pass");

    // ONE ASSERTION PER LINE OF THAT FILE, IN THE ORDER IT WRITES THEM, so the
    // file's own comments and this list are checkable against each other by
    // reading them side by side -- which is what the comment column in
    // satellite_words/dump.cpp is for, applied to a test.
    check(holds(raised, Code::CONFIG_NOT_A_SETTING), "`[machine]` is S0801");
    check(holds(raised, Code::CONFIG_NO_SUCH_SETTING), "`THREAD_COUNTS` is S0802");
    check(holds(raised, Code::CONFIG_TOO_SMALL), "`THREAD_COUNT=0` is S0807");
    check(holds(raised, Code::CONFIG_TOO_LARGE), "`CORE_COUNT=99999` is S0808");
    check(holds(raised, Code::CONFIG_NEEDS_A_UNIT), "`MEMORY_MAX=48` is S0805");
    check(holds(raised, Code::CONFIG_NOT_A_NUMBER), "`max_depth=twelve` is S0804");
    check(!holds(raised, Code::CONFIG_NOT_A_FACT),
          "and S0810 is NOT in that file, which is a fact about the file rather "
          "than about the reader: all three machine settings are spent there on "
          "S0805, S0807 and S0808, and naming one a second time raises S0803. "
          "reading.cpp raises it from a string, the way S0806, S0809 and S0891 "
          "are raised");
    check(holds(raised, Code::CONFIG_SET_TWICE), "the second division_digits is S0803");
    check(holds(raised, Code::NOTE_CONFIG_SET_HERE), "and it carries the note S0890");

    check(raised.size() == 8,
          "eight and no more: seven errors and one note. A ninth would mean a "
          "line of that file raises something its own comment does not name");

    // AND THE SUGGESTION, WHICH IS M5's SUGGESTER OVER A LIST THAT IS NOT THE
    // TRIE. errors::suggest() searches one node's children, which is right for
    // a path and wrong for seven names that are not siblings -- three of them
    // are in no numbering at all -- so config_internal.hpp reuses the distance
    // and the threshold instead. This is the assertion that says it reused
    // enough of it.
    Held ignored;
    std::vector<satellite::errors::Diagnostic> found;
    reads(bad, ignored, found);
    bool suggested = false;
    for (const satellite::errors::Diagnostic &problem : found)
        if (problem.code == Code::CONFIG_NO_SUCH_SETTING)
            suggested = problem.suggestion == "THREAD_COUNT";
    check(suggested, "`THREAD_COUNTS` is answered with `did you mean "
                     "THREAD_COUNT?` and not with a list of seven");

    // --- the clamp, through begin() ---------------------------------------
    //
    // THE ONLY CHECK IN THIS SUITE THAT GOES THROUGH begin(), and it is here
    // because the clamp lives nowhere else. read_config() reads a file;
    // begin() is what fills in the machine's answers first, lets the file
    // overwrite them, and then decides that a ceiling above what the machine
    // HAS is not a ceiling at all. A mutation that clamped the value and
    // forgot to mark the row `Clamped` was invisible to every other assertion
    // here -- verified 2026-08-30 -- because everything else stops at the
    // reader.
    //
    // IT IS SAFE TO CALL, and that is worth saying because begin() also starts
    // a pool and a watchdog. pool::start() is a no-op once the pool exists, and
    // section_pool has already made it; the watchdog is one detached thread
    // whose MEMORY_MAX here is the machine's own total, so it wakes once a
    // second, compares, and goes back to sleep for as long as this binary runs.
    //
    // The file is written to /tmp, which is where satc_test already writes --
    // §5's write cannot be tested without a disk, and neither can this.
    const std::string path = "/tmp/satl_limits_test_clamp.ini";
    if (std::FILE *file = std::fopen(path.c_str(), "wb")) {
        std::fputs("MEMORY_MAX=1024TiB\n", file);
        std::fclose(file);

        // A ROW THAT ALREADY READS FROM THE MACHINE CANNOT BE CLAMPED, and
        // begin() does not even open /proc/meminfo to find that out -- which is
        // the whole reason the clamp tests the ORIGIN rather than the value.
        // Checked before the clamp below, because both go through the one
        // process-wide Held and the last call wins.
        const std::string named = "/tmp/satl_limits_test_named.ini";
        if (std::FILE *second = std::fopen(named.c_str(), "wb")) {
            std::fputs("MEMORY_MAX=arguments.memory.total\n", second);
            std::fclose(second);
            check(satellite::limits::begin(named) == 0,
                  "a config naming the machine's own memory reads");
            check(satellite::limits::held().memory_max.origin == Origin::Machine,
                  "and is NOT clamped, because a ceiling that IS the machine's "
                  "total is not above it -- there is nothing to compare and "
                  "nothing is read to compare it with");
            std::remove(named.c_str());
        }

        check(satellite::limits::begin(path) == 0,
              "a config asking for more memory than the machine has still READS "
              "-- it is not malformed, it is optimistic");

        const satellite::limits::Held &now = satellite::limits::held();
        check(now.memory_max.origin == Origin::Clamped,
              "and the row says so: `the machine, over the file`, which is what "
              "makes clamping a thing the user is told rather than a thing done "
              "behind their back");
        check(now.memory_max.value() ==
                  satellite::facts::mem_total_bytes(),
              "and the value is the machine's own total, because past that the "
              "run is not slow, it is over");
        check(now.memory_max.written == 1024ULL * 1024 * 1024 * 1024 * 1024,
              "while the file's own 1024TiB survives in `written`, which is the "
              "only place it survives at all -- the clamp moves the ORIGIN and "
              "does not overwrite what somebody asked for");
        check(now.config_path == path, "and the file it came from is named");
        std::remove(path.c_str());
    } else {
        check(false, "could not write " + path + " -- this check needs a disk, "
                     "and a check that cannot run is a failure and not a skip");
    }
}

} // namespace limits_test
