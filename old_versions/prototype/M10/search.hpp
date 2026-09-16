#pragma once

// The search power -- a comparator, a walker, and a dial.
// Milestone 10 Prototype in prototype/M10.
//
// Layer 1: the comparator   two values -> similarity score (search.cpp)
// Layer 2: the walker       a value -> all matches inside it (search_walk.cpp)
// Layer 3: the application  search_collect, threshold dials (search_apply.cpp)

#include "value.hpp"
#include <string>
#include <vector>

namespace satellite {

// The 10-level search ladder, tightest first.
// Each level is a strict superset of the one below it.
enum SearchLevel : int {
    SEARCH_EXACT       = 1,   // exact value / key equality
    SEARCH_CASE        = 2,   // ASCII case fold ("Bolt" finds "bolt")
    SEARCH_CROSS_TYPE  = 3,   // 12 finds "12"
    SEARCH_TRIMMED     = 4,   // outer whitespace stripped
    SEARCH_PREFIX      = 5,   // prefix match in either direction
    SEARCH_SUBSTRING   = 6,   // substring match in either direction
    SEARCH_DECODED     = 7,   // live system codes decoded
    SEARCH_ONE_TYPO    = 8,   // Levenshtein edit distance <= 1
    SEARCH_TWO_TYPOS   = 9,   // Levenshtein edit distance <= 2
    SEARCH_SUBSEQUENCE = 10,  // fuzzy subsequence matching
};

inline constexpr int SEARCH_NO_MATCH = 0;
inline constexpr int SEARCH_TIGHTEST = SEARCH_EXACT;
inline constexpr int SEARCH_LOOSEST  = SEARCH_SUBSEQUENCE;

inline int score_both(int a, int b)
{
    if (a == SEARCH_NO_MATCH || b == SEARCH_NO_MATCH)
        return SEARCH_NO_MATCH;
    return a > b ? a : b;
}

inline int score_either(int a, int b)
{
    if (a == SEARCH_NO_MATCH)
        return b;
    if (b == SEARCH_NO_MATCH)
        return a;
    return a < b ? a : b;
}

// Scores similarity between needle and hay according to the 10-level ladder.
int search_score(const Value &needle, const Value &hay);

// Canonical map key helper
bool map_key_of(const Value &v, std::string &out);

struct SearchHit {
    ValuePtr value;
    ValuePtr key;
    List path;
    int score = 0;
};

// Traverses root recursively and collects all hits scoring <= threshold.
bool search_walk(const ValuePtr &root, const ValuePtr &pattern, int threshold,
                 int max_depth, std::vector<SearchHit> &out);

// Thread-local threshold management (default SEARCH_EXACT = 1)
int search_threshold();
void set_search_threshold(int level);

// Collects search hits formatted into a Satellite list
ValuePtr search_collect(const ValuePtr &target, const ValuePtr &pattern,
                        bool rich, int max_depth, std::string &error);

} // namespace satellite

