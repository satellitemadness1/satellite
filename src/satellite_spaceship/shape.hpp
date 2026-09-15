#pragma once

// What a `satellite.include(...)` names -- PLAN M25, built 2026-09-13; paths
// added at 003 revision 07, 2026-09-15.
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
// AND THE SAME TWO SPELLINGS AS A PATH, IN QUOTES -- revision 07. The author:
// "satellite.include("dir/dir/file") I don't need a file extension added, if
// there isn't one, but you could optionally accept a file extension", and then
// "../dir/file and any combination of that, and ../../file ... relative
// including, from that files directory". So
//
//     satellite.include("parts/ship")              parts/ship.satl
//     satellite.include("../shared/ship.satl")     the extension may be written
//     satellite.include("/home/me/ships/ship")     a path from the root
//     satellite.include("parts/ship"(1, "two"))    and with arguments
//
// A path that does not start with `/` is relative to the directory of THE FILE
// THAT WRITES THE INCLUDE, exactly as a bare name is -- so a spaceship in
// `parts/` that includes "../shared/log" reaches `shared/log.satl` beside
// `parts/`, whoever included it. The spaceship is still named by its FILE:
// every path above names the spaceship `ship`, reached as `ship.setup()`.
//
// ONE READING, THREE READERS. The loader finds the file from it, resolve
// resolves the arguments and nothing else, and the compiler compiles them --
// and the tree the parser built is an ordinary expression (a Name, a String, a
// Call, a Member), so three copies of "is this a spaceship and where are its
// arguments" would be three places another spelling could be half-accepted.
// It is here once, over the arena, with no numbering and no file system.

#include "abstract_syntax_tree/ast.hpp"

#include <string_view>

namespace satellite::spaceship {

enum class Named : uint8_t {
    Nothing,     // `satellite.include()` `1 1 0`
    Runtime,     // `satellite.include(satellite)` `1 1 1`
    Spaceship,   // one of the spellings above -- `1 1 2`
    NotAName,    // anything else: a number, a member, an option -- S1604
    BadPath,     // a quoted path whose file name is not a name -- S1607
};

struct Shape {
    Named named = Named::Nothing;

    // The node that spells the spaceship -- a Name, or for a path its String --
    // and its call's argument list, kNoList when no parentheses follow it.
    NodeIndex name = kNoNode;
    NodeIndex call = kNoNode;
    ListId arguments = kNoList;

    // True when `name` is a quoted path rather than a bare name.
    bool path = false;

    uint32_t argument_count(const Ast &ast) const
    {
        return arguments == kNoList ? 0 : ast.list_size(arguments);
    }
};

// What an Include node names. `include` must be a NodeKind::Include.
Shape shape_of(const Ast &ast, NodeIndex include);

// The spaceship's NAME, or empty when it names none: `ship` for every spelling
// above, a view into the tree's own text so it lives as long as the tree does.
std::string_view name_of(const Ast &ast, const Shape &shape);

// What was written between the quotes of a path -- "parts/ship" -- or the bare
// name for the other spellings. The loader turns it into a file.
std::string_view written_of(const Ast &ast, const Shape &shape);

} // namespace satellite::spaceship
