#pragma once
// satellite/bytecode/access_calls.hpp -- satellite.access(name): WHAT A NAME IS, WHAT IT
// HOLDS, AND HOW TO REACH EVERY LEVEL OF IT.
//
// The author, 2026-09-18 (MILESTONES M34): "access is a little bit of info about the
// object, the objects value and type only". And 2026-09-25 (SCRATCH.md/CONTAINERS.md,
// step 2): "for any combination of containers, lists, maps, multiples ... when we type
// satellite.access(complex_list_of_maps_object) it gives the correct syntax to access
// that object, no matter what that object is" -- and "we can assume the declaration of
// the container is correct". So every line is worked out from the DECLARATION, with the
// value only filling in how many there are now; nothing is guessed from a line that went
// wrong.
//
//     satellite.container.list<satellite.container.map<satellite.variable.string,
//         satellite.container.list<satellite.variable.number>>> rows = {{"a": {1, 2}}, {"b": {3}}}
//     satellite.access(rows)
//
//     rows is a list of maps (string -> list of numbers), 2 items
//       value                       {{"a": {1, 2}}, {"b": {3}}}
//       rows[n]                     a map (string -> list of numbers), n counting from 1 (1 to 2)
//       rows[n]["key"]              a list of numbers, "key" a string
//       rows[n]["key"][m]           a number, m counting from 1
//       rows.append({"key": {1}})   adds a map
//       rows[n]["key"] = {1}        puts a list of numbers under "key"
//       rows[n]["key"].append(1)    adds a number
//       rows.size                   how many maps it holds
//       rows[n].keys                the keys of a map, as a list
//
// A SPACESUIT'S OBJECT is reached by its capsules, since a field is reached only from
// inside (suit_layout.hpp): its public capsules are the lines, and its fields are listed
// with their values as what it holds.
//
// IT ANSWERS TEXT, so display(satellite.access(rows)) prints it and a program can keep
// it. A LINE THAT IS NOTHING BUT satellite.access(rows) PRINTS IT -- in a file and at the
// prompt -- because a line that works something out and throws it away does nothing.
//
// THE access SWITCH (arguments.access, feature bit 0) is not asked. It decides whether the
// last-known STORE is kept -- every name's last value, readable after the program stops
// (SATELLITE_ARGUMENTS.md) -- and that store is not built. This reads a name in scope now.

#include "expression.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

struct CapsuleTable;

// satellite.access 1 31, and satellite.access(name) 1 31 1, which a call lexes to.
bool is_access_word(token::Code code);

// THE CHECKER'S: "" when `satellite.access(` at `at` (on the word) holds one name and
// nothing else, or what to say instead. The name itself is judged as declared by the
// checker's own loop, as every name in a statement is.
std::string access_refused(const std::vector<std::bitset<16>> &row, std::size_t at);

// THE WALKER'S: `at` on the `(` after the word, left past the `)`.
Value call_access(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context);

// The text itself, for a name, what it was declared as, and what it holds now. Separate
// so the prompt could ask it of a name it keeps.
std::string access_text(const std::string &name, const TypeShape &shape, const Value &value,
                        const CapsuleTable *capsules);

} // namespace satellite004
