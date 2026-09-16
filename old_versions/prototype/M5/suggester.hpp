#pragma once

// Did-You-Mean Suggester -- Milestone 5 Error Reporter.
//
// Computes Damerau-Levenshtein edit distance and suggests intended words
// over the trie level that failed or candidate lists.
//
// DESIGN §4.6: "A failed walk knows which segment failed and which node it
// failed under, so satellite.consle.display answers 'no consle under
// satellite -- did you mean console?' by running edit distance over that
// one node's children."

#include "satellite_words/words.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

struct SuggestionMatch {
    std::string candidate;
    size_t distance = 0;
};

// Damerau-Levenshtein distance (insertion, deletion, substitution, transposition).
size_t edit_distance(std::string_view a, std::string_view b);

// Finds the closest child or alias under `under` node in M2's words trie.
std::optional<std::string> suggest_trie_word(
    words::NodeId under,
    std::string_view misspelled,
    size_t max_distance = 3);

// Finds all candidate children under `under` within max_distance, sorted by similarity.
std::vector<SuggestionMatch> rank_trie_candidates(
    words::NodeId under,
    std::string_view misspelled,
    size_t max_distance = 3);

// Finds the closest match from an arbitrary candidate list (e.g. keywords, types, file modes).
std::optional<std::string> suggest_candidate(
    std::string_view misspelled,
    const std::vector<std::string> &candidates,
    size_t max_distance = 3);

// Formats a standard "did you mean" sentence.
std::string format_trie_did_you_mean(
    std::string_view misspelled,
    std::string_view suggestion,
    std::string_view under_name = "");

} // namespace satellite

