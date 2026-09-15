// MILESTONES/M4.5.md §5's clause, counted -- "M7's resolve has to learn to skip
// a path the `.satc` has already numbered before the cache pays for itself".
//
// COUNTED AND NOT TIMED, WHICH IS A CORRECTION TO THE DRAFT DONE-WHEN. The
// version of that clause offered at the start of this milestone read "a warm
// hit beats `--unparse` on hello_world", and that is not a comparison the two
// commands can lose or win: `--unparse` does not resolve anything, so it is not
// doing the work the skip saves. What M4.5 §5 actually measured is that a warm
// hit reads a LARGER file, runs the substitution back, and then lexes and
// parses exactly as `--unparse` does -- and on a 273-byte program the walk it
// saves is far under what a shell loop can see. A count is exact where a
// millisecond is not, and it is the only thing that can catch the skip
// disappearing: a resolver that stopped reading the marks would still produce
// every correct number, because the walk is the fallback and the fallback works.
//
// THE STRONGER HALF IS THAT THE ANSWERS AGREE. A skip that took the wrong
// number would be a cache that changes what a program MEANS, which is the
// failure SATC §2's digest exists for one level down. So every assertion below
// checks the numbers first and the counts second.

#include "resolve_test.hpp"

#include "name_resolver/resolve.hpp"
#include "satellite_cache/cache.hpp"

#include <string>
#include <vector>

namespace resolve_test {

namespace {

const std::string kProgram = R"(
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("hello")
    satellite.console.input(">>>", arguments)
    satellite.return(satellite)
}
)";

// M19.6's fixture. All THREE fold shapes on three lines, which
// example/containers.satl also has and for the same reason: they take three
// different routes through fold_option() and a test with only the middle one
// would pass with the other two broken.
//
//   sort()          no option at all -- an ordinary selector, still walked for
//   sort("down")    folds to sort_down() 1 4 2 5, the row the option names
//   sort("up")      folds onto the BARE sort() 1 4 2 3, because sort_up() does
//                   not exist and WORD_NUMBERS §1.5 lets a fold land on the
//                   word it was spelled from -- M16's rule
const std::string kOptions = R"(
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.sort()
    l.sort("down")
    l.sort("up")
    satellite.return(satellite)
}
)";

} // namespace

