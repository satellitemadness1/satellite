#pragma once

// The search power -- a comparator, and a walker, and nothing about lists of
// maps in either of them. v1's `evaluator/search.hpp`, ported at M16 with two
// deliberate changes recorded below.
//
// MACHINE-FREE, which is v1's own contract kept one tree over: no Machine, no
// Span, no refuse() -- the whole power is testable without building a program,
// and the layers cannot start reaching for each other.
//
//     the comparator   two values -> how alike are they      (search_score.cpp)
//     the walker       a value -> every hit inside it        (search_walk.cpp)
//     the application  what [ ... ] and .search() do with it (the callers)
//
// The comparator never learns what a container is beyond comparing one to
// another of the same kind. The walker never learns what a match is. That is
// what makes a new match rule one case in one file.
//
// THE TWO CHANGES FROM v1, AND WHY:
//
//   1. THERE IS NO max_depth AND NO "too deep" ERROR. v1 bounded the walk and
//      reported the knob; the 2026-09-01 rule (DESIGN §7.5) is that the
//      language has no depth limit and `max_depth` `1 14 2 2` is a BYTE
//      ceiling on the control stack, which a count of levels cannot read.
//      PLAN §8's M16 entry closes it: "M16 reads no dial ... if it is a
//      walker of its own, it keeps its own stack the way M8.5's four do and
//      is bounded by memory like everything else." It is a walker of its own,
//      both walks below keep their stacks on the heap, and a search that
//      cannot finish is a program that ran out of memory -- an event the
//      language already has words and an exit status for.
//
//   2. THE THRESHOLD IS THE MACHINE'S AND NOT A thread_local. v1's DECISION
//      5a made the dial per-thread; this tree's Policy note says why nothing
//      here is process-wide -- M22 runs many programs in one process -- and
//      until M23 there are no threads for a thread_local to separate. The
//      dial rides on the Machine (search_threshold), the walk takes it as an
//      argument, and M23 inherits a per-machine dial it can split per-thread
//      with the argument already in place.

#include "satellite_value/value.hpp"

#include <vector>

