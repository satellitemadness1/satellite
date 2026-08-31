#pragma once

// The FORMAT of a `satellite_config.ini` -- its keys, its units, its bounds, and
// the small text helpers over them. Private to src/machine_limits/; the door is
// limits.hpp and config.cpp is the reader.
//
// SPLIT ALONG A SUBJECT AND NOT A LINE COUNT, which is the seam
// programs/source_file.hpp was split on and the one parser_internal.hpp keeps.
// This file says what the file may CONTAIN; config.cpp says what happens when it
// contains something else. The first is a table and changes when a setting is
// added; the second is a loop and changes when a message changes.
//
// EVERYTHING HERE IS inline OR constexpr because that is what lets a header
// hold definitions at all -- and it is what lets tests/limits_test read the key
// table and the unit table directly, which is how the suite checks that a unit
// means what its name says without going through a file.

#include "error_reporter/report.hpp"
#include "error_reporter/suggest.hpp"
#include "machine_limits/limits.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace satellite::limits {

// What a value is allowed to be.
enum class Kind {
    Count,   // a whole number of things, 1 .. kMostThreads
    Size,    // a whole number of units, with the unit written down
    Dial,    // a whole number, stored and not interpreted -- see limits.hpp
};

// The largest THREAD_COUNT or CORE_COUNT this file will accept.
//
// MEASURED RATHER THAN PICKED. PLAN §4.5.1.1 timed the pool's creation at
// ~590 us for 23 threads on this machine, so a thread costs about 25.6 us to
// make -- and 1024 of them is ~26 ms, which is fifteen times satl's entire
// 1.75 ms startup (§4.3). The ceiling exists because §4.5.1.2 makes the pool
// start at startup ALWAYS: a fat-fingered `THREAD_COUNT=240000` would then
// spawn a quarter of a million threads before the first argument is read, on
// every run, including `satl --help`. There is no setting a person means by
// that, and refusing it with a caret costs them one line of the file.
inline constexpr unsigned long long kMostThreads = 1024;

struct Key {
    std::string_view name;
    Kind kind;
};

// The three SHOUTED settings, which are §4.5's own spelling and are in no
// numbering. The four dials are not here: their names come out of words.def,
// below, because a dial's spelling in this file IS its spelling in the language
// and writing it twice is how the two drift.
inline constexpr Key kSettings[] = {
    {"THREAD_COUNT", Kind::Count},
    {"CORE_COUNT", Kind::Count},
    {"MEMORY_MAX", Kind::Size},
};

inline constexpr size_t kSettingCount = sizeof kSettings / sizeof kSettings[0];

// A dial's own segment -- `min_free_mb`, not the whole path -- straight out of
// the node table.
//
// THE REGISTRY IS THE SPELLING AND THIS IS WHAT THAT BUYS. words.def already
// says `min_free_mb` is the text of node `1 14 2 3`; a second copy in this file
// would be a place for the two to disagree, and the disagreement would look
// like a config file with a typo in it that the user did not make.
inline std::string_view dial_name(DialId id)
{
    const words::NodeId node = kDialNodes[static_cast<size_t>(id)];
    return words::kNodes[static_cast<words::PathId>(node)].text;
}

// --- the units --------------------------------------------------------------
//
// BOTH FAMILIES, WITH THEIR REAL MEANINGS, WHICH IS THE WHOLE OF WHY A UNIT IS
// REQUIRED AT ALL. PLAN §4.5.4 asked what unit MEMORY_MAX is in and put the
// question in one line: "61.9 GiB and 64.9 GB are the same memory." A file that
// carried a bare number would have to pick one silently, and the person reading
// it back could not tell which was picked. So the file says.
struct Unit {
    std::string_view suffix;   // upper case; the match is case-insensitive
    unsigned long long multiplier;
};

inline constexpr unsigned long long kKi = 1024ULL;
inline constexpr unsigned long long kK = 1000ULL;

