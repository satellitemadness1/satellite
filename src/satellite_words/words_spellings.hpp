#pragma once

// The spelling interner, and every node's children -- part of words.hpp.
//
// DESIGN §4.4 asks for a string interner "so that `list` under `container` and
// `list` under `directory` are two nodes sharing one piece of text", and this is
// it: MANY NODES, ONE STRING. WORD_NUMBERS §2.3's alias is the other direction
// -- ONE NODE, MANY STRINGS -- and both are real, so both exist; the aliases
// live in the node table next door and are resolved by the walk.
//
// NEITHER IS THE TRIE. The trie holds the nodes and their numbers; the spellings
// are a table beside it. That separation is what lets `1 5 1` print back as
// `satellite.console.display` with no global word-id-to-text map anywhere.
//
// A SPELLING ID IS THE LOWEST NODE THAT CARRIES THE SPELLING, so interning needs
// no second table of strings and two nodes spelled alike compare equal in one
// integer. That is what DESIGN §4.4 means by "§1's reserved word is one interner
// comparison": whether a bare word is spelled `satellite` is answered at the
// binding site against kSatelliteSpelling, with no walk.
//
// SPLIT FROM words_walk.hpp BY SUBJECT, when the two together reached 305 lines
// against PLAN §3's target of 300. The line count is what prompted the look; the
// seam is what justified the split, and it is a real one -- everything here is a
// compile-time table about the SHAPE of the trie, and everything there is the act
// of READING a path against it. A seam chosen to satisfy an arithmetic would have
// gone somewhere else and been worse.

#include <array>
#include <cstdint>
#include <string_view>

#include "satellite_words/words_nodes.hpp"
#include "satellite_words/words_numbers.hpp"

namespace satellite::words {

using SpellingId = PathId;

inline constexpr SpellingId kNoSpelling = 0;

namespace detail {

// The children of every node, as a linked list in ascending file order.
//
// A NODE'S CHILDREN ARE NOT CONTIGUOUS IN words.def, because the file is
// depth-first: satellite.container's children are 1 4 1 and 1 4 2, and TEN rows
// sit between them -- the map's bare shape and its nine methods. A first/next
// pair costs two words per node and turns "the children of this node" into a
// walk of exactly those children rather than a scan of all 260.
//
// (This paragraph said "the map's twenty-nine methods" until 2026-08-28. The
// map has nine; 29 was a figure PLAN M16 carried for the map and the list
// TOGETHER, and that figure was itself wrong -- it is 34.)
struct Children {
    std::array<PathId, kNodeCount + 1> first;
    std::array<PathId, kNodeCount + 1> next;
};

constexpr Children compute_children()
{
    Children c{};
    // BACKWARDS, so that prepending yields a list in ascending order -- which
    // is ascending NUMBER order too, and the walk below depends on that when it
    // takes "the lowest-numbered shape" for a word written with no arguments.
    for (PathId i = kNodeCount; i > 0; i--) {
        const PathId parent = static_cast<PathId>(kNodes[i].parent);
        c.next[i] = c.first[parent];
        c.first[parent] = i;
    }
    return c;
}

inline constexpr Children kChildren = compute_children();

// Each node's spelling id: the lowest node index spelled the same way.
constexpr std::array<SpellingId, kNodeCount + 1> compute_spellings()
{
    // THE SPELLINGS ARE CUT ONCE, BEFORE THE SEARCH, and that is a compile-time
    // budget rather than a style preference. Written as one loop that called
    // spelling_of() on both sides of the comparison, this re-scanned a text for
    // its `(` on every one of the 32,000 pairs and clang stopped the build at
    // its million-step constexpr limit. Cutting first turns the inner loop into
    // a compare of two views that already know their length.
    std::array<std::string_view, kNodeCount + 1> word{};
    for (PathId i = 1; i <= kNodeCount; i++)
        word[i] = spelling_of(static_cast<NodeId>(i));

    std::array<SpellingId, kNodeCount + 1> s{};
    for (PathId i = 1; i <= kNodeCount; i++) {
        // A row that begins with `(` has no spelling and must never intern to
        // one, or "(satellite)" would answer a lookup for a word.
        if (word[i].empty()) {
            s[i] = kNoSpelling;
            continue;
        }
        s[i] = i;
        for (PathId j = 1; j < i; j++)
            if (word[j] == word[i]) {
                s[i] = s[j];
                break;
            }
    }
    return s;
}

inline constexpr std::array<SpellingId, kNodeCount + 1> kSpellings = compute_spellings();

} // namespace detail

constexpr SpellingId spelling_id(NodeId id)
{
    return detail::kSpellings[static_cast<PathId>(id)];
}

// The reserved word, as one integer (DESIGN §2, §4.4).
inline constexpr SpellingId kSatelliteSpelling = spelling_id(NodeId::SATELLITE);

constexpr PathId first_child(NodeId id)
{
    return detail::kChildren.first[static_cast<PathId>(id)];
}

constexpr PathId next_sibling(PathId id)
{
    return detail::kChildren.next[id];
}

// A bare word's spelling id, or kNoSpelling if the language does not use it.
//
// LINEAR, AND NOW MEASURED. This comment used to end "there is nothing to
// measure until M3 runs it over a real source", and M3 ran it over a 273-byte
// program where the whole lex cost 0.06 ms and the question stayed open --
// MILESTONES/M3.md §6 item 5 said the number to take was throughput on a large
// generated source. M4 took it, because M4 is the milestone that first reads a
// whole program.
//
// MEASURED HERE, 2026-08-30, clang -O3, best of 20 runs, on a generated
// 321 KB / 11,001-line source lexing to 81,008 tokens:
//
//     lex                                32.088 ms
//     lex with this scan removed         13.717 ms
//     parse the same stream               6.131 ms
//
// SO THIS FUNCTION IS 18.4 ms OF A 32 ms LEX -- 57% of lexing, and nearly three
// times what parsing the same stream costs. The scan is free at the 255 tokens
// of the largest program in example/ and is the dominant cost of reading a real
// one, which is exactly the shape M3 predicted and could not yet see.
//
// IT IS STILL NOT CHANGED HERE, and that is a scheduling decision rather than a
// disagreement with the number: what the shape guarantees is that replacing
// this with a hash changes one function and no caller, so it can be done by
// whichever milestone first has a reason to care about a large file --
// M4.5's `.satc` writer is reading one by definition. MILESTONES/M4.md §7
// carries the measurement and §6 carries it as open.
inline SpellingId intern(std::string_view word)
{
    if (word.empty())
        return kNoSpelling;
    for (PathId i = 1; i <= kNodeCount; i++)
        if (detail::kSpellings[i] == i && spelling_of(static_cast<NodeId>(i)) == word)
            return i;
    return kNoSpelling;
}

} // namespace satellite::words
