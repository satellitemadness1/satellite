#pragma once
// satellite/satellite_object/satellite_capsule.hpp -- a capsule as a VALUE.
//
// (the author, 2026-09-16) "we need satelliteUserDefinedObject to have spots for
// satelliteCapsule and stuff, like, satelliteCapsule must hold the
// satelliteBytecode thing".
//
// SO A CAPSULE HOLDS ITS BYTECODE. That is the author's ruling and this file is
// it: `body` is a satellite_bytecode, by value, and a capsule handed around
// carries its own codes with it.
//
// WHAT THIS DOES NOT REPLACE, and the distinction is worth keeping straight
// because both are right. The WALKER still runs a program out of the shared
// BytecodeRegistry through a CapsuleSite of two integers -- `{which row, where
// the brace was}` -- and nothing is allocated to call one. That path is measured
// and it stays. THIS type is a capsule the OBJECT MODEL can hold: one that a
// user's class owns as a method, that can be stored in a variable, passed, and
// built at run time by a program that writes satellite (which is what QUAD does).
// A site is how the walker reaches a capsule that is already in the program; a
// satelliteCapsule is a capsule that is a value in its own right.
//
// `parameters` ARE NAMES, NOT TYPES, and that is the object model's whole
// difficulty in one line -- the author: "building the satellite object model will
// be very very hard to do, given that names are simply strings". A parameter's
// type is known only where the caller's argument meets it, which is why nothing
// here records one yet.

#include "satellite_bytecode.hpp"

#include <string>
#include <vector>

namespace satellite004 {

struct satelliteCapsule {
    std::string name;                        // satellite.main, or a name the user invented
    std::vector<std::string> parameters;     // in written order
    satellite_bytecode body;                 // the author: a capsule HOLDS the bytecode

    satelliteCapsule() = default;
    explicit satelliteCapsule(std::string its_name) : name(std::move(its_name)) {}

    bool empty() const { return body.empty(); }
    std::size_t takes() const { return parameters.size(); }

    // Two capsules are the same capsule when they have the same name, the same
    // parameters and the same codes. Compared because the variant's arms are
    // compared, not because a program is expected to ask often.
    friend bool operator==(const satelliteCapsule &l, const satelliteCapsule &r)
    {
        return l.name == r.name && l.parameters == r.parameters && l.body == r.body;
    }
    friend bool operator!=(const satelliteCapsule &l, const satelliteCapsule &r) { return !(l == r); }
};

} // namespace satellite004
