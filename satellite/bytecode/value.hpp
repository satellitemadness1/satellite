#pragma once
// satellite/bytecode/value.hpp -- where a capsule's variables live, and the one
// name the walker knows the object model by.
//
// WHAT CHANGED, 2026-09-16: `Value` IS `satelliteObject` NOW. This file used to
// define its own value type -- a Kind enum beside a std::string, a
// satellite_number and a bool, all three always present. The object model
// replaces it (satellite_object/satellite_value.hpp), and this file keeps only
// what is about the WALKER rather than about values: a variable, and the table a
// running body holds them in.
//
// THE ALIAS IS DELIBERATE AND IS NOT A TRANSITION SHIM. `Value` is what the
// walker calls what an expression is worth, and satelliteObject is what the
// object model calls it. Both names are right in their own file, and one
// `using` is cheaper than renaming the word `Value` through every line of
// expression.cpp and program_walk.cpp -- where it reads correctly already.
//
// WHAT THE NEW TYPE BUYS THE WALKER, beyond the arms it did not have:
//   - A STRING IS A satellite_string, not a std::string of UTF-8 bytes. The
//     language's own 16/32-bit string with the author's character table reaches
//     the interpreter for the first time; UTF-8 is now only a doorway, at the
//     literal coming in (`of_utf8`) and at a library's text scenario going out
//     (`text_utf8`).
//   - ONE PLACE DECIDES WHAT TWO KINDS DO. `left.add(right, out, why)` routes on
//     the pair of tags to a one-function-one-file header. expression.cpp no
//     longer carries a branch per pair.
//
// THERE ARE NO GLOBALS (the author, 2026-09-16): "we begin exe inside of main,
// and end exe inside of main... the only globals are the includes, other files".
// So a VariableTable belongs to ONE running body and is created by run_body when
// that body starts. A capsule cannot see its caller's variables, because it is
// handed a different table -- enforced by construction rather than by a check
// that could be forgotten.
//
// A VARIABLE REMEMBERS THE WORD THAT DECLARED IT. `satellite.variable.number n`
// stores the code of satellite.variable.number beside the value, so a later
// `n = "text"` is refused with types_do_not_meet (27) instead of quietly making
// n a string. satellite is typed by its declaration, and this is where that
// survives past the line that wrote it.

#include "suit_layout.hpp"
#include "token_codes.hpp"
#include "type_shape.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"

#include <string>
#include <unordered_map>

namespace satellite004 {

using Value = satelliteObject;

struct Variable {
    token::Code declared = 0;   // the word code of satellite.variable.number, .string, ...

    // AND WHAT WAS BETWEEN ITS < AND >, when it had any:
    // `satellite.container.index<satellite.variable.string, satellite.variable.number>`
    // keeps both, and `satellite.container.multiple<a, b>` keeps the list of
    // types the name will accept. `declared` is still the word, so every test
    // that only cares which word it was reads exactly as it did.
    //
    // EMPTY MEANS "OF ANYTHING", not "unchecked by accident": a list declared
    // with no <> holds whatever the braced literal put in it, which is what the
    // literal already makes. type_shape.hpp says why that is the default.
    TypeShape shape;

    Value value;
};

// A NAME AS A BODY SEES IT: one of its own variables, or a field of the object its
// method runs on. `value` is null when it is neither. Pointers into the table or the
// object, never copies -- a list's `.append` must reach the real one (satellite_list.hpp).
struct Seen {
    Value *value = nullptr;
    token::Code declared = 0;
    const TypeShape *shape = nullptr;
    bool field = false;

    explicit operator bool() const { return value != nullptr; }
};

// One running body's variables. Created per body, never shared: see the header.
//
// AND, FOR A METHOD, THE OBJECT IT RUNS ON (2026-09-22). A spacesuit's capsules read
// and write its fields by their bare names -- the author's `path = path_input` in a
// constructor, `satellite.return(spacesuit_name)` in a method -- and the fields are
// the OBJECT's, so they live there and not in this table: a method calling another
// method of the same object must see what the first one wrote. `self` is that object,
// and null for every capsule that is not a spacesuit's. There are still no globals:
// a field is reached only by a body running on its object.
//
// ITS OWN NAMES COME FIRST, then the object's -- and the checker refuses a variable
// or a parameter named like a field, so the order is never what decides.
struct VariableTable : std::unordered_map<std::string, Variable> {
    UserDefinedHandle self;
    // HOW MANY OF ITS FIELDS THIS CAPSULE'S SPACESUIT HAS: a supertype's capsule, running
    // on an object of a spacesuit that extends it, sees the supertype's (suit_layout.hpp).
    std::size_t fields_seen = kNoSlot;

    Seen seen(const std::string &name)
    {
        const iterator own = find(name);
        if (own != end())
            return Seen{&own->second.value, own->second.declared, &own->second.shape, false};
        if (self != nullptr && self->layout != nullptr) {
            const std::size_t slot = self->layout->slot_of(name, fields_seen);
            if (slot != kNoSlot) {
                const SuitField &field = self->layout->fields[slot];
                return Seen{&self->fields[slot], field.shape.word, &field.shape, true};
            }
        }
        return Seen{};
    }
};

} // namespace satellite004
