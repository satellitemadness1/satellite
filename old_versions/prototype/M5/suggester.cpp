// Did-You-Mean Suggester implementation -- Milestone 5.

#include "suggester.hpp"
#include "satellite_words/words_spellings.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <vector>

namespace satellite {


size_t edit_distance(std::string_view a, std::string_view b)
{
    const size_t m = a.size();
    const size_t n = b.size();
    if (m == 0) return n;
    if (n == 0) return m;

    std::vector<std::vector<size_t>> d(m + 1, std::vector<size_t>(n + 1, 0));

    for (size_t i = 0; i <= m; i++) d[i][0] = i;
    for (size_t j = 0; j <= n; j++) d[0][j] = j;

    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            const size_t cost = (std::tolower(static_cast<unsigned char>(a[i - 1])) ==
                                 std::tolower(static_cast<unsigned char>(b[j - 1]))) ? 0 : 1;

            d[i][j] = std::min({
                d[i - 1][j] + 1,      // deletion
                d[i][j - 1] + 1,      // insertion
                d[i - 1][j - 1] + cost // substitution
            });

            // Transposition (Damerau)
            if (i > 1 && j > 1 &&
                std::tolower(static_cast<unsigned char>(a[i - 1])) == std::tolower(static_cast<unsigned char>(b[j - 2])) &&
                std::tolower(static_cast<unsigned char>(a[i - 2])) == std::tolower(static_cast<unsigned char>(b[j - 1]))) {
                d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + 1);
            }
        }
    }

    return d[m][n];
}

std::vector<SuggestionMatch> rank_trie_candidates(
    words::NodeId under,
    std::string_view misspelled,
    size_t max_distance)
{
    std::vector<SuggestionMatch> matches;
    if (misspelled.empty()) return matches;

    std::unordered_set<std::string> seen;

    // 1. Direct children in words trie
    for (words::PathId c = words::first_child(under); c != words::kNoPath; c = words::next_sibling(c)) {
        const auto child_id = static_cast<words::NodeId>(c);
        std::string_view sp = words::spelling_of(child_id);
        if (sp.empty()) continue;

        std::string candidate(sp);
        if (seen.insert(candidate).second) {
            size_t dist = edit_distance(misspelled, candidate);
            if (dist <= max_distance) {
                matches.push_back({candidate, dist});
            }
        }
    }

    // 2. Aliases declared under this parent
    for (size_t i = 0; i < words::kAliasCount; i++) {
        if (words::parent_of(words::kAliases[i].of) == under) {
            std::string_view txt = words::kAliases[i].text;
            if (txt.empty()) continue;

            // Strip argument signatures if present
            size_t paren = txt.find('(');
            std::string candidate = std::string(paren == std::string_view::npos ? txt : txt.substr(0, paren));

            if (seen.insert(candidate).second) {
                size_t dist = edit_distance(misspelled, candidate);
                if (dist <= max_distance) {
                    matches.push_back({candidate, dist});
                }
            }
        }
    }

    std::sort(matches.begin(), matches.end(), [](const SuggestionMatch &a, const SuggestionMatch &b) {
        if (a.distance != b.distance) return a.distance < b.distance;
        return a.candidate < b.candidate;
    });

    return matches;
}

std::optional<std::string> suggest_trie_word(
    words::NodeId under,
    std::string_view misspelled,
    size_t max_distance)
{
    auto matches = rank_trie_candidates(under, misspelled, max_distance);
    if (!matches.empty()) {
        return matches.front().candidate;
    }
    return std::nullopt;
}

std::optional<std::string> suggest_candidate(
    std::string_view misspelled,
    const std::vector<std::string> &candidates,
    size_t max_distance)
{
    if (misspelled.empty() || candidates.empty())
        return std::nullopt;

    size_t best_dist = max_distance + 1;
    std::string best_match;

    for (const auto &cand : candidates) {
        size_t dist = edit_distance(misspelled, cand);
        if (dist < best_dist) {
            best_dist = dist;
            best_match = cand;
        }
    }

    if (best_dist <= max_distance)
        return best_match;

    return std::nullopt;
}

std::string format_trie_did_you_mean(
    std::string_view misspelled,
    std::string_view suggestion,
    std::string_view under_name)
{
    if (!under_name.empty()) {
        return "no '" + std::string(misspelled) + "' under '" + std::string(under_name) +
               "' — did you mean '" + std::string(suggestion) + "'?";
    }
    return "did you mean '" + std::string(suggestion) + "'?";
}

} // namespace satellite

