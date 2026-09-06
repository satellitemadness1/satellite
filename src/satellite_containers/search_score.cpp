// The comparator: how alike are two values, and at what level.
//
// Layer one of the search power -- v1's `evaluator/search.cpp`, ported whole.
// It knows about SCALARS, and about the two containers only as far as
// comparing one to another of the same kind; it does NOT descend, because
// descending is the walker's job and a comparator that also walked would
// double every nested hit.
//
// THE ONE STRUCTURAL CHANGE FROM v1 IS THE STACK. v1's score_containers
// recursed on the C++ stack, and a pattern nests as deep as the program that
// built it chose -- DESIGN §7.5's exact case. The container arm below is the
// same aggregation run as a task machine: a task either pushes the tasks its
// answer depends on, or folds answers already computed. search.hpp carries
// the port's two recorded changes.

#include "satellite_containers/search.hpp"

#include "satellite_string/satellite_string.hpp"
#include "satellite_value/render.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace satellite::containers {
namespace {

// ---------------------------------------------------------------------------
// The alphabet
// ---------------------------------------------------------------------------

// The fold is ARITHMETIC, not a table, and the code table is why: a..z at
// 1..26 and A..Z at 27..52, contiguous and in order, so lowering a letter is
// one subtraction. Everything else -- digits, punctuation, the live codes, the
// raw area where a space parks -- compares as itself. When the table grows a
// cased script, this function grows one line and nothing else in the power has
// to know.
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

// Space is not in the code table -- it parks in the raw area, and so do tab,
// newline and carriage return. That is where whitespace has to be recognised,
// and naming it here keeps the one fact in one place.
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
// A string is its OWN codes and is never re-encoded: DESIGN §5's live codes
// expand at decode time, and going through text would make the comparison
// depend on the machine at every level rather than only at 7, which is the
// whole point of putting decoding at 7. Everything else renders through
// text_of() and comes back as codes -- encode_raw puts letters, digits and
// punctuation in the table proper, so the digits of the number 12 and the
// characters of the string "12" are the SAME codes, which is exactly what
// level 3 needs and why it costs nothing.
SatString search_text(const Value &v)
{
    if (const Str *s = std::get_if<Str>(&v))
        return *s ? **s : SatString();
    return encode_raw(text_of(v));
}

const SatString *string_of(const Value &v)
{
    const Str *s = std::get_if<Str>(&v);
    return s && *s ? s->get() : nullptr;
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
// and the honest reading of "everything remotely like it" -- level 10. An
// EMPTY needle is a subsequence of everything, and that is left true on
// purpose: at 10 the user asked for everything remotely alike.
bool is_subsequence(const SatString &needle, const SatString &hay)
{
    size_t i = 0;
    for (SatChar c : hay)
        if (i < needle.size() && needle[i] == c)
            i++;
    return i == needle.size();
}

// Levenshtein, bounded before it is computed. The length check is O(1) and the
// matrix is O(n*m), so two strings that cannot possibly be within `limit`
// never build one. Two rows rather than a full matrix: the distance is all
// that is wanted, never the alignment.
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

// ---------------------------------------------------------------------------
// The ladder, over two scalars
// ---------------------------------------------------------------------------

int scalar_score(const Value &needle, const Value &hay)
{
    // 1 -- EXACT. The map's own key contract, so what the search calls the
    // same value is what a lookup calls the same key -- two notions of
    // identity over one structure would mean `m[k]` and a search at level 1
    // could disagree about the very key they both just found. map_key_of
    // covers strings and numbers; anything else falls to same(), which is how
    // a bool, a time or a float still has an exact level -- and same() is
    // M15's rule, so a number finds the float it equals.
    std::string nk, hk;
    if (map_key_of(needle, nk) && map_key_of(hay, hk)) {
        if (nk == hk)
            return SEARCH_EXACT;
    } else if (same(needle, hay)) {
        return SEARCH_EXACT;
    }

    const SatString *ns = string_of(needle);
    const SatString *hs = string_of(hay);

    // 2 -- CASE. Strings only: there is no case in a digit, and folding a
    // number's rendering would silently make this level do level 3's job.
    if (ns && hs && folded(*ns) == folded(*hs))
        return SEARCH_CASE;

    // Below here everything is text, so it is computed once.
    const SatString nt = ns ? *ns : search_text(needle);
    const SatString ht = hs ? *hs : search_text(hay);
    const SatString nf = folded(nt), hf = folded(ht);

    // 3 -- CROSS-TYPE. The type tag dropped on request: 12 finds "12".
    // Reaching here with equal text means the types differed, because 1 and 2
    // have already answered every same-type equality.
    if (nf == hf)
        return SEARCH_CROSS_TYPE;

    // 4 -- TRIMMED.
    const SatString ntr = trimmed(nf), htr = trimmed(hf);
    if (ntr == htr)
        return SEARCH_TRIMMED;

    // An empty needle stops here. It is a prefix, a substring and a
    // subsequence of everything, so letting it through would make threshold 5
    // and above return the entire structure for a pattern the user almost
    // certainly did not mean -- except at 10, where returning everything is
    // the request.
    if (ntr.empty() || htr.empty())
        return is_subsequence(ntr, htr) ? SEARCH_SUBSEQUENCE : SEARCH_NO_MATCH;

    // 5 -- PREFIX, either direction: the user asked for more and not less, and
    // one direction would make the dial care which side of the comparison the
    // longer string happened to be on.
    if (has_prefix(htr, ntr) || has_prefix(ntr, htr))
        return SEARCH_PREFIX;

    // 6 -- SUBSTRING, either direction.
    if (htr.find(ntr) != SatString::npos || ntr.find(htr) != SatString::npos)
        return SEARCH_SUBSTRING;

    // 7 -- DECODED, and see the warning in search.hpp. The live codes expand
    // HERE and nowhere below, which is what keeps a match machine-independent
    // at every tighter level. live_text is the same six answers the console
    // prints with.
    if (folded(encode_raw(live_text(nt))) == folded(encode_raw(live_text(ht))))
        return SEARCH_DECODED;

    // 8 and 9 -- TYPOS. Numbers are compared by their rendered digits: one
    // machine for both types, and no epsilon had to be invented to say that
    // 85845 finds 85846.
    if (within_distance(ntr, htr, 1))
        return SEARCH_ONE_TYPO;
    if (within_distance(ntr, htr, 2))
        return SEARCH_TWO_TYPOS;

    // 10 -- SUBSEQUENCE.
    if (is_subsequence(ntr, htr))
        return SEARCH_SUBSEQUENCE;

    return SEARCH_NO_MATCH;
}

// ---------------------------------------------------------------------------
// Containers, on a task stack
// ---------------------------------------------------------------------------

// A container against a container of the same kind. The aggregation is v1's
// exactly -- a list pair is elementwise and the score is the WORST part's,
// because every part had to match; a map needle is a SUBSET question, every
// entry it names findable at the best combination its key and value reach.
// What changed is only where the pending work lives.
struct Task {
    enum Kind : uint8_t {
        Score,    // score (a, b); push one answer
        ListFold, // pop element i's score, fold into acc, continue the list
        WantStep, // pop one hay entry's combined score, fold, continue
        Both,     // pop two answers, push score_both of them
    } kind;
    const Value *a = nullptr;
    const Value *b = nullptr;
    const List *nl = nullptr;
    const List *hl = nullptr;
    const MapBody *nm = nullptr;
    const MapBody *hm = nullptr;
    size_t i = 0;   // ListFold: element just scored. WantStep: hay entry.
    size_t want = 0; // WantStep: the needle entry being sought
    int acc = SEARCH_EXACT; // ListFold: worst so far. WantStep: worst so far.
    int best = SEARCH_NO_MATCH; // WantStep: best for this want entry so far
};

int container_score(const Value &needle, const Value &hay)
{
    std::vector<Task> tasks;
    std::vector<int> answers;
    tasks.push_back({Task::Score, &needle, &hay, nullptr, nullptr, nullptr,
                     nullptr, 0, 0, SEARCH_EXACT, SEARCH_NO_MATCH});

    // One want entry's next hay comparison: the two halves, combined by Both,
    // folded by WantStep. Pushed continuation-first so the halves run first.
    const auto seek = [&tasks](Task fold) {
        const MapEntry &want = fold.nm->entries[fold.want];
        const MapEntry &got = fold.hm->entries[fold.i];
        tasks.push_back(fold);
        tasks.push_back({Task::Both, nullptr, nullptr, nullptr, nullptr,
                         nullptr, nullptr, 0, 0, 0, 0});
        tasks.push_back({Task::Score, &want.value, &got.value, nullptr,
                         nullptr, nullptr, nullptr, 0, 0, 0, 0});
        tasks.push_back({Task::Score, &want.key, &got.key, nullptr, nullptr,
                         nullptr, nullptr, 0, 0, 0, 0});
    };

    while (!tasks.empty()) {
        Task task = tasks.back();
        tasks.pop_back();

        switch (task.kind) {
        case Task::Score: {
            const Value &a = *task.a;
            const Value &b = *task.b;
            const bool boxes = (a.is_list() || a.is_map()) &&
                               (b.is_list() || b.is_map());
            if (!boxes) {
                answers.push_back((a.is_list() || a.is_map() || b.is_list() ||
                                   b.is_map())
                                      ? SEARCH_NO_MATCH
                                      : scalar_score(a, b));
                break;
            }
            if (const List *nl = as_list(a)) {
                const List *hl = as_list(b);
                if (!hl || nl->size() != hl->size()) {
                    answers.push_back(SEARCH_NO_MATCH);
                    break;
                }
                if (nl->empty()) {
                    answers.push_back(SEARCH_EXACT);
                    break;
                }
                tasks.push_back({Task::ListFold, nullptr, nullptr, nl, hl,
                                 nullptr, nullptr, 0, 0, SEARCH_EXACT,
                                 SEARCH_NO_MATCH});
                tasks.push_back({Task::Score, &(*nl)[0], &(*hl)[0], nullptr,
                                 nullptr, nullptr, nullptr, 0, 0, 0, 0});
                break;
            }
            const MapBody *nm = as_map(a);
            const MapBody *hm = as_map(b);
            if (!nm || !hm) {
                answers.push_back(SEARCH_NO_MATCH);
                break;
            }
            if (nm->entries.empty()) {
                answers.push_back(SEARCH_EXACT);
                break;
            }
            if (hm->entries.empty()) {
                // Every want entry has nowhere to be found.
                answers.push_back(SEARCH_NO_MATCH);
                break;
            }
            seek({Task::WantStep, nullptr, nullptr, nullptr, nullptr, nm, hm,
                  0, 0, SEARCH_EXACT, SEARCH_NO_MATCH});
            break;
        }

        case Task::ListFold: {
            const int part = answers.back();
            answers.pop_back();
            if (part == SEARCH_NO_MATCH) {
                answers.push_back(SEARCH_NO_MATCH);
                break;
            }
            task.acc = std::max(task.acc, part);
            if (++task.i < task.nl->size()) {
                const size_t at = task.i;
                tasks.push_back(task);
                tasks.push_back({Task::Score, &(*task.nl)[at],
                                 &(*task.hl)[at], nullptr, nullptr, nullptr,
                                 nullptr, 0, 0, 0, 0});
                break;
            }
            answers.push_back(task.acc);
            break;
        }

        case Task::WantStep: {
            const int combined = answers.back();
            answers.pop_back();
            task.best = score_either(task.best, combined);
            if (++task.i < task.hm->entries.size()) {
                seek(task);
                break;
            }
            // This want entry has met every hay entry.
            if (task.best == SEARCH_NO_MATCH) {
                answers.push_back(SEARCH_NO_MATCH);
                break;
            }
            task.acc = std::max(task.acc, task.best);
            if (++task.want < task.nm->entries.size()) {
                task.i = 0;
                task.best = SEARCH_NO_MATCH;
                seek(task);
                break;
            }
            answers.push_back(task.acc);
            break;
        }

        case Task::Both: {
            const int second = answers.back();
            answers.pop_back();
            const int first = answers.back();
            answers.pop_back();
            answers.push_back(score_both(first, second));
            break;
        }
        }
    }
    return answers.back();
}

} // namespace

int search_score(const Value &needle, const Value &hay)
{
    const bool needle_box = needle.is_list() || needle.is_map();
    const bool hay_box = hay.is_list() || hay.is_map();
    if (needle_box || hay_box) {
        if (needle_box && hay_box)
            return container_score(needle, hay);
        return SEARCH_NO_MATCH;
    }
    return scalar_score(needle, hay);
}

} // namespace satellite::containers
