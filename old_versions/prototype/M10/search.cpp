// The comparator: how alike are two values, and at what level.
// Layer 1 of the search power -- Milestone 10 Prototype in prototype/M10.

#include "search.hpp"
#include <algorithm>
#include <cctype>
#include <vector>

namespace satellite {

// ---------------------------------------------------------------------------
// Key Canonicalisation
// ---------------------------------------------------------------------------

bool map_key_of(const Value &v, std::string &out)
{
    if (v.is_number()) {
        out = "n" + std::get<Number>(v).to_string();
        return true;
    }
    if (v.is_string()) {
        auto str_ptr = std::get<Str>(v);
        if (str_ptr) {
            out = "s";
            out.append(reinterpret_cast<const char *>(str_ptr->data()),
                       str_ptr->size() * sizeof(SatChar));
            return true;
        }
    }
    return false;
}

namespace {

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

bool is_space(SatChar c)
{
    return c == static_cast<SatChar>(SAT_RAW_BASE + ' ') ||
           c == static_cast<SatChar>(SAT_RAW_BASE + '\t') ||
           c == static_cast<SatChar>(SAT_RAW_BASE + '\n') ||
           c == static_cast<SatChar>(SAT_RAW_BASE + '\r') ||
           c == ' ' || c == '\t' || c == '\n' || c == '\r';
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

SatString search_text(const Value &v)
{
    if (v.is_string()) {
        auto str_ptr = std::get<Str>(v);
        if (str_ptr) return *str_ptr;
    }
    return encode_raw(v.to_string());
}

bool has_prefix(const SatString &whole, const SatString &part)
{
    return part.size() <= whole.size() &&
           whole.compare(0, part.size(), part) == 0;
}

bool is_subsequence(const SatString &needle, const SatString &hay)
{
    size_t i = 0;
    for (SatChar c : hay) {
        if (i < needle.size() && needle[i] == c)
            i++;
    }
    return i == needle.size();
}

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
        if (row_best > limit)
            return false;
        previous.swap(current);
    }
    return previous[lb] <= limit;
}

int score_containers(const Value &needle, const Value &hay);

} // namespace

int search_score(const Value &needle, const Value &hay)
{
    const bool needle_box = needle.is_list() || std::holds_alternative<MapRef>(needle);
    const bool hay_box = hay.is_list() || std::holds_alternative<MapRef>(hay);
    if (needle_box || hay_box) {
        if (needle_box && hay_box)
            return score_containers(needle, hay);
        return SEARCH_NO_MATCH;
    }

    // 1 -- EXACT
    std::string nk, hk;
    if (map_key_of(needle, nk) && map_key_of(hay, hk)) {
        if (nk == hk)
            return SEARCH_EXACT;
    } else if (needle == hay) {
        return SEARCH_EXACT;
    }

    const SatString *ns = needle.is_string() && std::get<Str>(needle) ? std::get<Str>(needle).get() : nullptr;
    const SatString *hs = hay.is_string() && std::get<Str>(hay) ? std::get<Str>(hay).get() : nullptr;

    // 2 -- CASE
    if (ns && hs && folded(*ns) == folded(*hs))
        return SEARCH_CASE;

    const SatString nt = ns ? *ns : search_text(needle);
    const SatString ht = hs ? *hs : search_text(hay);
    const SatString nf = folded(nt), hf = folded(ht);

    // 3 -- CROSS-TYPE
    if (nf == hf)
        return SEARCH_CROSS_TYPE;

    // 4 -- TRIMMED
    const SatString ntr = trimmed(nf), htr = trimmed(hf);
    if (ntr == htr)
        return SEARCH_TRIMMED;

    if (ntr.empty() || htr.empty())
        return is_subsequence(ntr, htr) ? SEARCH_SUBSEQUENCE : SEARCH_NO_MATCH;

    // 5 -- PREFIX
    if (has_prefix(htr, ntr) || has_prefix(ntr, htr))
        return SEARCH_PREFIX;

    // 6 -- SUBSTRING
    if (htr.find(ntr) != SatString::npos || ntr.find(htr) != SatString::npos)
        return SEARCH_SUBSTRING;

    // 7 -- DECODED
    if (folded(encode_raw(decode(nt))) == folded(encode_raw(decode(ht))))
        return SEARCH_DECODED;

    // 8 & 9 -- TYPOS
    if (within_distance(ntr, htr, 1))
        return SEARCH_ONE_TYPO;
    if (within_distance(ntr, htr, 2))
        return SEARCH_TWO_TYPOS;

    // 10 -- SUBSEQUENCE
    if (is_subsequence(ntr, htr))
        return SEARCH_SUBSEQUENCE;

    return SEARCH_NO_MATCH;
}

namespace {

int score_containers(const Value &needle, const Value &hay)
{
    if (needle.is_list()) {
        if (!hay.is_list()) return SEARCH_NO_MATCH;
        auto nl = std::get<ListRef>(needle);
        auto hl = std::get<ListRef>(hay);
        if (!nl || !hl || nl->size() != hl->size())
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

    if (std::holds_alternative<MapRef>(needle)) {
        if (!std::holds_alternative<MapRef>(hay)) return SEARCH_NO_MATCH;
        auto nm = std::get<MapRef>(needle);
        auto hm = std::get<MapRef>(hay);
        if (!nm || !hm) return SEARCH_NO_MATCH;

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

    return SEARCH_NO_MATCH;
}

} // namespace
} // namespace satellite

