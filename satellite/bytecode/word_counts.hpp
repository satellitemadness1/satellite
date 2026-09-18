#pragma once
// PER-WORD CALL COUNTS -- the `word_counts` bit, and the FIRST FEATURE IN 004
// THAT ACTUALLY READS ITS BIT.
//
// Everything before this built the chain and left the far end open: `satl
// --rebuild` composes the register, config.ini holds it, start-up reads it,
// `MachineState` carries it, and the switch hierarchy dispatches on its tiers --
// and then nothing tested an individual bit to do anything different. The author,
// 2026-09-18: *"We still need to build the thing that reads the bits don't we?"*
// He is right, and this is it, end to end.
//
// WHY THIS FEATURE FIRST AND NOT A HARDER ONE. It is the cheapest real one on
// SATELLITE_ERROR's list (Part 11.2), and cheap for a reason that is worth saying
// out loud: **a word's 16-bit code IS the array index.** function_table.hpp says
// so about itself -- "THE CODE IS THE INDEX, and that is what the 4096 range is
// for. `table[code]` is one subtraction and one load: no string compare, no hash,
// no search." So counting a word is `++counts[code - 4097]`, which is the same
// arithmetic the dispatch already did. Nothing is looked up, nothing is hashed,
// and no name is compared.
//
// AND IT IS A PROFILER, WHICH IS THE POINT. The author asked what the report
// could hold that would "aid us in debugging stuff ... we can use this as a debug
// tool anyways, so it will facilitate development". For a language whose client
// reads corpora, "which word ran 40 million times" is the first question worth
// asking of a slow program, and it is the one the interpreter can answer for
// almost nothing.
//
// ---
//
// WHERE THE COST IS, HONESTLY. The increment is gated by one bit test per word
// call, which is the per-statement cost the switch hierarchy exists to hoist --
// and it is NOT hoisted yet. MILESTONES M35 and SATELLITE_ERROR F5b are where
// `RunPlan::plain` gets a walker with no test compiled into it at all. Until
// then a run with the bit off pays one register test a word, which
// feature_switch.hpp measured at about 0.5 ns and which is the honest price of
// having the feature before having the optimisation. **The author asked for the
// structure first and the optimisation later, in those words.**
//
// ONE ARRAY, NO LOCK, AND THAT IS A DELIBERATE INACCURACY. Threads share these
// counters unsynchronised, so two threads counting the same word at the same
// instant can lose a count. The alternative is an atomic increment on the hottest
// path in the language, which would cost more than the feature is worth -- and a
// profiler that is occasionally one short of 40,000,000 is telling the truth
// about where the time went. It is written down rather than discovered, and a
// report that prints these says they are approximate when threads ran.

#include "word_codes.hpp"
#include "../machine/machine_codes.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace satellite004 {

// EVERY WORD, INDEXED BY ITS CODE. 367 words today and the array is sized from
// the generated table, so adding a word to words.tsv resizes this and cannot
// overflow it.
inline constexpr std::size_t kWordCountSlots = word::kWordsInTable;

struct WordCounts {
    // A plain array and not a map: the index is the code, so there is nothing to
    // look up. 367 x 8 bytes is under 3 KB -- one page, touched only by the words
    // a program actually uses.
    std::array<std::uint64_t, kWordCountSlots> hits{};
    std::uint64_t total = 0;

    // THE ONE CALL ON THE HOT PATH, and it is two adds. `code` is a word code,
    // which word_codes.hpp guarantees is 4097 or above, so the subtraction cannot
    // wrap; the bound is checked anyway because a code from a .sate file written
    // by a NEWER build could be past the end of this build's table, and reading
    // past an array to count a word nobody here knows is not worth the risk.
    void saw(token::Code code)
    {
        const std::size_t slot = static_cast<std::size_t>(code) - word::kFirst;
        if (slot < kWordCountSlots) {
            ++hits[slot];
            ++total;
        }
    }
};

// THE ONE PER RUN. A function-local static, which is `Console::the()`'s shape in
// 003 and is here for the same reason: nothing is built before main(), and a run
// that never turns the bit on never touches the page.
inline WordCounts &word_counts()
{
    static WordCounts one;
    return one;
}

// What the report and `--debug` print: every word that ran, most first.
//
// SORTED HERE AND NOT KEPT SORTED, because this runs once at the end of a program
// and the hot path must not pay for the order a person wants to read.
inline std::string word_counts_table(const WordCounts &counts, std::size_t how_many = 0)
{
    struct Row { std::uint64_t hits; token::Code code; };
    std::vector<Row> rows;
    for (std::size_t slot = 0; slot < kWordCountSlots; ++slot)
        if (counts.hits[slot] != 0)
            rows.push_back({counts.hits[slot],
                            static_cast<token::Code>(slot + word::kFirst)});

    // A hand-rolled selection rather than <algorithm>, so this header adds no
    // include to everything that reads it. The list is the words a program USED,
    // which is tens, not the 367.
    for (std::size_t i = 0; i < rows.size(); ++i)
        for (std::size_t j = i + 1; j < rows.size(); ++j)
            if (rows[j].hits > rows[i].hits) {
                const Row keep = rows[i];
                rows[i] = rows[j];
                rows[j] = keep;
            }

    std::string out;
    out += "    calls        word\n";
    out += "    -----------  ------------------------------------------------\n";
    std::size_t shown = 0;
    for (const Row &row : rows) {
        if (how_many != 0 && shown >= how_many)
            break;
        std::string number = std::to_string(row.hits);
        while (number.size() < 11)
            number = " " + number;
        out += "    " + number + "  " + word::spelling_of(row.code) + "\n";
        ++shown;
    }
    if (rows.empty())
        out += "    (no word was called)\n";
    else
        out += "    " + std::to_string(counts.total) + " calls over " +
               std::to_string(rows.size()) + " different words\n";
    return out;
}

} // namespace satellite004
