#pragma once
// satellite/satellite_object/satellite_spacesuit.hpp -- THE SPACESUIT: a variant
// over the two kinds there are.
//
// (the author, 2026-09-16) "there is another class, a satelliteSpacesuit and it
// is just std::variant<satelliteObject, satelliteUserDefinedObject> and then we
// begin to build fast paths with that."
//
// TWO LAYERS, ONE SHAPE:
//
//     satelliteObject     the one WE hand design -- a variant over every built-in
//                         type: satelliteCapsule, satellite_number,
//                         satellite_string, satellite_bytecode, the bool, and
//                         satellite_time and satellite_file when they are built.
//     satelliteSpacesuit  a variant over the TWO KINDS of spacesuit: one of ours,
//                         or one the user defined.
//
// A variant, a Kind that is its index, `pair_of` for the switch, and pair files
// named for what they join. Nothing new to learn at the second layer -- which is
// the author's "it's just more templates built out of our fast paths of
// functions that we defined then".
//
// A SPACESUIT IS JUST A COLLECTION OF BYTECODE (the author, and this file is
// built on that line). Its bytecode is where the registry already has it: the
// scan (bytecode/capsule_scopes.cpp) finds a spacesuit's fields and capsules as
// POSITIONS in its file's row, and an object points at the layout built from them
// -- nothing here re-derives, or copies, what the registry already has.
//
// THE USER-DEFINED ARM IS A HANDLE AND THE HAND-DESIGNED ONE IS NOT, which is
// one difference from the author's line and it is worth the sentence. A
// satelliteUserDefinedObject holds satelliteObjects as fields, so putting it in
// the variant directly is a type that contains itself; and DESIGN 7.4 wants it
// by reference anyway -- "a spacesuit is a reference type", which M26 built as a
// capsule mutating one and the CALLER SEEING IT. So the arm is
// std::shared_ptr<satelliteUserDefinedObject>. A built-in has no such need: a
// number IS its value.

#include "satellite_object.hpp"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace satellite004 {

// ---------------------------------------------------------------------------
// ONE OBJECT: ITS SPACESUIT'S LAYOUT, AND ITS OWN VALUES.
// ---------------------------------------------------------------------------
//
// WHAT CHANGED, 2026-09-22 (MILESTONES M8's "owed inside this one"). This struct
// used to carry its spacesuit's name, a COPY of its bytecode, its field names and
// a satelliteCapsule per method -- in every object. A thousand objects were a
// thousand copies of one declaration, and two sources of truth for each method:
// the copy, and the site the walker actually runs. 003 had already solved it
// (src/satellite_spacesuit/suit_object.hpp): what every object of one spacesuit
// shares is ITS LAYOUT, made once, and an object is a pointer to that and its
// own values. That is this struct now.
//
// THE LAYOUT IS DEFINED IN bytecode/suit_layout.hpp, because a field's declared
// type is a TypeShape and TypeShape is the bytecode's -- this file only needs to
// know there is one. A std::shared_ptr of an incomplete type is complete, the same
// trick satellite_object.hpp plays with this struct.
//
// FIELDS ARE ONE VECTOR IN THE LAYOUT'S ORDER, and a name is resolved to its slot
// by the layout (slot_of) -- the string compare happens there and nowhere else, as
// it did before. What an object holds is values and nothing else.
struct satelliteSuitLayout;

struct satelliteUserDefinedObject {
    std::shared_ptr<const satelliteSuitLayout> layout;   // shared by every object of its spacesuit
    std::vector<satelliteObject> fields;                 // one per field, in the layout's order
};

// ---------------------------------------------------------------------------
// THE SPACESUIT. Same shape as satelliteObject, one layer up.
// ---------------------------------------------------------------------------
class satelliteSpacesuit {
public:
    using Held = std::variant<satelliteObject,   // 0  one we hand design
                              UserDefinedHandle  // 1  one they defined
                              // APPEND HERE, NEVER INSERT ABOVE -- Kind is the index.
                              >;

    enum Kind : std::size_t {
        hand_designed = 0,
        user_defined = 1,
        how_many_kinds = 2
    };

    static_assert(std::variant_size_v<Held> == how_many_kinds, "Kind must name every arm of Held");

    Held held;

    satelliteSpacesuit() = default;
    satelliteSpacesuit(satelliteObject from) : held(std::move(from)) {}
    satelliteSpacesuit(UserDefinedHandle from) : held(std::move(from)) {}

    static satelliteSpacesuit of_object(satelliteObject from) { return satelliteSpacesuit(std::move(from)); }
    static satelliteSpacesuit of_user_defined(UserDefinedHandle from)
    {
        return satelliteSpacesuit(std::move(from));
    }

    Kind kind() const { return static_cast<Kind>(held.index()); }
    bool is_hand_designed() const { return held.index() == hand_designed; }
    bool is_user_defined() const { return held.index() == user_defined; }

    const satelliteObject *as_object() const { return std::get_if<satelliteObject>(&held); }
    satelliteObject *as_object() { return std::get_if<satelliteObject>(&held); }
    const UserDefinedHandle *as_user_defined() const { return std::get_if<UserDefinedHandle>(&held); }
    UserDefinedHandle *as_user_defined() { return std::get_if<UserDefinedHandle>(&held); }

    const char *kind_name() const
    {
        return is_user_defined() ? "an object" : (as_object() != nullptr ? as_object()->kind_name() : "nothing");
    }
};

// The pair of tags as one integer, exactly as satelliteObject's `pair_of` -- so a
// fast path between two spacesuits is named and found the same way as one
// between two built-ins.
inline constexpr std::size_t suit_pair_of(satelliteSpacesuit::Kind left, satelliteSpacesuit::Kind right)
{
    return (std::size_t)left * (std::size_t)satelliteSpacesuit::how_many_kinds + (std::size_t)right;
}

} // namespace satellite004
