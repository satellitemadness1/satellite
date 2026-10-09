#pragma once
// satellite/bytecode/suit_layout.hpp -- WHAT EVERY OBJECT OF ONE SPACESUIT SHARES.
//
// A SPACESUIT IS DECLARED ONCE AND MADE MANY TIMES, so what does not change from one
// object to the next is built once, here, when the scan finds the declaration
// (capsule_scopes.cpp), and every object points at it: its name, which scope it is,
// and each field's name and declared type. An object holds nothing but that pointer
// and its own values (satellite_object/satellite_spacesuit.hpp). 003 built the same
// split and called it the Layout (src/satellite_spacesuit/suit_object.hpp); 004's
// first object copied the whole declaration into every object instead.
//
// WHAT IS NOT HERE, AND WHERE IT IS. A spacesuit's capsules are CapsuleSites in the
// scope table, found by name through its scope, exactly as a file's are -- a method is
// a capsule that runs with an object, not a second kind of thing to keep beside it.
// Its constructor is one of those sites. Its field initialisers are statements in its
// file's row, run where they stand each time an object is made.

#include "type_shape.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace satellite004 {

inline constexpr std::size_t kNoSlot = static_cast<std::size_t>(-1);

// ONE FIELD, as a line in satellite.protected (or satellite.public) declares it.
struct SuitField {
    std::string name;
    TypeShape shape;            // a word and its <>, or another spacesuit
    std::size_t row = 0;        // the file its declaring statement is in -- a supertype's may be another
    std::size_t at = 0;         // where that statement starts, in that row
    bool is_public = false;     // declared in satellite.public (it changes nothing: fields are reached inside only)
    bool made = false;          // a spacesuit's type WITH brackets: every object makes one of these
};

struct satelliteSuitLayout {
    std::string name;           // as its declaration writes it: run_log
    std::string shown;          // as a sentence names it: tagged_report.run_log, ship.engine
    std::size_t suit = kNoSuit; // its scope in the program's CapsuleTable
    std::size_t row = 0;        // the file it is declared in

    // ITS FIELDS, ITS SUPERTYPE'S FIRST (003's rule: "parent fields first"), so a slot a
    // supertype's capsule reads is the same slot in every spacesuit that extends it.
    std::vector<SuitField> fields;
    std::size_t own_fields = 0; // where its own begin in `fields`, after its supertypes'

    // ITSELF, THEN ITS SUPERTYPE, THEN THAT ONE'S, as far as they go -- what an object of
    // it IS, and so what a name declared as any of them may hold.
    std::vector<std::size_t> lineage;

    bool is_a(std::size_t other) const
    {
        for (const std::size_t each : lineage)
            if (each == other)
                return true;
        return other == suit;
    }

    // THE STRING COMPARE HAPPENS HERE AND NOWHERE ELSE: a name is turned into a slot,
    // and an object is reached by the slot.
    //
    // FROM THE END, AND ONLY AS FAR AS `seen` REACHES. A spacesuit may declare again a
    // field its supertype has -- every one of the author's creator_data subtypes has its
    // own `spacesuit_name` -- and then there are two slots of one name, as in 003: a
    // supertype's capsule reads the supertype's, and the subtype's capsule its own. A
    // capsule of a spacesuit sees the first `seen` fields, which are that spacesuit's
    // (its supertypes' come first), and the last of a name among them is the one it means.
    std::size_t slot_of(const std::string &field, std::size_t seen = kNoSlot) const
    {
        for (std::size_t i = seen < fields.size() ? seen : fields.size(); i > 0; --i)
            if (fields[i - 1].name == field)
                return i - 1;
        return kNoSlot;
    }
};

// A NEW OBJECT: every field nothing until its initialiser runs (suit_run.cpp).
inline UserDefinedHandle make_object(const std::shared_ptr<const satelliteSuitLayout> &layout)
{
    UserDefinedHandle made = std::make_shared<satelliteUserDefinedObject>();
    made->layout = layout;
    made->fields.resize(layout->fields.size());
    return made;
}

} // namespace satellite004