namespace satellite::containers {

// The ladder, tightest first. Each level is a STRICT SUPERSET of the one below
// it: a search at 6 returns everything a search at 5 returned and more, and
// that property is what makes the dial a dial rather than six unrelated modes.
//
// The numbers are the surface. `satellite.system.threshold(3)` `1 22 6` is the
// spelling and these are the values it takes, so the enum is the language's
// own scale and not an internal encoding of it.
enum SearchLevel : int {
    SEARCH_EXACT       = 1,  // map_key_of equality: same type, same bytes
    SEARCH_CASE        = 2,  // fold over the code table. "Bolt" finds "bolt"
    SEARCH_CROSS_TYPE  = 3,  // 12 finds "12" -- the type tag dropped on request
    SEARCH_TRIMMED     = 4,  // outer whitespace off both sides
    SEARCH_PREFIX      = 5,  // either direction
    SEARCH_SUBSTRING   = 6,  // either direction
    SEARCH_DECODED     = 7,  // \home expands. Machine-dependent -- see below
    SEARCH_ONE_TYPO    = 8,  // edit distance <= 1
    SEARCH_TWO_TYPOS   = 9,  // edit distance <= 2, numbers by their digits
    SEARCH_SUBSEQUENCE = 10, // every char of the needle, in order
};

inline constexpr int SEARCH_NO_MATCH = 0;
inline constexpr int SEARCH_TIGHTEST = SEARCH_EXACT;
inline constexpr int SEARCH_LOOSEST  = SEARCH_SUBSEQUENCE;

// LEVEL 7 IS NOT WHERE IT IS BY ACCIDENT. map_key_of's note calls decoding a
// key "the sharpest trap in the whole feature": DESIGN §5's live codes expand
// at decode time, so a decoded comparison makes a match depend on the machine,
// on the user and on the current directory -- and a key that matched before a
// directory change stops matching after it. That is a legitimate thing to ask
// for and a disastrous default, so it is reachable at 7 and unreachable below.

// Combining two scores, and the two ways there are to combine them. Both
// layers need them -- the comparator combines the halves of a pair, the walker
// combines the ways one entry could have matched -- so they live here: one
// definition, or the two drift and a hit gets a different rank depending on
// which layer found it.

// Both must match: a pair is only as tight as its looser half.
inline int score_both(int a, int b)
{
    if (a == SEARCH_NO_MATCH || b == SEARCH_NO_MATCH)
        return SEARCH_NO_MATCH;
    return a > b ? a : b;
}

// Either will do: take the tighter of the two that matched.
inline int score_either(int a, int b)
{
    if (a == SEARCH_NO_MATCH)
        return b;
    if (b == SEARCH_NO_MATCH)
        return a;
    return a < b ? a : b;
}

// The TIGHTEST level at which needle and hay match, or SEARCH_NO_MATCH.
//
// IT SCORES, IT DOES NOT TEST -- v1's DECISION 2, kept. A `matches_at(a, b,
// n)` would make the superset property something ten branches maintain by
// hand, and the first level that forgot to include the one below it would be a
// bug with no symptom. A score does not depend on the threshold at all, so
// "raising the dial can only add hits" is true by construction. It also hands
// back the ranking for free: results sort by score, so the exact hit is first
// even with the dial at 10.
//
// STRICTLY STRUCTURAL. A scalar is compared to a scalar and a container to a
// container; a scalar against a container is 0. Descending is the WALKER's
// job, and a comparator that also descended would double every nested hit.
// Nested container-against-container scoring keeps its own stack (change 1).
int search_score(const Value &needle, const Value &hay);

// One thing the walk found.
struct SearchHit {
    // What matched. For a map entry this is the entry's VALUE even when it was
    // the key that matched, because "find me `bolt`" means "give me what bolt
    // points at" -- the question the request was actually asking.
    Value value;

    // The map key `value` was stored under, or nothing for a list element or
    // the root. Kept because a hit's key is how a program says WHERE the
    // answer came from without walking the path itself.
    Value key;

    // Indices and keys from the root, in order. An ordinary satellite list, so
    // the rich spelling hands it straight back -- and so the result of a
    // search is itself a structure this power can search.
    List path;

    int score = 0;
};

// Every hit in `root` scoring at or tighter than `threshold`, appended to
// `out`, best score first and walk order breaking ties.
//
// `pattern` is what the user wrote between the brackets, already evaluated. A
// two-element list is an ENTRY pattern -- key then value -- because two is the
// shape a map entry has and it is the shape that was asked for; it is ALSO
// tried as an ordinary value, so a two-element list nested in a list of lists
// still finds itself. More, not less: that is the whole instruction.
void search_walk(const Value &root, const Value &pattern, int threshold,
                 std::vector<SearchHit> &out);

// Every hit in `target` matching `pattern`, as the language sees them.
//
// `rich` picks the spelling -- v1's DECISION 7a, and WORD_NUMBERS §2.6 is
// where this tree minted its numbers: false is the subscript's answer, the
// VALUES that matched, and true is `.search(pattern)`'s `1 4 1 10` /
// `1 4 2 26`, a map per hit carrying the key, the path and the score as well.
// Two spellings because they answer two questions -- "what is in there" and
// "where is it" -- and a subscript that always answered the long one would
// make the short question expensive to ask.
//
// Answers a list, EMPTY when nothing matched, and that is the one place a
// search deliberately parts company with `m[k]`. A lookup asserts the key is
// there (S0726, and `.has()` is how to ask first); a search asserts nothing --
// asking whether a structure contains something is the whole point, and a
// question that errors when the answer is "no" cannot be asked.
Value search_collect(const Value &target, const Value &pattern, bool rich,
                     int threshold);

} // namespace satellite::containers
