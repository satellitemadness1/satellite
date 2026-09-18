#pragma once
// satellite/satellite_object/satellite_list.hpp -- A BRACED LIST OF OBJECTS.
//
// The author, 2026-09-18: *"I want satellite.feedback({"string", "string"}) to
// just be built into the system, we need to build satellite object definitions
// to be this: = {series_of_objects, another_object} and we need to build it this
// way if it doesn't already have the left-bracket, right-bracket type if syntax
// ... that is how I want the interpreter to be built"*.
//
// So `{` where a VALUE is expected opens a list, and everything to the matching
// `}` is its items:
//
//     satellite.feedback({"the caret is great", "the prompt eats my tabs"})
//     x = {1, 2, 3}
//     y = {}                       -- a list of nothing, which is still a list
//     z = {1, "two", {3, 4}}       -- mixed, and nested, because each item is
//                                     just an object and a list IS an object
//
// A BRACE IS STILL A BLOCK EVERYWHERE IT ALREADY WAS. `if x { ... }` is
// untouched, because a block's `{` never appears where a value is expected --
// the two live in different positions and never in the same one. That is why
// this needed no new token and no lookahead: the PARSER'S POSITION already knows
// which is which. (See one_operand in bytecode/expression.cpp, which is only
// ever called where a value belongs.)
//
// ---------------------------------------------------------------------------
// A HANDLE, LIKE THE SPACESUIT AND THE FILE, AND FOR A PLAINER REASON THAN
// EITHER.
// ---------------------------------------------------------------------------
//
// A list holds satelliteObjects, and a satelliteObject may BE a list. Put the
// vector in the variant directly and the type contains itself, which is the same
// wall satelliteUserDefinedObject hit. So the arm is a shared_ptr and the
// definition lives here, after satelliteObject is complete.
//
// WHETHER TWO NAMES FOR ONE LIST ARE ONE LIST IS **NOT DECIDED HERE**, and the
// distinction is worth stating because the handle looks like it decides it.
//
//   RED NOTE FOR THE AUTHOR: `b = a` on a list currently SHARES, because copying
//   the handle is what the variant does. Nothing can observe that yet -- 004 has
//   no word that changes a list once it is made, so a shared list and a copied
//   list behave identically in every program that can be written today.
//
//   THE MOMENT AN APPEND EXISTS, IT MATTERS, and 003 already ruled: §12 made
//   containers copy-on-write, with the `use_count() == 1` check that
//   19526c9 later had to split in two. That is the behaviour to build when
//   `satellite.container.list` gets its words -- a write with the handle shared
//   clones first, so `b = a` then `b.append(x)` leaves `a` alone.
//
//   It is left undone rather than guessed at because the cost of guessing is a
//   language where assignment sometimes aliases: the one bug a person cannot
//   see in their own code.

#include "satellite_object.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace satellite004 {

struct satelliteList {
    std::vector<satelliteObject> items;

    satelliteList() = default;
    explicit satelliteList(std::vector<satelliteObject> its_items) : items(std::move(its_items)) {}

    std::size_t size() const { return items.size(); }
    bool empty() const { return items.empty(); }
};

inline ListHandle make_list(std::vector<satelliteObject> items)
{
    return std::make_shared<satelliteList>(std::move(items));
}

inline ListHandle make_list()
{
    return std::make_shared<satelliteList>();
}

} // namespace satellite004
