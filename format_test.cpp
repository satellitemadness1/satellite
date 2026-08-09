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

#include "format.hpp"

#include <cstdio>
#include <cstring>
#include <string>

using namespace satellite::format;

static int failures = 0;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

static void check_str(const char *got, const char *want, const char *what)
{
    if (std::strcmp(got, want) != 0) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what, want, got);
        failures++;
    }
}

// --- the kind space ---------------------------------------------------------

static void test_kinds()
{
    // §17's table, which is now the only place a kind is assigned. These seven
    // are asserted individually rather than by count, because the collision this
    // file exists to prevent was two DIFFERENT things claiming one number, and a
    // count would have been satisfied by that.
    check(static_cast<uint64_t>(Kind::NAME_CODE) == 0, "kind 0 is the name code");
    check(static_cast<uint64_t>(Kind::IMMEDIATE) == 1, "kind 1 is an immediate");
    check(static_cast<uint64_t>(Kind::POOL_REF) == 2, "kind 2 is a pool ref");
    check(static_cast<uint64_t>(Kind::REGISTER) == 3, "kind 3 is a register");
    check(static_cast<uint64_t>(Kind::SPACESUIT_ID) == 4, "kind 4 is a spacesuit id (§17.2)");
    check(static_cast<uint64_t>(Kind::CAPSULE_ID) == 5, "kind 5 is a capsule id (§17.2)");
    check(static_cast<uint64_t>(Kind::MACHINE_OP) == 6, "kind 6 is a machine op (§17.5)");

    // The collision itself, stated as the thing it is: two spaces, two tags.
    check(Kind::SPACESUIT_ID != Kind::MACHINE_OP,
          "a spacesuit id and a machine op are different kinds");

    check_str(name(Kind::MACHINE_OP), "machine op", "kind 6 names itself");
    check_str(name(Kind::SPACESUIT_ID), "spacesuit id", "kind 4 names itself");

    // Four bits. A kind past 15 does not survive the encoding at all, so the
    // ceiling is not a style preference.
    //
    // Over EVERY row, not over one named enumerator. This check read
    // `Kind::MACHINE_OP <= 15` under this same message, which is true forever and
    // says nothing about a row added later — the run-time half of a header assert
    // that was counting rows instead of bounding ids. Both were wrong the same
    // way, which is why the wrong one survived review of the other.
    for (size_t i = 0; i < sizeof(detail::kKindIds) / sizeof(uint64_t); i++)
        check(detail::kKindIds[i] <= 15, "every kind fits in four bits");
}

// --- the registry -----------------------------------------------------------

static void test_registry()
{
    // §17's worked example, word for word.
    check(static_cast<uint64_t>(Word::SATELLITE) == 1, "satellite is 1");
    check(static_cast<uint64_t>(Word::CONSOLE) == 6, "console is 6");
    check(static_cast<uint64_t>(Word::DISPLAY) == 7, "display is 7");

    // The block boundaries §17 records in prose.
    check(static_cast<uint64_t>(Word::PLUS) == 38, "the method block starts at 38");
    check(static_cast<uint64_t>(Word::EQUALS) == 67, "comparisons start at 67 (§17.5)");
    check(static_cast<uint64_t>(Word::MAP) == 73, "the map block starts at 73 (§8.6)");
    // From the DATA, not from Word::VALUES: `VALUES == 79` stays true after a
    // word is appended and quietly stops describing the end of the registry.
    //
    // This one is a deliberate tripwire and fails the moment a word is added,
    // which is the intent: adding to a FROZEN registry should not be something a
    // green build lets you do without looking. Bump the number here, in the same
    // commit as the SAT_WORD row.
    check(kWordIds[kWordCount - 1] == 79, "the registry ends at 79");

    // 18..20 are held for break and continue. This is the one hole allowed, and
    // it is checked from both sides so that filling it needs a deliberate edit.
    check(static_cast<uint64_t>(Word::VARIABLE) == 17, "17 is the last before the reserve");
    check(static_cast<uint64_t>(Word::BOOL) == 21, "21 is the first after the reserve");
    check(!is_defined_word(18) && !is_defined_word(19) && !is_defined_word(20),
          "18..20 are reserved and undefined (break, continue)");

    // Round-trip. A missing switch arm returns the default string, so probing a
    // word from each block catches a hole the compiler cannot see.
    check_str(name(Word::SATELLITE), "satellite", "1 -> satellite");
    check_str(name(Word::DISPLAY), "display", "7 -> display");
    check_str(name(Word::TO_STRING), "to_string", "47 -> to_string");
    check_str(name(Word::GREATER_OR_EQUAL), "greater_or_equal", "72 -> greater_or_equal");
    check_str(name(Word::VALUES), "values", "79 -> values");

    // The identifiers are deliberately not the bare words where C++ forbids it,
    // and the quoted text is what the language actually calls the thing.
    check_str(name(Word::TRUE_), "true", "TRUE_ is spelled true");
    check_str(name(Word::AND_), "and", "AND_ is spelled and");

    // An id past the end is not a word. A decoder reading a stream from a newer
    // version must be able to say so rather than index off the end of a table.
    check(!is_defined_word(80), "80 is not yet assigned");
    check(!is_defined_word(0), "0 is not a word — it means an absent segment");
}

