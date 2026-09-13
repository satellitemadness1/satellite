#pragma once

// What a `satellite.include(...)` names -- PLAN M25, built 2026-09-13.
//
// FOUR SPELLINGS OF ONE THING, and the author chose all four in one sentence:
// "we will design the satellite.include(filename(args)) and it can optionally
// take a .satl extension in the filename". So
//
//     satellite.include(ship)                 ship.satl, no arguments
//     satellite.include(ship(1, "two"))       ship.satl, two arguments
//     satellite.include(ship.satl)            the same file, written out
//     satellite.include(ship.satl(1, "two"))  and with arguments
//
// all name the spaceship `ship`, and the arguments go to its launch capsules.
//
// ONE READING, THREE READERS. The loader finds the file from it, resolve
// resolves the arguments and nothing else, and the compiler compiles them --
// and the tree the parser built is an ordinary expression (a Name, a Call, a
// Member), so three copies of "is this a spaceship and where are its
// arguments" would be three places a fifth spelling could be half-accepted.
// It is here once, over the arena, with no numbering and no file system.

#include "abstract_syntax_tree/ast.hpp"

#include <string_view>

namespace satellite::spaceship {

enum class Named : uint8_t {
    Nothing,     // `satellite.include()` `1 1 0`
    Runtime,     // `satellite.include(satellite)` `1 1 1`
    Spaceship,   // one of the four spellings above -- `1 1 2`
    NotAName,    // anything else: a string, a path, an option -- S1604
};

struct Shape {
    Named named = Named::Nothing;

    // The Name node that spells the spaceship, and its call's argument list --
    // kNoList when the include wrote no parentheses after the name.
    NodeIndex name = kNoNode;
    NodeIndex call = kNoNode;
    ListId arguments = kNoList;

    uint32_t argument_count(const Ast &ast) const
    {
        return arguments == kNoList ? 0 : ast.list_size(arguments);
    }
};

// What an Include node names. `include` must be a NodeKind::Include.
Shape shape_of(const Ast &ast, NodeIndex include);

// The spaceship's name as written, or empty when it names none.
std::string_view name_of(const Ast &ast, const Shape &shape);

} // namespace satellite::spaceship
