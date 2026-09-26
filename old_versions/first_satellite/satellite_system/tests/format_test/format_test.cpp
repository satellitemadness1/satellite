// format.def / format.hpp — the registry, and the invariants it had been
// running on trust.
//
// Most of what matters here has already happened by the time main() runs: the
// static_asserts at the bottom of format.hpp fire at COMPILE time, so a duplicate
// id, a hole in the registry, a path naming an undefined word or a kind space
// wider than four bits fails the build rather than this binary. Including the
// header is itself the larger half of the test, and that is deliberate — every
// future consumer inherits the same checks.
//
// What is left for run time is the part a static_assert cannot reach:
//
//   1. The worked examples in DESIGN.md are literal claims about bits, and they
//      are checked here against the tables. §17 spells out
//      satellite.console.display as {1, 6, 7, 0}; if the registry ever disagrees
//      with the document's own hex dump, one of them is lying to a reader.
//   2. The text column. A missing switch arm is NOT the risk here and this file
//      once claimed it was: the switch and the enum are two expansions of one
//      list, so an arm cannot go missing, and -Wswitch would say so if it could.
//      What nothing checks is the SPELLING — the third macro argument is free
//      text, and a disassembler is only as good as it. So the probes are for
//      words whose identifier and text deliberately differ (TRUE_, AND_), which
//      is where a typo would actually land.
//   3. The counts. §17.5 quotes figures for how many selectors exist and which
//      ids they span; those numbers are in prose and go stale, so the true ones
//      are computed and printed, and the shape is asserted.
//
// THIS FILE is the driver: the check and check_str pair every section calls, the
// one failure counter, the figures printed at the end, and main(). The sections
// themselves are one file per topic beside it -- format_test_kinds.cpp,
// format_test_registry.cpp, format_test_selectors.cpp, format_test_paths.cpp
// and format_test_ops.cpp -- and format_test.hpp declares them so that main()
// can call them in the order they are written here.

#include "format_test.hpp"

#include <cstdio>
#include <cstring>
#include <string>

using namespace satellite::format;

// Defined once, here, and declared extern in format_test.hpp. See the note
// there: the sections live in separate translation units now, so a static copy
// per file would count failures nobody reads.
int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

void check_str(const char *got, const char *want, const char *what)
{
    if (std::strcmp(got, want) != 0) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what, want, got);
        failures++;
    }
}

// --- report -----------------------------------------------------------------
//
// This one stays with main() rather than moving to a topic file: it asserts
// nothing -- there is not one check call in it -- and what it prints leads
// straight into the PASS line below, so the two are one piece of output.

static void report()
{
    // The figures §17 quotes in prose. Printed rather than asserted where the
    // true number is a property of the language and will keep moving; the point
    // is that the document can be corrected FROM here.
    uint64_t lo = 0, hi = 0;
    for (size_t i = 0; i < kSelectorCount; i++) {
        const uint64_t id = static_cast<uint64_t>(kSelectors[i]);
        if (lo == 0 || id < lo)
            lo = id;
        if (id > hi)
            hi = id;
    }
    printf("  registry: %zu words, ids 1..%llu, 18..20 reserved\n", kWordCount,
           (unsigned long long)kWordIds[kWordCount - 1]);

    // RUNS, not a span with holes named. The line before this one printed
    // "spanning 38..86, with 56 and 73 absent", which was readable only while
    // the selectors were one nearly-solid block; `open` (31) is a selector as of
    // §8.3.1's completion, and the same code would now print a span of 31..101
    // followed by every one of the twenty-odd module and type words in between.
    // A shape that degenerates the moment the data stops being contiguous is the
    // shape this file keeps having to replace — first the literal range in §17.5,
    // then the two hardcoded holes here. Contiguous runs say the same thing and
    // go on saying it however the set is scattered.
    printf("  selectors: %zu, in runs", kSelectorCount);
    const char *sep = " ";
    for (uint64_t id = lo; id <= hi; id++) {
        if (!is_selector(static_cast<Word>(id)))
            continue;
        uint64_t end = id;
        while (end + 1 <= hi && is_selector(static_cast<Word>(end + 1)))
            end++;
        if (end == id)
            printf("%s%llu", sep, (unsigned long long)id);
        else
            printf("%s%llu..%llu", sep, (unsigned long long)id,
                   (unsigned long long)end);
        sep = ", ";
        id = end;
    }
    printf("\n");
    size_t variadic = 0;
    for (size_t i = 0; i < kPathCount; i++)
        if (is_variadic(kPaths[i]))
            variadic++;
    printf("  paths: %zu with a recorded arity, %zu of them variadic\n",
           kPathCount, variadic);
}

int main()
{
    test_kinds();
    test_registry();
    test_selectors();
    test_paths();
    test_variadic();
    test_ops();
    report();

    if (failures) {
        printf("FAILURES: %d\n", failures);
        return 1;
    }
    printf("PASS: format (eight kinds with spacesuit ids and machine ops apart "
           "and durations apart from immediates; "
           "the registry frozen, ascending and holed only at 18..20; selectors "
           "as a list rather than a stale range; the arity table keyed by whole "
           "path, with satellite.help variadic rather than silently zero; word "
           "and op spaces independent)\n");
    return 0;
}