// THE OPTION TOKEN, COLD AND WARM -- M19.6. SATC.md §5.1 used to forbid this
// file from saying anything about a fold; the write now happens after resolve
// and `0#down` is what the move made writable.
void section_option_token()
{
    using satellite::resolve::Origin;

    Run cold;
    resolve_source(kOptions, cold);
    check(cold.parsed_clean() && cold.resolved.ok(),
          "the option fixture resolves");
    check(resolved_to(cold, "1 4 2 5"),
          "cold: sort(\"down\") folds to sort_down() 1 4 2 5");
    check(origin_of(cold, "1 4 2 5") == Origin::Walked,
          "cold: and it was WALKED for, because a source says only `down`");

    // THE FILE SAYS IT NOW, WHICH IS THE MILESTONE. Written from the resolved
    // tree -- the fourth argument is the whole of the change to this call.
    const satellite::cache::Source stamp{"options.satl", 1, kOptions.size()};
    const std::string satc = satellite::cache::satc_text(
        cold.parsed.ast, cold.words, stamp,
        satellite::resolve::folds_of(cold.resolved));

    check(satc.find("l.sort(0#down)") != std::string::npos,
          "the `.satc` carries the option token: " + satc);
    check(satc.find("l.sort(0#up)") != std::string::npos,
          "and the one whose fold lands on the bare word carries it too");
    check(satc.find("l.sort()\n") != std::string::npos ||
              satc.find("l.sort()") != std::string::npos,
          "and a call with no option is untouched");
    check(satc.find("\"down\"") == std::string::npos,
          "the option is no longer written as a string -- §3 gains a row "
          "rather than losing one");

    // WARM: read it back and resolve with what the file said.
    satellite::words::Words warm_words;
    const satellite::cache::Reading found =
        satellite::cache::read_text(satc, stamp, warm_words);
    check(found.hit(), "a `.satc` with an option token in it reads back");
    check(found.folded.size() == 2,
          "and it hands back the two selectors it said were folded, got " +
              std::to_string(found.folded.size()));

    const satellite::resolve::Resolved warm = satellite::resolve::resolve(
        found.program.ast, warm_words, found.marks, found.folded);
    check(warm.ok(), "and it resolves clean");

    Run warm_run;
    warm_run.parsed = found.program;
    warm_run.resolved = warm;

    // THE ANSWER FIRST, WHICH IS THE ORDER THIS FILE ALREADY KEEPS. A skip that
    // reached a different row would be a cache that changes what a program
    // does -- and `sort_down` against `sort` is exactly the pair where that
    // would reverse a list rather than raise anything.
    check(resolved_to(warm_run, "1 4 2 5"),
          "warm: the same fold, 1 4 2 5 -- PLAN's done-when for this milestone");
    check(origin_of(warm_run, "1 4 2 5") == Origin::Cached,
          "warm: and it came FROM the file, without the resolver deciding "
          "again that `down` is an option");
    check(warm.from_cache > cold.resolved.from_cache,
          "and the warm run takes more from the file than the cold one, which "
          "took nothing");

    // AND THE THIRD SHAPE, WHICH IS THE ONE THAT NEEDS TWO LOOKUPS. `sort_up`
    // is not a row; the token says `up` is an option and the bare retry is what
    // finds `sort()`. Asserted separately because a cached branch that only
    // tried the folded spelling would fall through here and still be right --
    // silently paying for the decision this milestone removed.
    // COUNTED AND NOT ASKED FIRST-MATCH, because BOTH `l.sort()` and
    // `l.sort(0#up)` land on `1 4 2 3` in this fixture and they must arrive
    // there by different routes: the bare call has no token and is walked for,
    // the folded one is taken from the file. origin_of() answers about
    // whichever comes first and cannot tell them apart.
    check(count_origin(warm_run, "1 4 2 3", Origin::Cached) == 1,
          "warm: sort(0#up) lands on the bare sort() 1 4 2 3 FROM the file -- "
          "M16's bare-word rule reached through the token rather than "
          "re-derived");
    check(count_origin(warm_run, "1 4 2 3", Origin::Walked) == 1,
          "warm: and the sort() that had no option is still walked for, which "
          "is what says the token did the work and not the number");
}

