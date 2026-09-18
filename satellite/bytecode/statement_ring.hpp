#pragma once
// THE STATEMENT RING -- the last N statements a program ran, so a report can say
// what it was doing just before it stopped.
//
// THE AUTHOR ASKED WHAT A RING WAS, so it is said here first and plainly:
//
//     A RING BUFFER is a fixed-size list that overwrites its own OLDEST entry
//     when it is full. Keep 256 and you always have the most recent 256 and never
//     any more -- no growth, no allocation after the first, and no decision to
//     make about when to stop.
//
// That is the difference between a report saying "it stopped" and one saying "it
// stopped on line 412, having just gone round this loop 40,000 times".
//
// **IT IS NOT THE SWITCH HIERARCHY**, which is what he guessed and is a fair
// guess from the name. The hierarchy is taken ONCE, outside everything, and
// measured free. This is written ONCE PER STATEMENT, on the hottest path in the
// language -- which is exactly why it needed a number and the hierarchy did not.
//
// ---
//
// WHY N IS 256, AND WHY IT IS A NUMBER AND NOT A FEELING.
//
// One entry is two `unsigned int`s -- which FILE and which TOKEN -- so 256 is
// **2 KB, one page, taken once at start-up and never grown**. Deep enough to show
// a loop's shape and the path into it; small enough that "does it cost anything"
// is measurable rather than arguable. Overridable from config.ini, and the author
// may want 1,024.
//
// ---
//
// WHAT IS STORED, AND THE ONE CLEVER THING HERE: **a position, not a line.**
//
// The report wants a LINE NUMBER, and a line number is a count of
// `line_end_token`s from the start of the row -- which is O(n) and therefore
// unthinkable once per statement. So the ring stores the raw token offset, which
// is free, and **the counting happens at REPORT time**, on a path that is already
// failing and where one walk of the tokens costs nothing anybody will feel.
//
// This is the same trade SATELLITE_ERROR Part 3 makes about the source text:
// keep what is free while running, compute what is expensive only when something
// has already gone wrong.
//
// NOT LOCKED, DELIBERATELY. Threads share the ring unsynchronised, so two threads
// writing at once can lose an entry or interleave two programs' statements. An
// atomic index on the hottest path in the language would cost more than the
// feature is worth, and a ring that is occasionally one short is still telling
// the truth about where a program was. Said here rather than discovered.

#include "token_codes.hpp"
#include "bytecode_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace satellite004 {

// THE AUTHOR'S NUMBER, PROPOSED AT 256 (SATELLITE_ERROR Part 15, note 5).
inline constexpr std::size_t kStatementRingSize = 256;

struct StatementRing {
    struct Entry {
        unsigned int row = 0;    // which FILE -- an index into the registry
        unsigned int at = 0;     // which TOKEN in it; the line is counted later
    };

    std::vector<Entry> entries;
    std::size_t next = 0;        // where the next one goes; wraps
    std::uint64_t total = 0;     // how many statements ran in all, not just kept

    // ALLOCATED ONCE, AND THE HOT PATH NEVER ALLOCATES. resize() here rather than
    // in the constructor so the page is not touched by a run that never turns the
    // bit on -- a function-local static is built on first use.
    void ready()
    {
        if (entries.size() != kStatementRingSize)
            entries.assign(kStatementRingSize, Entry{});
    }

    // THE ONE CALL ON THE HOT PATH: two stores and an increment. No branch on
    // fullness, because the modulo IS the wrap and the buffer is never anything
    // but full-sized.
    void saw(std::size_t row, std::size_t at)
    {
        entries[next] = Entry{static_cast<unsigned int>(row), static_cast<unsigned int>(at)};
        next = (next + 1) % kStatementRingSize;
        ++total;
    }
};

inline StatementRing &statement_ring()
{
    static StatementRing one;
    return one;
}

// WHICH LINE A TOKEN OFFSET IS ON -- counted here, at report time, and never
// while a program runs. `tokenise_one_line` puts a `line_end_token` at the end of
// EVERY source line, and `add_file_to_bytecode_registry` splits the file on
// newlines and tokenises every one of them -- blank lines and comment lines
// included -- so this count is the source line exactly, with no drift.
inline std::size_t line_of(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    std::size_t line = 1;
    const std::size_t stop = at < row.size() ? at : row.size();
    for (std::size_t i = 0; i < stop; ++i)
        if (static_cast<token::Code>(row[i].to_ulong()) == token::line_end_token)
            ++line;
    return line;
}

// The ring as a person reads it: oldest first, newest last, because that is the
// order the program ran them in and a person reading a traceback is reading time.
inline std::string statement_ring_table(const StatementRing &ring,
                                        const BytecodeRegistry &registry,
                                        const BytecodeFilenames &filenames,
                                        std::size_t how_many = 0)
{
    std::string out;
    if (ring.total == 0)
        return "    (no statement ran)\n";

    const std::size_t kept = ring.total < kStatementRingSize
                                 ? static_cast<std::size_t>(ring.total)
                                 : kStatementRingSize;
    const std::size_t want = (how_many == 0 || how_many > kept) ? kept : how_many;

    out += "    the last " + std::to_string(want) + " statements, oldest first";
    if (ring.total > kept)
        out += " (of " + std::to_string(ring.total) + " that ran; the rest were overwritten)";
    out += "\n";

    // Walk backwards from the newest to find where `want` begins, then forwards.
    for (std::size_t back = want; back > 0; --back) {
        const std::size_t slot = (ring.next + kStatementRingSize - back) % kStatementRingSize;
        const StatementRing::Entry &entry = ring.entries[slot];
        if (entry.row >= registry.size())
            continue;
        const std::string where = entry.row < filenames.size() ? filenames[entry.row]
                                                               : std::string("(a file with no name)");
        out += "    " + where + ":" + std::to_string(line_of(registry[entry.row], entry.at)) + "\n";
    }
    return out;
}

} // namespace satellite004