inline constexpr Unit kUnits[] = {
    {"B", 1ULL},
    {"KIB", kKi}, {"MIB", kKi * kKi}, {"GIB", kKi * kKi * kKi},
    {"TIB", kKi * kKi * kKi * kKi},
    {"KB", kK}, {"MB", kK * kK}, {"GB", kK * kK * kK},
    {"TB", kK * kK * kK * kK},
};

inline constexpr size_t kUnitCount = sizeof kUnits / sizeof kUnits[0];

// --- text -------------------------------------------------------------------

inline bool blank(char c) { return c == ' ' || c == '\t' || c == '\r'; }

// A half-open byte range of `text` with the surrounding blanks taken off. The
// OFFSETS move with it, because a caret has to land on the character that is
// wrong rather than on the space in front of it.
inline void trim(std::string_view text, size_t &start, size_t &end)
{
    while (start < end && blank(text[start]))
        start++;
    while (end > start && blank(text[end - 1]))
        end--;
}

inline errors::Span span(size_t start, size_t end, unsigned line)
{
    return errors::Span{static_cast<uint32_t>(start),
                        static_cast<uint32_t>(end),
                        static_cast<uint32_t>(line)};
}

inline char upper(char c)
{
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

inline bool same_ignoring_case(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); i++)
        if (upper(a[i]) != upper(b[i]))
            return false;
    return true;
}

// The key this name is, or kSettingCount + kDialCount for none.
// The multiplier a suffix stands for, or 0 for none. Case-insensitive, so
// `gib`, `GiB` and `GIB` are one unit -- a person typing a config file is not
// choosing between them and should not be corrected about it.
inline unsigned long long unit_multiplier(std::string_view suffix)
{
    for (size_t i = 0; i < kUnitCount; i++)
        if (same_ignoring_case(suffix, kUnits[i].suffix))
            return kUnits[i].multiplier;
    return 0;
}

inline size_t key_index(std::string_view name)
{
    for (size_t i = 0; i < kSettingCount; i++)
        if (kSettings[i].name == name)
            return i;
    for (size_t i = 0; i < kDialCount; i++)
        if (dial_name(static_cast<DialId>(i)) == name)
            return kSettingCount + i;
    return kSettingCount + kDialCount;
}

// The key a misspelling was most likely meant to be, or empty.
//
// M5's SUGGESTER, OVER A LIST THAT IS NOT THE TRIE. errors::suggest() searches
// one node's children, which is right for a path and wrong here -- the seven
// names below are not siblings in the numbering, and three of them are not in it
// at all. What is shared is the part that is actually the same fact: the
// distance and the threshold, both of which suggest.hpp exposes for exactly
// this. `THREAD_COUNTS` gets `THREAD_COUNT`, and `min_free` gets `min_free_mb`.
inline std::string_view nearest_key(std::string_view name)
{
    std::string_view best;
    size_t closest = errors::kTooFar;
    for (const std::string_view candidate : config_keys()) {
        const size_t edits = errors::distance(name, candidate);
        const size_t longest = std::max(name.size(), candidate.size());
        if (edits < closest && errors::close_enough(edits, longest)) {
            closest = edits;
            best = candidate;
        }
    }
    return best;
}

// --- values -----------------------------------------------------------------

// A run of decimal digits at `at`, as a number. False on no digits or overflow.
inline bool whole_number(std::string_view text, size_t at, size_t stop,
                  unsigned long long &into)
{
    if (at >= stop || text[at] < '0' || text[at] > '9')
        return false;
    unsigned long long value = 0;
    for (; at < stop && text[at] >= '0' && text[at] <= '9'; at++) {
        const unsigned digit = static_cast<unsigned>(text[at] - '0');
        // Refused rather than wrapped. A ceiling that silently became a small
        // number would be a watchdog that fires on a healthy process.
        if (value > (~0ULL - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return at == stop && (into = value, true);
}

} // namespace satellite::limits
