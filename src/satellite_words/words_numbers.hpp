#pragma once

// The numbers, derived -- part of words.hpp, which includes this file.
//
// WORD_NUMBERS.md §2.2 WRITES A NUMBER IN EVERY ROW AND words.def CARRIES NONE.
// That is PLAN §8's instruction ("its position among that parent's children IS
// its number") and it is the one decision in this module that pays for itself
// twice: a number column would be a second place the numbering lives, and the
// project's own rule is that prose may explain a number but may never be the
// only place it lives -- which cuts both ways. Two places that can disagree is
// the failure; one place plus a derivation is not.
//
// So the numbers below are COMPUTED AT COMPILE TIME from file order, and
// tests/words_test compares all 223 of them against the markdown that is the
// authority. The `// 1 5 1` comments in words.def are read by people only.
//
// The whole table is one forward pass, which is legal only because a parent is
// declared before its children. That is not assumed: words_invariants.hpp
// asserts it, and it is the same property as "no node is its own ancestor"
// seen from the other end.

#include <array>
#include <cstdint>
#include <string>

#include "satellite_words/words_nodes.hpp"

namespace satellite::words {

namespace detail {

// Both tables in one pass, because they are the same count read two ways: a
// node's number is what its parent's counter said when the row was reached, and
// the parent's child count is what that counter finished at.
struct Numbering {
    std::array<uint32_t, kNodeCount + 1> number;
    std::array<uint32_t, kNodeCount + 1> children;
};

constexpr Numbering compute_numbering()
{
    Numbering n{};
    for (PathId i = 1; i <= kNodeCount; i++) {
        const PathId parent = static_cast<PathId>(kNodes[i].parent);
        // A BARE SHAPE TAKES NO POSITION FROM ITS SIBLINGS. It is position 0
        // and the counter does not move, which is what keeps a parent's
        // numbered children dense from 1 whether it has one or not.
        if (kNodes[i].kind == kBare) {
            n.number[i] = 0;
            continue;
        }
        n.number[i] = ++n.children[parent];
    }
    return n;
}

inline constexpr Numbering kNumbering = compute_numbering();

} // namespace detail

// This node's position among its parent's children. 0 is the bare call shape.
constexpr uint32_t number_of(NodeId id)
{
    return detail::kNumbering.number[static_cast<PathId>(id)];
}

// How many numbered children words.def gives this node.
//
// THE FROZEN COUNT, AND THEREFORE THE FIRST NUMBER A USER'S NAME CAN TAKE.
// PLAN §8.1 requires every node to be able to say what is free under it, and
// this is where that answer starts -- words_runtime.hpp is what carries it
// forward as names are met. The language's children are numbered first and
// never move; the user's are appended after them.
constexpr uint32_t frozen_children(NodeId id)
{
    return detail::kNumbering.children[static_cast<PathId>(id)];
}

// How many segments the path has. `satellite` is 1.
constexpr uint32_t depth_of(NodeId id)
{
    uint32_t depth = 0;
    for (NodeId at = id; at != NodeId::NONE; at = parent_of(at))
        depth++;
    return depth;
}

// The number, as WORD_NUMBERS §2.2 writes it: "1 5 1".
//
// Built back to front and reversed, because a node knows its parent and not its
// children, and that is the direction the whole module is arranged in.
inline std::string number_text(NodeId id)
{
    std::string out;
    for (NodeId at = id; at != NodeId::NONE; at = parent_of(at)) {
        std::string one = std::to_string(number_of(at));
        if (!out.empty())
            one += ' ';
        out.insert(0, one);
    }
    return out;
}

// The path, as §2.2 writes it: "satellite.console.input(prompt, target)".
//
// A TEXT BEGINNING WITH `(` JOINS WITH NO DOT, and that one rule is what makes
// the join reproduce the authority's path column character for character --
// which tests/words_test checks for every row rather than trusting.
// `satellite.include(satellite)` is include's child, so the dot would be wrong;
// `satellite.console.display` is console's child, so it is right.
inline std::string path_text(NodeId id)
{
    std::string out;
    for (NodeId at = id; at != NodeId::NONE; at = parent_of(at)) {
        // THE TEST IS ON THE CHILD, NOT ON THIS SEGMENT. What decides whether a
        // dot belongs is whether the thing already built starts an argument
        // list, and that is out's first character rather than this node's.
        // Written the other way round it produces "include.(satellite)", which
        // is a path the language does not have.
        const bool child_is_argument = !out.empty() && out.front() == '(';
        const bool separate = !out.empty() && !child_is_argument;
        out.insert(0, std::string(text_of(at)) + (separate ? "." : ""));
    }
    return out;
}

} // namespace satellite::words
