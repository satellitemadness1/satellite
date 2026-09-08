#pragma once

// WHAT IS BUILT -- PLAN M18's predicate, and the correction DESIGN §4.6 needed
// before help could be written at all.
//
// §4.6 SAID "THE TRIE **IS** WHAT EXISTS", AND THE TRIE IS WHAT IS NUMBERED.
// After M2 it holds every path in words.def, so help that walked it would
// advertise `satellite.network.https(port, cert, key)` `1 20 7` and the other
// hundred-odd paths no milestone has reached -- which is worse than the first
// satellite's help, that at least only listed what somebody had written, and
// breaks DESIGN §1.1 outright: telling a person a path works when nothing
// implements it is doing something behind their back.
//
// AND "WHAT IS IN handlers[path_id]" IS NOT THE FIX EITHER, WHICH IS THE PART
// NOBODY HAD NOTICED. Of the six paths hello world is written in, exactly one --
// `satellite.console.display` `1 5 1` -- is a row in that table. `include`,
// `capsule`, `main`, `return` and the type names are recognised by the parser
// and the resolver and are never dispatched, so a help built on the handler
// table alone prints one word of the language's own first program and stays
// silent about the other five. Found 2026-09-07, before any of this existed.
//
// SO THE PREDICATE IS FOUR THINGS AND THE FOURTH IS DERIVED FROM THE OTHERS:
//
//   a HANDLER row      -- the call runs. handlers[path_id], DESIGN §4.5.
//   an ASSIGNER row    -- the write runs. M15's dials, dispatch.hpp's Assigners.
//   a FRONT-END word   -- the front end knows it by name and nothing dispatches
//                         it. words.def's fourth list, and it is DATA because a
//                         set inferred from which spellings a parser compares
//                         against moves the first time somebody refactors a
//                         comparison, silently.
//   an ANCESTOR of one -- `satellite.console` `1 5` has no row of any kind and
//                         is plainly built, because nine words under it are. A
//                         namespace is vouched for by what it contains, so this
//                         one is DERIVED and must not be listed anywhere: two
//                         answers to one question is the drift again.
//
// §4.6'S SENTENCE THEN NEEDS ONE MORE WORD THAN IT HAD: the trie is what
// EXISTS, the handler table is what RUNS, and help answers for what is BUILT,
// which is the larger set. DESIGN §4.6 now says so and points here.
//
// COMPUTED FRESH AND NEVER CACHED. The handler table is filled by each module's
// install_handlers() at the top of a run, so a set computed once and remembered
// would be right or wrong depending on whether the first ask happened before or
// after the installs -- a bug that appears only in whichever program asks first.
// It is 264 rows of one bool and help is asked by a person; there is nothing
// here worth being clever with.

#include "satellite_words/words.hpp"

#include <array>

namespace satellite::help {

class BuiltSet {
public:
    // Walk the tables as they stand right now.
    static BuiltSet now();

    bool contains(words::PathId id) const
    {
        return id != words::kNoPath && id <= words::kNodeCount && rows_[id];
    }

    // How many of the 264 the language answers for today. `satl --words` and
    // MILESTONES/M18.md both print it, and it is the number that is supposed to
    // change every milestone.
    size_t count() const;

private:
    std::array<bool, words::kNodeCount + 1> rows_{};
};

} // namespace satellite::help