// --- selectors --------------------------------------------------------------

static void test_selectors()
{
    // §17.5 wrote the builtin set as the range 38..72. It is a list because the
    // set is not contiguous, and these three are exactly where a range is wrong.
    check(is_selector(Word::PLUS), "plus is a selector");
    check(is_selector(Word::GREATER_OR_EQUAL), "greater_or_equal is a selector");
    check(is_selector(Word::GET), "get is a selector — 74, past the old 38..72");
    check(is_selector(Word::VALUES), "values is a selector — 79, past the old 38..72");

    // The two holes inside the span, each for its own reason.
    check(!is_selector(Word::MAP), "map is a type name, not a selector");
    check(!is_selector(Word::EMPTY), "empty answers to no receiver — id 56 stays spent");

    // Nothing outside the method blocks is a selector.
    check(!is_selector(Word::SATELLITE), "satellite is not a selector");
    check(!is_selector(Word::CONSOLE), "console is not a selector");
    check(!is_selector(Word::LIST), "list is a type name, not a selector");

    // The count §17.5 will quote. Computed, so the prose can be corrected from
    // the data rather than the other way round.
    check(kSelectorCount == 40, "there are 40 selectors");

    // Every selector sits in the method range, and no selector appears in the
    // arity table AT ALL: CALL_METHOD carries an explicit count, so a row would
    // be unreachable.
    //
    // Every segment of every row is scanned, not just `satellite.<sel>`. §7 makes
    // a method sugar for a module function over one table, so a selector could
    // plausibly be written into a third or fourth segment — and probing only the
    // two-segment shape would have called that absent when it was not.
    for (size_t i = 0; i < kSelectorCount; i++) {
        const uint64_t id = static_cast<uint64_t>(kSelectors[i]);
        check(id >= 38 && id <= 79, "a selector is in the method blocks");
        for (size_t j = 0; j < kPathCount; j++)
            for (int s = 0; s < 4; s++)
                check(kPaths[j].segment[s] != id, "a selector has no arity row");
    }
}

// --- the arity table --------------------------------------------------------

static void test_paths()
{
    // DESIGN.md §17.3 dumps satellite.console.display("hello, world!") in full,
    // and word 0 of its name code is quoted as 0x...0001 with word1 = 6 and
    // word2 = 7. This is that claim, checked against the table.
    const uint64_t display[4] = {1, 6, 7, 0};
    const Path *p = find_path(display);
    check(p != nullptr, "satellite.console.display has an arity row");
    if (p) {
        check(p->arity == 1, "display takes one operand unit");
        check_str(p->ident, "P_DISPLAY", "the display row is P_DISPLAY");
    }

    // Built from the enum rather than from literals, so renaming a word cannot
    // leave this passing against a stale number.
    const uint64_t built[4] = {static_cast<uint64_t>(Word::SATELLITE),
                               static_cast<uint64_t>(Word::CONSOLE),
                               static_cast<uint64_t>(Word::DISPLAY), 0};
    check(find_path(built) == p, "the enum and the hex dump agree");

    // Zero-argument paths exist and must not be confused with absent rows.
    const uint64_t now[4] = {1, 24, 30, 0};
    const Path *n = find_path(now);
    check(n != nullptr && n->arity == 0, "satellite.time.now takes no operands");

    // A path nobody has recorded is a stream that cannot be decoded, and
    // find_path says so rather than guessing.
    const uint64_t bogus[4] = {1, 6, 99, 0};
    check(find_path(bogus) == nullptr, "an unrecorded path has no arity");

    // Prefixes are distinct from the paths that extend them: the space is flat,
    // so only the whole quadruple identifies a row.
    const uint64_t prefix[4] = {1, 6, 0, 0};
    check(find_path(prefix) == nullptr, "satellite.console alone is not a path");

    check(kPathCount == 11, "eleven paths carry an arity");
}

