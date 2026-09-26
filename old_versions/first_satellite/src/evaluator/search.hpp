#pragma once

// The search power — a comparator, and a walker, and nothing about lists of
// maps in either of them.
//
// Part of src/evaluator/ and on the RUNTIME side of the seam eval_internal.hpp
// draws: a bytecode VM calls this unchanged. It is deliberately free of the
// Evaluator class — no fail(), no Span, no session state — so the whole power
// is testable without building a program, and so the layers below cannot start
// reaching for each other. plans/search_power.txt, DECISION 1.
//
//     the comparator   two values -> how alike are they      (search.cpp)
//     the walker       a value -> every hit inside it        (search_walk.cpp)
//     the application  what [ ... ] and .search() do with it (elsewhere)
//
// The comparator never learns what a container is. The walker never learns
// what a match is. That is what makes a new match rule one case in one file.

#include "satellite_value/value.hpp"

#include <vector>

namespace satellite {

// The ladder, tightest first. Each level is a STRICT SUPERSET of the one below
// it: a search at 6 returns everything a search at 5 returned and more, and
// that property is what makes the dial a dial rather than six unrelated modes.
//
// The numbers are the surface. satellite.system.threshold(3) is DECISION 5's
// spelling and these are the values it takes, so the enum is the language's
// own scale and not an internal encoding of it.
enum SearchLevel : int {
    SEARCH_EXACT       = 1,   // map_key_of equality: same type, same bytes
    SEARCH_CASE        = 2,   // ASCII fold. "Bolt" finds "bolt"
    SEARCH_CROSS_TYPE  = 3,   // 12 finds "12". §8.6's tag, dropped on request
    SEARCH_TRIMMED     = 4,   // outer whitespace off both sides
    SEARCH_PREFIX      = 5,   // either direction
    SEARCH_SUBSTRING   = 6,   // either direction
    SEARCH_DECODED     = 7,   // \home expands. Machine-dependent -- see below
    SEARCH_ONE_TYPO    = 8,   // edit distance <= 1
    SEARCH_TWO_TYPOS   = 9,   // edit distance <= 2, numbers by their digits
    SEARCH_SUBSEQUENCE = 10,  // every char of the needle, in order
};

inline constexpr int SEARCH_NO_MATCH = 0;
inline constexpr int SEARCH_TIGHTEST = SEARCH_EXACT;
inline constexpr int SEARCH_LOOSEST  = SEARCH_SUBSEQUENCE;

// LEVEL 7 IS NOT WHERE IT IS BY ACCIDENT. src/evaluator/maps.cpp calls decoding
// a key "the sharpest trap in the whole feature": §8.5's \home, \user and \cwd
// are LIVE codes that expand at decode() time, so a decoded comparison makes a
// match depend on the machine, on the user and on the current directory -- and
// a key that matched before a satellite.directory.change() stops matching
// after it. That is a legitimate thing to ask for and a disastrous default, so
// it is reachable at 7 and unreachable below it.

// Combining two scores, and the two ways there are to combine them. They are
// here rather than in search.cpp because both layers need them: the comparator
// combines the halves of a pair, and the walker combines the ways one entry
// could have matched. One definition, or the two drift and a hit gets a
// different rank depending on which layer found it.

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
// IT SCORES, IT DOES NOT TEST, and DECISION 2 is why. A `matches_at(a, b, n)`
// would make the superset property something ten branches maintain by hand,
// and the first level that forgot to include the one below it would be a bug
// with no symptom. A score does not depend on the threshold at all, so "raising
// the dial can only add hits" is true by construction rather than by care.
//
// It also hands back the ranking for free: results sort by score, so the exact
// hit is first even with the dial at 10, and .first() is an answer rather than
// an accident of walk order.
//
// STRICTLY STRUCTURAL. A scalar is compared to a scalar and a container to a
// container; a scalar against a container is 0. Descending is the WALKER's job,
// and a comparator that also descended would double every nested hit.
int search_score(const Value &needle, const Value &hay);

// One thing the walk found.
struct SearchHit {
    // What matched. For a map entry this is the entry's VALUE even when it was
    // the key that matched, because "find me `bolt`" means "give me what bolt
    // points at" -- which is the question the request was actually asking.
    ValuePtr value;

    // The map key `value` was stored under, or null when it was a list element
    // or the root. Kept because a hit's key is how a program says WHERE the
    // answer came from without walking the path itself.
    ValuePtr key;

    // Indices and keys from the root, in order. An ordinary satellite list, so
    // .search() can hand it straight back with no conversion -- and so the
    // result of a search is itself a structure this power can search.
    //
    // It is here for a second reason recorded in the plan: it is exactly the
    // input a write-through needs, so building the read side this way is what
    // keeps the write side from being a second traversal.
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
//
// Returns false when the structure was deeper than `max_depth`, which the
// caller reports the way every other depth failure in the evaluator is
// reported -- by naming the knob.
bool search_walk(const ValuePtr &root, const ValuePtr &pattern, int threshold,
                 int max_depth, std::vector<SearchHit> &out);

// ---------------------------------------------------------------------------
// The dial, and the domain
// ---------------------------------------------------------------------------

// satellite.system.threshold. THREAD-LOCAL, which is DECISION 5a's per-thread
// answer with no Evaluator member to carry it: a knob a program moves mid-run
// has to be read on every search, and reading it through the Library would mean
// taking the Library's lock in the one operation that runs in a loop.
//
// FREE FUNCTIONS, and that is this header's own contract being kept rather than
// a convenience. The note at the top says the power is deliberately free of the
// Evaluator class; a `threshold_` member would have made that false, and the
// class is at the 325-line ceiling besides.
int  search_threshold();
void set_search_threshold(int level);

// Every hit in `target` matching `pattern`, as the language sees them.
//
// `rich` picks the spelling (DECISION 7a): false is the subscript's -- the
// VALUES that matched -- and true is .search()'s, a map per hit carrying the
// key, the path and the score as well. Two spellings because they answer two
// questions, "what is in there" and "where is it", and a subscript that always
// answered the long one would make the short question expensive to ask.
//
// Answers a list, EMPTY when nothing matched, and that is the one place a
// search deliberately parts company with m["k"]. A lookup asserts the key is
// there and §8.6 makes a missing one an error on purpose, because nil is a
// legitimate stored value and "absent" and "present but nil" must not be one
// answer. A search asserts nothing: asking whether a structure contains
// something is the whole point, and a question that errors when the answer is
// "no" cannot be asked. .length() reads the result, as it does for any list.
//
// Null with `error` set when the structure was deeper than `max_depth`.
ValuePtr search_collect(const ValuePtr &target, const ValuePtr &pattern,
                        bool rich, int max_depth, std::string &error);

// satellite.system.threshold(n), and () to read it back. Answers 0 when the
// path is not this one, 1 when it handled the call, and -1 when it handled it
// and `error` says why it failed -- the shape a free function needs in place of
// the std::optional a module arm on the Evaluator would return.
int search_threshold_call(const std::string &full,
                          const std::vector<ValuePtr> &argv, ValuePtr &result,
                          std::string &error);

} // namespace satellite
