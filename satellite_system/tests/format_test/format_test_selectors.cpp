// The selector set -- which words answer to a receiver, which words only look
// like they should, and the count §17.5 quotes. Part of the format_test binary;
// the harness is declared in format_test.hpp.
//
// The third tripwire of the three this test holds lives here: `kSelectorCount ==
// 50` is a count and not a range, because the range is what kept going stale.
// The long comment inside the loop at the bottom records why the check that
// follows it was NARROWED rather than widened, and names the one-character fix
// it forbids; it is the most valuable thing in this file and it moves with the
// code it explains.

#include "format_test.hpp"

using namespace satellite::format;

// --- selectors --------------------------------------------------------------

void test_selectors()
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

    // §8.3.1's file surface, finished: `open` is a selector as well as the tail
    // of satellite.file.open, and `clear` is new.
    check(is_selector(Word::CLEAR), "clear is a selector — 101");

    // §21. The THIRD run, added rather than widening the second — the comment
    // on the range check below forbids the one-character fix by name.
    check(is_selector(Word::TO_BINARY), "to_binary is a selector — 108");
    check(is_selector(Word::NAMES), "names is a selector — 110, the newest");
    check(is_selector(Word::COUNT), "count is a selector — 109");
    check(!is_selector(Word::BINARY), "binary is a type name, not a selector");
    check(!is_selector(Word::HEX), "hex is a type name, not a selector");
    check(!is_selector(Word::HEXADECIMAL),
          "hexadecimal is a type name, not a selector — the one alias");
    check(is_selector(Word::OPEN),
          "open is a selector — 31, and also segment 3 of satellite.file.open");

    // The count §17.5 will quote. Computed, so the prose can be corrected from
    // the data rather than the other way round.
    check(kSelectorCount == 50, "there are 50 selectors");

    // NO SELECTOR IS A PATH BY ITSELF: CALL_METHOD carries an explicit operand
    // count, so a `satellite.<sel>` arity row would be unreachable, and a bare
    // selector's own one-segment name code must not find one either.
    //
    // THIS CHECK USED TO BE WIDER AND THE WIDTH WAS WRONG. It scanned every
    // segment of every row, on the reasoning that "a selector could plausibly
    // be written into a third or fourth segment". A selector can be written
    // there, and one now is — `open` is word 31 in `my_file.open()` and word 31
    // in `satellite.file.open(path, mode)`, one word with one id, because the
    // registry space is FLAT and that is what flat means. The two are still
    // distinct paths, which is the fact that matters: §7 makes the method
    // sugar for satellite.variable.file.open(my_file), {1,17,25,31}, against the
    // module function's {1,25,31,0}, and find_path keys on all four segments.
    // The old check was a true observation about the table as it stood, frozen
    // into an invariant it never was — the third time a stale generalisation in
    // this file has had to be narrowed to what it actually meant.
    for (size_t i = 0; i < kSelectorCount; i++) {
        const uint64_t id = static_cast<uint64_t>(kSelectors[i]);
        check(is_defined_word(id), "a selector is a defined word");
        const uint64_t alone[4] = {id, 0, 0, 0};
        check(find_path(alone) == nullptr, "a selector alone is not a path");
    }
}