// --- machine ops ------------------------------------------------------------

static void test_ops()
{
    check(static_cast<uint64_t>(Op::OP_MOVE) == 1, "the op space restarts at 1");
    // A count, from the data. `OP_RETURN == 9` was labelled "nine machine ops"
    // and checked neither the count nor the density — the ids being dense from 1
    // is now a static_assert in the header, which is what makes this a count.
    check(sizeof(detail::kOpIds) / sizeof(uint64_t) == 9, "nine machine ops");
    check(static_cast<uint64_t>(Op::OP_RETURN) == 9, "return is the last of them");

    check_str(name(Op::OP_CALL_METHOD), "call_method", "op 8 names itself");

    // The operand counts §17.4 and §17.5 specify.
    check(arity(Op::OP_MOVE) == 2, "move takes dst, src");
    check(arity(Op::OP_JUMP) == 1, "jump takes a target");
    check(arity(Op::OP_CALL) == 4, "call takes capsule, first arg, count, dst");
    check(arity(Op::OP_CALL_METHOD) == 5,
          "call_method takes recv, selector, first arg, count, dst");

    // Both polarities exist from the start, because && and || cannot lower to
    // the and/or selectors — a call evaluates its arguments and a short circuit
    // must not (§17.5).
    check(arity(Op::OP_JUMP_IF_TRUE) == 2 && arity(Op::OP_JUMP_IF_FALSE) == 2,
          "a branch of each polarity, both taking test and target");

    // The two spaces are separate. Op 7 and word 7 are unrelated, and reading one
    // against the other's table is the bug the kind tag exists to catch.
    check(static_cast<uint64_t>(Op::OP_CALL) == static_cast<uint64_t>(Word::DISPLAY),
          "op 7 and word 7 share a number, as separate spaces are entitled to");
    check_str(name(Op::OP_CALL), "call", "...and op 7 is call");
    check_str(name(Word::DISPLAY), "display", "...while word 7 is display");
}

// --- report -----------------------------------------------------------------

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

    // The holes are FOUND, not named. This line read "with 56 and 73 absent"
    // against two hardcoded enumerators, which would have gone on saying exactly
    // that after a third hole appeared — in a report whose whole purpose is to be
    // the thing prose gets corrected from.
    printf("  selectors: %zu, spanning %llu..%llu, with", kSelectorCount,
           (unsigned long long)lo, (unsigned long long)hi);
    const char *sep = " ";
    for (uint64_t id = lo; id <= hi; id++)
        if (is_defined_word(id) && !is_selector(static_cast<Word>(id))) {
            printf("%s%llu", sep, (unsigned long long)id);
            sep = " and ";
        }
    printf(" absent\n");
    printf("  paths: %zu with a recorded arity\n", kPathCount);
}

int main()
{
    test_kinds();
    test_registry();
    test_selectors();
    test_paths();
    test_ops();
    report();

    if (failures) {
        printf("FAILURES: %d\n", failures);
        return 1;
    }
    printf("PASS: format (seven kinds with spacesuit ids and machine ops apart; "
           "the registry frozen, ascending and holed only at 18..20; selectors "
           "as a list rather than a stale range; the arity table keyed by whole "
           "path; word and op spaces independent)\n");
    return 0;
}
