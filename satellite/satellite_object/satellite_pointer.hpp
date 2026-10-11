#pragma once
// satellite/satellite_object/satellite_pointer.hpp -- A POINTER AT AN OBJECT, which does
// not keep the object living (the author, 2026-10-02).
//
// The author, 2026-10-02: *"we could make .pointer() be something that points at the
// object, and .reference() something that is a copy of the object, so the object can be
// emptied and the reference is still there, so the reference never goes empty, it's an
// exact copy, and the pointer() is something that just points at the object, doesn't keep
// the object living, if there's nothing else"*. The copy is object_copy.hpp's; this is
// the pointer. It is 2026-09-12's ruling for satellite 003 (MILESTONES/M26.md §2.1: "A
// pointer is spelled object_name.pointer() -- a method, not a keyword -- and it answers a
// weak reference"), built at last.
//
// AN OBJECT LIVES WHILE SOMETHING HOLDS IT -- a name, a field, an item of a list, a
// capsule running on it -- because an object is a std::shared_ptr and those are its
// handles (satellite_object.hpp). A pointer is a std::weak_ptr beside them: it reaches the
// object while anything else holds it and is counted by nothing, so a pointer back to a
// parent, or an object's pointer at itself, never keeps a circle of objects alive.
//
// WHAT IT IS, AS A VALUE, IS ITS SPACESUIT'S: `box p = a.pointer()` declares p a box, and
// every capsule of a box is called through it. So the layout travels with it -- a pointer
// whose object is gone is still a pointer at a box, and a name declared as some other
// spacesuit still refuses it.

#include <memory>

namespace satellite004 {

struct satelliteUserDefinedObject;
struct satelliteSuitLayout;

struct ObjectPointer {
    std::weak_ptr<satelliteUserDefinedObject> target;  // the object, while anything else holds it
    std::shared_ptr<const satelliteSuitLayout> layout; // its spacesuit, which outlives it
};

// THE OBJECT, held for as long as the caller keeps the answer -- or null when it is gone.
inline std::shared_ptr<satelliteUserDefinedObject> object_of(const ObjectPointer &pointer)
{
    return pointer.target.lock();
}

// TWO POINTERS ARE THE SAME WHEN THEY POINT AT THE SAME OBJECT, gone or not: an object's
// sameness is where it is (satellite_object.hpp's operator==), and a pointer keeps that
// much of it after the object is gone.
inline bool same_target(const ObjectPointer &left, const ObjectPointer &right)
{
    return !left.target.owner_before(right.target) && !right.target.owner_before(left.target);
}

} // namespace satellite004
