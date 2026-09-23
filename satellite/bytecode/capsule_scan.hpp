#pragma once
// satellite/bytecode/capsule_scan.hpp -- THE SCAN'S OWN PIECES, shared by the two files
// that make it: capsule_scopes.cpp (files, spaces, capsules, includes) and
// suit_scan.cpp (a spacesuit's body). Nothing outside those two includes this; what
// the rest of satl reads is capsule_scopes.hpp.

#include "capsule_scopes.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {
namespace scan {

// WHAT EACH `{` THE SCAN HAS OPENED IS, innermost last. A spacesuit's sections are
// not scopes -- they say whether what is written in them is public -- so a section
// is the spacesuit's scope again, opened as a part of it.
enum class Opened { file, space, suit, protected_part, public_part };

struct Open {
    std::size_t scope;
    Opened what;
};

inline bool in_a_suit(const Open &top)
{
    return top.what == Opened::suit || top.what == Opened::protected_part || top.what == Opened::public_part;
}

// HOW A SENTENCE NAMES A SCOPE: "this file", the space or the spacesuit by its dotted name.
std::string where_is(const CapsuleScope &scope);

void refuse(CapsuleTable &table, std::size_t row, std::size_t at, signed long long int code, std::string why);

// A NAME IS DECLARED ONCE IN A SCOPE -- a capsule, a space, a spacesuit or a field.
bool free_in(CapsuleTable &table, std::size_t scope, const std::string &name, std::size_t row, std::size_t at,
             const char *declaring);

// A CAPSULE'S HEADER AFTER ITS NAME: its parameters, its satellite.returns, and the `{`
// its body opens with -- `k` on what follows the name. Answers the brace's position,
// or 0 when there is none; a header it cannot read goes into `trouble`.
std::size_t header_rest(const std::vector<std::bitset<16>> &row, std::size_t k, const std::string &name,
                        std::vector<CapsuleParameter> &parameters, TypeShape &returns, std::string &trouble);

// The `{` a declaration's body opens with, from `at`, past its line and any line ends.
std::size_t body_after(const std::vector<std::bitset<16>> &row, std::size_t at);

// A CAPSULE DECLARED IN `here` (a file, a space or a spacesuit), its header read by
// the caller: made a site, named in its scope. `suit_part` says which part of a
// spacesuit it was written in (Opened::file for any other scope).
void declare_capsule(CapsuleTable &table, std::size_t r, std::size_t file_scope, std::size_t here, std::size_t at,
                     std::size_t brace, std::string name, std::vector<CapsuleParameter> parameters,
                     TypeShape returns, Opened suit_part);

// suit_scan.cpp: `satellite.spacesuit name { ... }` at `i` -- the scope made and opened,
// `i` left inside its body. Answers false when it has no body, `i` then past its line.
bool open_suit(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, std::size_t file_scope,
               std::vector<Open> &open, std::size_t &i);

// suit_scan.cpp: ONE LINE INSIDE A SPACESUIT, `i` on its first code and left past it.
void suit_line(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, std::size_t file_scope,
               std::vector<Open> &open, std::size_t &i);

// suit_reach.cpp: every spacesuit NAME written as a type in a header or a field, made
// into the scope it reaches -- once every file is scanned and joined to its includes.
void resolve_types(CapsuleTable &table);

} // namespace scan
} // namespace satellite004
