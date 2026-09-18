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
// ---------------------------------------------------------------------------
// `b = a` COPIES. RED NOTE 8 IS CLOSED (2026-09-18), because `a[2] = x` now
// exists and a program can finally tell the difference.
// ---------------------------------------------------------------------------
//
// A LIST IS A VALUE, NOT A REFERENCE -- the opposite of a spacesuit (DESIGN 7.4)
// and of a file, and deliberately so. This is 003 §12's ruling carried forward,
// not a new decision:
//
//     b = a
//     b[1] = "changed"      -- a[1] is untouched
//
// THE HANDLE IS AN OPTIMISATION, NOT THE SEMANTICS. Copying `b = a` copies a
// pointer; the vector is only really copied when one of them is WRITTEN to and
// the handle is shared. That is copy-on-write, and it means `b = a` on a
// million-item list costs nothing until somebody changes one of them.
//
// THE TRAP 003 FELL INTO, WRITTEN DOWN SO 004 DOES NOT: §12 specified this fast
// path and it was DEAD CODE, because `use_count() == 1` was never true -- the
// interpreter always held a second handle, the copy on its value stack. The fix
// took two halves and a measurement to find (`19526c9`).
//
//   **SO: NEVER CALL about_to_change() THROUGH A COPY OF THE VALUE.** The walker
//   reaches a list to write it by holding a REFERENCE into the VariableTable
//   (`Variable &`, then `satelliteObject &` down the chain), so the only handles
//   that exist are the real ones. Take a `Value` by value anywhere on that path
//   and every write silently becomes a full copy -- correct, and quadratic, and
//   nothing fails to tell you.
//
// NESTING FALLS OUT OF THIS RATHER THAN NEEDING ITS OWN RULE. `a[1][2] = x`
// makes each handle along the way unique in turn: if `a` was shared, cloning it
// leaves its items shared with the original, so the inner handle is shared too
// and is cloned when it is reached. Path copying, and nobody had to write it.

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

// THE ONE DOORWAY TO CHANGING A LIST, and every write goes through it.
//
// It answers the body to write into, having first made sure nobody else is
// holding it. Read the header before calling this: it is correct only when the
// handle passed in is the REAL one -- the variable's own -- and not a copy.
//
// `use_count() == 1` IS THE WHOLE TEST, and it is exact rather than
// approximate: a shared_ptr knows how many handles exist, so "is anybody else
// holding this" is not a guess. 003 asked the same question in a place where
// the answer was always no.
inline satelliteList &about_to_change(ListHandle &handle)
{
    if (handle == nullptr) {
        handle = make_list();               // a name that never held one
        return *handle;
    }
    if (handle.use_count() > 1)
        handle = std::make_shared<satelliteList>(*handle);   // somebody else has it
    return *handle;
}

// COUNTING FROM 1, because that is what this language already does: a file's
// lines count from 1 (the author's own example, 2026-09-18) and `f[1]` is its
// first line. A list counting from 0 beside a file counting from 1 would be two
// rules for one bracket.
//
// Answers nullptr when there is no such item, so the caller says so in its own
// words rather than this inventing a sentence.
inline const satelliteObject *item_at(const satelliteList &list, unsigned long long int position)
{
    if (position == 0 || position > list.items.size()) return nullptr;
    return &list.items[static_cast<std::size_t>(position - 1)];
}

inline satelliteObject *item_at(satelliteList &list, unsigned long long int position)
{
    if (position == 0 || position > list.items.size()) return nullptr;
    return &list.items[static_cast<std::size_t>(position - 1)];
}

} // namespace satellite004
