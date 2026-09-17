#pragma once
// satellite/satellite_object/satellite_spacesuit.hpp -- THE CLASS AND THE
// INSTANCE, which are two things and not one.
//
// (the author, 2026-09-16) "this is our spacesuit, and a user defined spacesuit,
// NOT something else, this is satelliteSpacesuit not satelliteObject!"
//
// He was right, and the split runs deeper than the name. What was written first
// held the class's field NAMES and one instance's field VALUES in a single
// struct -- so every instance of a class carried its own copy of the layout. The
// language's own vocabulary says they are two words, and 003 had already built
// them as two structs (src/satellite_spacesuit/suit_object.hpp):
//
//     satelliteSpacesuit   the CLASS. One per `satellite.spacesuit` the user
//                          declares. Shared by every instance, never copied.
//                          003 calls this `Layout`.
//     satelliteObject      ONE INSTANCE. A pointer to its spacesuit, and a flat
//                          vector of its own field values. 003 calls this
//                          `SuitObject`.
//
// FIELDS ARE A FLAT VECTOR INDEXED BY SLOT, NOT A MAP, and this is the decision
// worth defending hardest. A map from name to value PER INSTANCE is what makes
// an interpreter slow: 004 already loses 5x to CPython on `i = i + 1` for that
// exact reason -- it rebuilds a std::string from 16-bit codes and hashes it on
// every evaluation, where CPython indexes an array slot. A class's field names
// are the same for every instance of it, so the NAME is resolved ONCE to a slot
// and the slot is an integer from then on. `slot_of` is the slow way in and
// belongs at check time; `field_at` is what runs in a loop.
//
// THE SPACESUIT IS A BARE POINTER AND THAT IS SAFE, for 003's reason exactly: it
// points into the compiled program, which outlives every value in the run. A
// shared_ptr there would be a refcount on something that cannot die first.
//
// OWED, AND NAMED SO IT IS NOT DISCOVERED LATER:
//   - THREADS. 003's SuitObject carries a recursive_mutex and a thread access
//     list (THREAD.md D1, D2) because two threads sharing one object wrote one
//     Value at once and freed a string twice. 004's walker is single-threaded
//     today, so nothing is built here yet -- but 256 threads are already warm
//     and this is where that bill arrives.
//   - DEEP CHAINS DESTRUCT RECURSIVELY. A chain of objects each holding the next
//     is a linked list, and freeing a long one recurses once per object. 003 hit
//     this and answered it with a staged burial (value.hpp's `Burial`). Not
//     built here; it is the same shape of hazard as the walker's own depth.

#include "satellite_value.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace satellite004 {

// ---------------------------------------------------------------------------
// THE CLASS. `satellite.spacesuit` is word `1 10` and `satellite.class` is the
// author's second spelling of it (2026-09-09).
// ---------------------------------------------------------------------------
struct satelliteSpacesuit {
    std::string name;
    std::vector<std::string> field_names;    // the layout: name -> slot
    std::vector<bool> field_is_public;       // satellite.protected / satellite.public
    std::vector<satelliteCapsule> capsules;  // the methods -- "spots for satelliteCapsule"

    satelliteSpacesuit() = default;
    explicit satelliteSpacesuit(std::string its_name) : name(std::move(its_name)) {}

    std::size_t fields() const { return field_names.size(); }
    std::size_t how_many_capsules() const { return capsules.size(); }

    // THE STRING COMPARE HAPPENS HERE AND NOWHERE ELSE. npos for a name this
    // spacesuit does not declare.
    std::size_t slot_of(const std::string &field) const
    {
        for (std::size_t i = 0; i < field_names.size(); ++i)
            if (field_names[i] == field)
                return i;
        return (std::size_t)-1;
    }

    bool is_public(std::size_t slot) const
    {
        return slot < field_is_public.size() && field_is_public[slot];
    }

    const satelliteCapsule *capsule_of(const std::string &its_name) const
    {
        for (std::size_t i = 0; i < capsules.size(); ++i)
            if (capsules[i].name == its_name)
                return &capsules[i];
        return nullptr;
    }

    // Declaring a field. Answers the slot, so whatever is building the class
    // keeps the integer and never looks the name up again.
    std::size_t declare_field(std::string field, bool public_field)
    {
        field_names.push_back(std::move(field));
        field_is_public.push_back(public_field);
        return field_names.size() - 1;
    }
    void declare_capsule(satelliteCapsule its_capsule) { capsules.push_back(std::move(its_capsule)); }
};

// ---------------------------------------------------------------------------
// ONE INSTANCE. "what a constructor produces is the object" -- DESIGN 13.
// ---------------------------------------------------------------------------
//
// NOT const BEHIND ITS HANDLE, which is DESIGN 7.4's requirement and not a
// convenience: a spacesuit is a reference type, so a method that changes a field
// changes it for every name that holds the object.
struct satelliteObject {
    const satelliteSpacesuit *suit = nullptr;   // the class, pointed at, never copied
    std::vector<satelliteValue> fields;         // this instance's own values, by slot

    satelliteObject() = default;

    // An instance of `its_suit`, every field `nothing` until something writes
    // it. The fields are sized from the class, so a slot is always in range.
    explicit satelliteObject(const satelliteSpacesuit *its_suit)
        : suit(its_suit), fields(its_suit == nullptr ? 0 : its_suit->fields())
    {
    }

    const std::string &class_name() const
    {
        static const std::string none;
        return suit == nullptr ? none : suit->name;
    }

    // THE FAST WAY IN: an integer index, no compare and no hash.
    const satelliteValue *field_at(std::size_t slot) const
    {
        return slot < fields.size() ? &fields[slot] : nullptr;
    }
    satelliteValue *field_at(std::size_t slot)
    {
        return slot < fields.size() ? &fields[slot] : nullptr;
    }

    // The slow way, for a name met for the first time. npos if this object's
    // spacesuit does not declare it.
    std::size_t slot_of(const std::string &field) const
    {
        return suit == nullptr ? (std::size_t)-1 : suit->slot_of(field);
    }
};

// One new object of a spacesuit, as the handle every value holds. This is the
// only place an instance is made, so the refcount begins in one place.
inline SpacesuitHandle make_object(const satelliteSpacesuit *suit)
{
    return std::make_shared<satelliteObject>(suit);
}

} // namespace satellite004
