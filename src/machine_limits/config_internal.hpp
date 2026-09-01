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
#include <string>
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
    Fact fact;             // the machine's own answer for this setting
    words::NodeId spells;  // and the path a file writes to ask for it
};

// The three SHOUTED settings, which are §4.5's own spelling and are in no
// numbering. The four dials are not here: their names come out of words.def,
// below, because a dial's spelling in this file IS its spelling in the language
// and writing it twice is how the two drift.
//
// THE THIRD AND FOURTH COLUMNS ARE DESIGN §7.7's PAIRING, WRITTEN DOWN ONCE.
// That section says it as plainly as it can be said: "arguments.machine.threads,
// arguments.machine.cores and arguments.memory.total are the same numbers as
// THREAD_COUNT, CORE_COUNT and MEMORY_MAX in the configuration ... Three ways to
// ask, one place that knows." One fact each, and the path is a NodeId rather
// than a string for the reason dial_name() gives one line down: words.def is
// the spelling, and a second copy here is a place for the two to disagree.
inline constexpr Key kSettings[] = {
    {"THREAD_COUNT", Kind::Count, Fact::Threads,
     words::NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_THREADS},
    {"CORE_COUNT", Kind::Count, Fact::Cores,
     words::NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_CORES},
    {"MEMORY_MAX", Kind::Size, Fact::MemoryTotal,
     words::NodeId::LIBRARY_MAIN_ARGUMENTS_MEMORY_TOTAL},
};

inline constexpr size_t kSettingCount = sizeof kSettings / sizeof kSettings[0];

// What a dial's value may be, in DialId order -- and `checked` is false for
// every dial whose meaning has not been decided yet.
//
// A RANGE IS A CLAIM ABOUT WHAT A VALUE MEANS, WHICH IS WHY THIS TABLE IS
// MOSTLY EMPTY. `float_digits` waits on M15, and `min_free_mb` is M6's own and
// has a meaning with no bound ever claimed for it: any number of megabytes is a
// number of megabytes, and unset means the machine's free memory is not watched
// at all.
//
// `max_depth`'s ROW IS UNCHECKED AND ITS MEANING IS DECIDED, WHICH IS NOT A
// CONTRADICTION. Since 2026-09-01 it is a memory ceiling on the control stack,
// in bytes, unset meaning the machine -- but M9 builds the stack that reads it,
// so the row stays `{false, 0, 0}` until there is a consumer to bound the value
// FOR. config.cpp's Dial arm carries the argument.
//
// What changed on 2026-08-31 is that `division_digits` HAS a meaning -- M8 gave
// it one -- so the two things a count of digits cannot be are now known, and
// knowing them is what lets them be refused with a caret instead of quietly
// replaced inside limits::division_digits(). The bounds themselves live beside
// the meaning in limits.hpp; this table only says which dial has one.
//
// IT IS NOT A BOUND ON THE LANGUAGE AND DESIGN §7.5 IS NOT WHAT IT BREAKS.
// The division itself has no ceiling: `division_digits=5000000` is five million
// digits and satl computes them. These two are the values that are not counts
// of digits at all -- nothing kept, and a number too wide to hold the count in.
struct DialRange {
    bool checked;
    unsigned long long least;
    unsigned long long most;
};

inline constexpr DialRange kDialRanges[kDialCount] = {
    {true, kDivisionDigitsLeast, kDivisionDigitsMost},   // division_digits, M8
    {false, 0, 0},                                       // max_depth, M9 -- bytes
    {false, 0, 0},                                       // min_free_mb
    {false, 0, 0},                                       // float_digits, M15
};

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

// --- the machine's own answer, as a file writes it ---------------------------
//
// `arguments.machine.cores`, WHICH IS WHAT A PROGRAM WRITES AND SO IS WHAT THIS
// FILE WRITES. DESIGN §7.7 is emphatic that `arguments` is "the one place a
// bare identifier is language-owned" -- a program never spells
// `satellite.library.main.arguments`, it writes the name it gave main's
// parameter -- so a config that made you write the full path would be asking
// for a spelling the language itself does not use.
//
// THE PREFIX IS TAKEN OFF THE REGISTRY RATHER THAN SPELLED HERE, which is the
// same argument dial_name() makes: words.def already knows that this node's
// full path is `satellite.library.main.arguments.machine.cores`, and a literal
// `"arguments.machine.cores"` in this file would be a second copy free to
// disagree with it after a rename.
inline std::string fact_spelling(words::NodeId node)
{
    const std::string full = words::path_text(node);
    const std::string under = words::path_text(words::NodeId::LIBRARY_MAIN) + ".";
    return full.size() > under.size() && full.compare(0, under.size(), under) == 0
               ? full.substr(under.size())
               : full;
}

// The node a written value names, or kNoPath.
//
// WALKED THROUGH THE REAL TRIE AND NOT COMPARED AGAINST A STRING, which is what
// makes all six spellings of `arguments` work here for free. DESIGN §7.7 gives
// the special variable six names -- `arg`, `args`, `argz`, `argument`,
// `arguments`, `argumentz` -- and words_walk.hpp already matches an alias at
// every segment, so `args.machine.cores` resolves for exactly the reason it
// resolves inside a program. A string comparison would have had to know about
// the aliases, which is the drift this whole module is arranged to avoid.
//
// THE ROOT IS PUT BACK ON HERE. words::walk() takes a path rooted at
// `satellite` -- §1's generating rule -- and what a file writes is the bare
// form, so this is the one place the two spellings are bridged.
inline words::PathId fact_named(std::string_view written)
{
    const std::string rooted =
        words::path_text(words::NodeId::LIBRARY_MAIN) + "." + std::string(written);
    const words::Walk found = words::walk(rooted);
    return found.error == words::WalkError::NONE ? found.id : words::kNoPath;
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
