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
// built on that line). satelliteUserDefinedObject holds `body` -- the codes its
// declaration is made of -- and nothing here re-derives what the registry
// already has.
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
// ONE THEY INSERT THE PIECES INTO (the author). Its codes, its fields, and the
// spots for satelliteCapsule.
// ---------------------------------------------------------------------------
//
// FIELDS ARE TWO PARALLEL VECTORS, name beside value, and the name is resolved
// ONCE to a slot. A map per instance is what makes an interpreter slow -- 004
// already loses 5x to CPython on `i = i + 1` for that exact reason -- so the
// string compare happens in `slot_of` and everything on a hot path holds the
// integer instead.
struct satelliteUserDefinedObject {
    std::string name;
    satellite_bytecode body;                 // a spacesuit is just a collection of bytecode
    std::vector<std::string> field_names;    // name -> slot
    std::vector<satelliteObject> fields;     // one per name, same order
    std::vector<satelliteCapsule> capsules;  // the spots for methods

    satelliteUserDefinedObject() = default;
    explicit satelliteUserDefinedObject(std::string its_name) : name(std::move(its_name)) {}

    std::size_t how_many_fields() const { return field_names.size(); }
    std::size_t how_many_capsules() const { return capsules.size(); }

    // THE STRING COMPARE HAPPENS HERE AND NOWHERE ELSE.
    std::size_t slot_of(const std::string &field) const
    {
        for (std::size_t i = 0; i < field_names.size(); ++i)
            if (field_names[i] == field)
                return i;
        return (std::size_t)-1;
    }

    // The fast way in: an integer index, no compare and no hash.
    const satelliteObject *field_at(std::size_t slot) const
    {
        return slot < fields.size() ? &fields[slot] : nullptr;
    }
    satelliteObject *field_at(std::size_t slot)
    {
        return slot < fields.size() ? &fields[slot] : nullptr;
    }

    // Answers the slot it went into, so whatever is building keeps the integer.
    std::size_t insert_field(std::string field, satelliteObject held)
    {
        field_names.push_back(std::move(field));
        fields.push_back(std::move(held));
        return field_names.size() - 1;
    }

    const satelliteCapsule *capsule_of(const std::string &its_name) const
    {
        for (std::size_t i = 0; i < capsules.size(); ++i)
            if (capsules[i].name == its_name)
                return &capsules[i];
        return nullptr;
    }
    void insert_capsule(satelliteCapsule its_capsule) { capsules.push_back(std::move(its_capsule)); }
};

inline UserDefinedHandle make_user_defined(std::string name)
{
    return std::make_shared<satelliteUserDefinedObject>(std::move(name));
}

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