void section_cache()
{
    using satellite::resolve::Origin;

    // COLD: the source, walked.
    Run cold;
    resolve_source(kProgram, cold);
    check(cold.parsed_clean() && cold.resolved.ok(), "the cache fixture resolves");
    check(cold.resolved.from_cache == 0,
          "a program read from source takes nothing from a `.satc`");
    check(cold.resolved.walked > 0, "and walks for every path it names");

    // WARM: the same program, through the `.satc` writer and reader.
    const satellite::cache::Source stamp{"cache_fixture.satl", 1, kProgram.size()};
    const std::string satc =
        satellite::cache::satc_text(cold.parsed.ast, cold.words, stamp);

    satellite::words::Words warm_words;
    const satellite::cache::Reading found =
        satellite::cache::read_text(satc, stamp, warm_words);
    check(found.hit(), "the `.satc` this build wrote is one it reads back");
    check(!found.marks.empty(),
          "and it comes back with the paths it had already numbered -- which is "
          "the half M4.5 §5 says the reader had nowhere to put");

    const satellite::resolve::Resolved warm =
        satellite::resolve::resolve(found.program.ast, warm_words, found.marks);
    check(warm.ok(), "and it resolves clean");

    // THE ANSWERS FIRST. Both runs must arrive at the same numbers, or the skip
    // is a cache that changes what a program means.
    Run warm_run;
    warm_run.parsed = found.program;
    warm_run.resolved = warm;
    std::vector<std::string> cold_numbers = numbers_of(cold);
    std::vector<std::string> warm_numbers = numbers_of(warm_run);
    check(cold_numbers.size() == warm_numbers.size(),
          "the warm run resolves the same number of paths as the cold one");
    for (const std::string &number : {std::string("1 5 1"), std::string("1 5 4"),
                                      std::string("1 4 2"), std::string("1 6 1"),
                                      std::string("1 1 1"), std::string("1 15 1")}) {
        check(resolved_to(cold, number),
              "cold: the fixture resolves something to " + number);
        check(resolved_to(warm_run, number),
              "warm: and the `.satc` agrees, for " + number);
    }

    // AND WHICH OF THEM WERE SKIPPED FOR, ONE AT A TIME. There are two places
    // a mark can be read -- a postfix chain and a TYPE -- and they are separate
    // branches of the resolver. Asserting the totals alone lets either one be
    // deleted without a failure, because the other still moves the count in the
    // right direction and the sums still add up. Found by mutation on
    // 2026-08-31: disabling the chain half walked through every assertion this
    // section had.
    check(origin_of(cold, "1 5 1") == Origin::Walked,
          "cold: satellite.console.display 1 5 1 was walked for");
    check(origin_of(warm_run, "1 5 1") == Origin::Cached,
          "warm: and taken from the `.satc` -- the CHAIN half of the skip");
    check(origin_of(cold, "1 4 2") == Origin::Walked,
          "cold: satellite.container.list 1 4 2 was walked for");
    check(origin_of(warm_run, "1 4 2") == Origin::Cached,
          "warm: and taken from the `.satc` -- the TYPE half, which is a "
          "different branch and has to be asserted separately");
    check(origin_of(warm_run, "1 5 4") == Origin::Cached,
          "warm: and a CALL SHAPE too -- input(prompt, target) 1 5 4, where the "
          "number covers the Call node and the mark ends at the word, which is "
          "the case the two offsets do not line up in");

    // AND A STATEMENT FORM IS STILL WALKED FOR, WHICH IS SAID RATHER THAN LEFT
    // TO BE DISCOVERED. `satellite.include(satellite)` is an Include node
    // anchored at the RESERVED WORD, not at `include`, so the mark's offset
    // belongs to no node's anchor and the skip cannot reach it.
    // MILESTONES/M7.md §6 carries what closing that would cost.
    check(origin_of(warm_run, "1 1 1") == Origin::Walked,
          "warm: satellite.include(satellite) is still walked for -- a "
          "statement form is anchored at `satellite` and the mark is not");

    // THE COUNTS SECOND, AND THIS IS THE CLAUSE.
    check(warm.from_cache > 0,
          "a warm run takes paths from the `.satc` -- MILESTONES/M4.5.md §5's "
          "clause, which had no implementation until this milestone");
    check(warm.walked < cold.resolved.walked,
          "and it walks for FEWER of them than the cold run did: " +
              std::to_string(warm.walked) + " against " +
              std::to_string(cold.resolved.walked));
    check(warm.walked + warm.from_cache == cold.resolved.walked,
          "and every walk it saved is one the file already knew -- the two "
          "numbers add up to the cold run's, which is what says the skip took "
          "paths out of the walk rather than adding work beside it");

    // AND THE MARKS ARE ORDERED, which is what lets the lookup bisect rather
    // than scan. unnumber() writes them as it substitutes, so they ascend by
    // construction; asserting it is how that stays true if the pass is ever
    // rewritten to look ahead.
    bool ascending = true;
    for (size_t i = 1; i < found.marks.size(); i++)
        if (found.marks[i].ends <= found.marks[i - 1].ends)
            ascending = false;
    check(ascending, "the marks ascend, which is what the binary search assumes");

    // A `.satc` FROM A DIFFERENT SOURCE IS A MISS AND CARRIES NO MARKS, which
    // is the same promise §4 makes about the tree: a miss costs one walk and
    // leaves nothing behind.
    satellite::words::Words stale_words;
    const satellite::cache::Source moved{"cache_fixture.satl", 2, kProgram.size()};
    const satellite::cache::Reading missed =
        satellite::cache::read_text(satc, moved, stale_words);
    check(!missed.hit(), "a `.satc` whose source moved on is a miss");
    check(missed.marks.empty(),
          "and it hands back no marks, so nothing can be skipped on the "
          "strength of a file that was not believed");
}

} // namespace resolve_test
