// The word registry -- the frozen list of ids in format.def, checked against the
// numbers DESIGN.md §17 quotes in prose, and against the one hole it allows.
// Part of the format_test binary; the harness is declared in format_test.hpp.
//
// Two of the checks below are deliberate TRIPWIRES rather than invariants: the
// `registry ends at` check and the name() probe of the last word both fail the
// moment a word is appended. That is the intent and the comments beside them say
// so -- a FROZEN registry should not grow without someone looking at it, and the
// fix is to bump the number in the same commit as the SAT_WORD row, not to
// loosen the check.

#include "format_test.hpp"

using namespace satellite::format;

// --- the registry -----------------------------------------------------------

void test_registry()
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
    check(static_cast<uint64_t>(Word::RANDOM) == 80, "the random block starts at 80 (§18)");
    check(kWordIds[kWordCount - 1] == 110, "the registry ends at 110");

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
    check_str(name(Word::RANGE), "range", "84 -> range");
    check_str(name(Word::SIZE), "size", "85 -> size");
    check_str(name(Word::DIGITS), "digits", "86 -> digits");
    check_str(name(Word::ANALYZE), "analyze", "87 -> analyze");
    check_str(name(Word::HOME), "home", "89 -> home");
    check_str(name(Word::DELETE_), "delete", "99 -> delete");
    check_str(name(Word::NEW_), "new", "100 -> new");
    // The LAST word in the registry, so this probe is also the one that catches
    // a missing switch arm at the END of the list -- the position a new word
    // always lands in, and the only one no earlier probe covers. It has to move
    // every time the registry grows, which is the point: it is the same
    // deliberate tripwire the `registry ends at` check above is.
    check_str(name(Word::TO_BINARY), "to_binary", "108 -> to_binary");
    check_str(name(Word::COUNT), "count", "109 -> count");
    check_str(name(Word::NAMES), "names", "110 -> names");

    // The identifiers are deliberately not the bare words where C++ forbids it,
    // and the quoted text is what the language actually calls the thing.
    check_str(name(Word::TRUE_), "true", "TRUE_ is spelled true");
    check_str(name(Word::AND_), "and", "AND_ is spelled and");
    check_str(name(Word::DELETE_), "delete", "DELETE_ is spelled delete");

    // An id past the end is not a word. A decoder reading a stream from a newer
    // version must be able to say so rather than index off the end of a table.
    check(!is_defined_word(111), "111 is not yet assigned");
    check(!is_defined_word(0), "0 is not a word — it means an absent segment");
}
