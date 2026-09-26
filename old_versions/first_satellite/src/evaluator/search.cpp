// The comparator: how alike are two values, and at what level.
//
// Layer one of the search power. It knows about SCALARS and about the two
// containers only as far as comparing one to another of the same kind; it does
// NOT descend, because descending is the walker's job and a comparator that
// also walked would double every nested hit. plans/search_power.txt, DECISION 1.

#include "evaluator/search.hpp"
#include "evaluator/eval_internal.hpp"

namespace satellite {
namespace {

// ---------------------------------------------------------------------------
// The alphabet
// ---------------------------------------------------------------------------

// The fold is ARITHMETIC, not a table, and satellite_string.hpp is why: the
// code table puts a..z at 1..26 and A..Z at 27..52, contiguous and in order, so
// lowering a letter is one subtraction. Inventing a Unicode fold here would be
// inventing a character set the language does not have (DECISION 3b).
//
// Everything else -- digits, punctuation, the live system codes, and the raw
// area where an unassigned byte parks -- compares as itself. When the code
// table grows a cased script, this function grows one line and nothing else
// in the power has to know.
SatChar fold(SatChar c)
{
    if (c >= SAT_UPPER_A && c <= SAT_UPPER_A + 25)
        return static_cast<SatChar>(c - 26);
    return c;
}

SatString folded(const SatString &s)
{
    SatString out;
    out.reserve(s.size());
    for (SatChar c : s)
        out.push_back(fold(c));
    return out;
}

// Space is not in the code table -- PUNCT has thirty-two characters and none of
// them is a space -- so it parks in the raw area, and so do tab, newline and
// carriage return. That is where whitespace has to be recognised, and naming it
// here keeps the one fact in one place.
bool is_space(SatChar c)
{
    return c == static_cast<SatChar>(SAT_RAW_BASE + ' ') ||
           c == static_cast<SatChar>(SAT_RAW_BASE + '\t') ||
           c == static_cast<SatChar>(SAT_RAW_BASE + '\n') ||
           c == static_cast<SatChar>(SAT_RAW_BASE + '\r');
}

SatString trimmed(const SatString &s)
{
    size_t b = 0, e = s.size();
    while (b < e && is_space(s[b]))
        b++;
    while (e > b && is_space(s[e - 1]))
        e--;
    return s.substr(b, e - b);
}

// The comparable text of a value, in the language's own codes.
//
// A string is its OWN codes and is never re-encoded: §8.5's live codes expand
// at decode() time, and going through text would make the comparison depend on
// the machine at every level rather than only at 7, which is the whole point of
// putting decoding at 7.
//
// Everything else renders through to_string() and comes back as codes.
// encode_raw puts letters, digits and punctuation in the table proper -- only
// an unassigned byte goes to the raw area -- so the digits of the number 12 and
// the characters of the string "12" are the SAME codes, which is exactly what
// level 3 needs and is why it costs nothing.
SatString search_text(const Value &v)
{
    if (const SatString *s = as_string(v))
        return *s;
    return encode_raw(to_string(v));
}

// ---------------------------------------------------------------------------
// The loose levels
// ---------------------------------------------------------------------------

bool has_prefix(const SatString &whole, const SatString &part)
{
    return part.size() <= whole.size() &&
           whole.compare(0, part.size(), part) == 0;
}

// Every char of `needle`, in order, somewhere in `hay`. The fuzzy-finder rule,
// and the honest reading of "everything remotely like it" -- level 10.
//
// An EMPTY needle is a subsequence of everything, and that is left true on
// purpose: at 10 the user asked for everything remotely alike, and an empty
// pattern is remotely like all of it. At any tighter level an empty needle
// still only matches an empty hay.
bool is_subsequence(const SatString &needle, const SatString &hay)
{
    size_t i = 0;
    for (SatChar c : hay) {
        if (i < needle.size() && needle[i] == c)
            i++;
    }
    return i == needle.size();
}

// Levenshtein, bounded before it is computed (DECISION 3c).
//
// The length check is O(1) and the matrix is O(n*m), so two strings that cannot
// possibly be within `limit` never build one. Without it, a threshold of 9 over
// a corpus of long strings is quadratic work per element for an answer that was
// always going to be no.
//
// Two rows rather than a full matrix: the distance is all that is wanted, never
// the alignment, so the other n-2 rows are never read again.
bool within_distance(const SatString &a, const SatString &b, size_t limit)
{
    const size_t la = a.size(), lb = b.size();
    if (la > lb + limit || lb > la + limit)
        return false;

    std::vector<size_t> previous(lb + 1), current(lb + 1);
    for (size_t j = 0; j <= lb; j++)
        previous[j] = j;

    for (size_t i = 1; i <= la; i++) {
        current[0] = i;
        size_t row_best = current[0];
        for (size_t j = 1; j <= lb; j++) {
            const size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            current[j] = std::min({previous[j] + 1, current[j - 1] + 1,
                                   previous[j - 1] + cost});
            row_best = std::min(row_best, current[j]);
        }
        // Nothing in this row is within the limit, and a row can only grow
        // downward, so no later row can be either.
        if (row_best > limit)
            return false;
        previous.swap(current);
    }
    return previous[lb] <= limit;
}

int score_containers(const Value &needle, const Value &hay);

} // namespace

// ---------------------------------------------------------------------------
// The ladder
// ---------------------------------------------------------------------------

int search_score(const Value &needle, const Value &hay)
{
    // A container is compared to a container and never to a scalar. The walker
    // is what reaches inside one.
    //
    // map_view() and not as_map(), so a RESULT is a container here: the result
    // of a search is a map of string keys underneath, and it was built that way
    // precisely so that it could be searched by the power that produced it with
    // no new code. This is that promise being kept, in one word.
    const bool needle_box = as_list(needle) || map_view(needle);
    const bool hay_box = as_list(hay) || map_view(hay);
    if (needle_box || hay_box) {
        if (needle_box && hay_box)
            return score_containers(needle, hay);
        return SEARCH_NO_MATCH;
    }

    // 1 -- EXACT. The map's own key contract, so what the search calls the same
    // value is what a lookup calls the same key. That is not a convenience: two
    // notions of identity over one structure would mean `m[k]` and a search at
    // level 1 could disagree about the very key they both just found.
    //
    // map_key_of covers strings and numbers -- 1 and 1.0 are one key, and 12
    // and "12" are two, both deliberately. Anything else falls to value_equals,
    // which is how a bool, a time, a bits or an object still has an exact level.
    std::string nk, hk;
    if (map_key_of(needle, nk) && map_key_of(hay, hk)) {
        if (nk == hk)
            return SEARCH_EXACT;
    } else if (value_equals(needle, hay)) {
        return SEARCH_EXACT;
    }

    const SatString *ns = as_string(needle);
    const SatString *hs = as_string(hay);

    // 2 -- CASE. Strings only: there is no case in a digit, and folding a
    // number's rendering would silently make this level do level 3's job.
    if (ns && hs && folded(*ns) == folded(*hs))
        return SEARCH_CASE;

    // Below here everything is text, so it is computed once.
    const SatString nt = ns ? *ns : search_text(needle);
    const SatString ht = hs ? *hs : search_text(hay);
    const SatString nf = folded(nt), hf = folded(ht);

    // 3 -- CROSS-TYPE. The type tag dropped on request: 12 finds "12". Reaching
    // here with equal text means the types differed, because 1 and 2 have
    // already answered every same-type equality.
    if (nf == hf)
        return SEARCH_CROSS_TYPE;

    // 4 -- TRIMMED.
    const SatString ntr = trimmed(nf), htr = trimmed(hf);
    if (ntr == htr)
        return SEARCH_TRIMMED;

    // An empty needle stops here. It is a prefix, a substring and a subsequence
    // of everything, so letting it through would make threshold 5 and above
    // return the entire structure for a pattern the user almost certainly did
    // not mean -- except at 10, where returning everything is the request.
    if (ntr.empty() || htr.empty())
        return is_subsequence(ntr, htr) ? SEARCH_SUBSEQUENCE : SEARCH_NO_MATCH;

    // 5 -- PREFIX, either direction. "bolt" finds "boltzmann", and a stored
    // "bolt" is found by a search for "boltzmann": the user asked for more and
    // not less, and one direction would make the dial care which side of the
    // comparison the longer string happened to be on.
    if (has_prefix(htr, ntr) || has_prefix(ntr, htr))
        return SEARCH_PREFIX;

    // 6 -- SUBSTRING, either direction.
    if (htr.find(ntr) != SatString::npos || ntr.find(htr) != SatString::npos)
        return SEARCH_SUBSTRING;

    // 7 -- DECODED, and see the warning in search.hpp. \home, \user and \cwd
    // expand HERE and nowhere below, which is what keeps a match machine-
    // independent at every tighter level.
    if (folded(encode_raw(decode(nt))) == folded(encode_raw(decode(ht))))
        return SEARCH_DECODED;

    // 8 and 9 -- TYPOS. Numbers are compared by their rendered digits, which is
    // one machine for both types and needs no argument about what "close"
    // means for an exact decimal: 85845 finds 85846 because the digit strings
    // are one edit apart, and no epsilon had to be invented to say so.
    if (within_distance(ntr, htr, 1))
        return SEARCH_ONE_TYPO;
    if (within_distance(ntr, htr, 2))
        return SEARCH_TWO_TYPOS;

    // 10 -- SUBSEQUENCE.
    if (is_subsequence(ntr, htr))
        return SEARCH_SUBSEQUENCE;

    return SEARCH_NO_MATCH;
}

namespace {

// A container against a container of the same kind. Recursive, and that
// recursion is the whole of DECISION 6's "a pattern nests": an element of a
// pattern that is itself a brace is scored against a container exactly as the
// top-level pattern was, so `{"outer", {"inner", 4}}` needs no special case.
//
// The score is the WORST part's, because every part had to match.
int score_containers(const Value &needle, const Value &hay)
{
    if (const List *nl = as_list(needle)) {
        const List *hl = as_list(hay);
        if (!hl || nl->size() != hl->size())
            return SEARCH_NO_MATCH;
        int worst = SEARCH_EXACT;
        for (size_t i = 0; i < nl->size(); i++) {
            if (!(*nl)[i] || !(*hl)[i])
                return SEARCH_NO_MATCH;
            const int part = search_score(*(*nl)[i], *(*hl)[i]);
            if (part == SEARCH_NO_MATCH)
                return SEARCH_NO_MATCH;
            worst = std::max(worst, part);
        }
        return worst;
    }

    // A map needle is a SUBSET question, not an equality one: every entry the
    // pattern names has to be findable, and the hay may hold others. That is
    // the reading that makes `{{"a", 1}}` a useful pattern against a map of
    // fifty entries, and equality would make it useful against exactly one map.
    const MapBody *nm = map_view(needle);
    const MapBody *hm = map_view(hay);
    if (!nm || !hm)
        return SEARCH_NO_MATCH;

    int worst = SEARCH_EXACT;
    for (const MapEntry &want : nm->entries) {
        int best = SEARCH_NO_MATCH;
        for (const MapEntry &got : hm->entries) {
            if (!want.key || !got.key || !want.value || !got.value)
                continue;
            best = score_either(best,
                                score_both(search_score(*want.key, *got.key),
                                           search_score(*want.value, *got.value)));
        }
        if (best == SEARCH_NO_MATCH)
            return SEARCH_NO_MATCH;
        worst = std::max(worst, best);
    }
    return worst;
}

} // namespace
} // namespace satellite
