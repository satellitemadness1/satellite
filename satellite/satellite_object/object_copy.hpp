#pragma once
// satellite/satellite_object/object_copy.hpp -- AN EXACT COPY OF AN OBJECT, which is what
// object.reference() answers (the author, 2026-10-02).
//
// The author: *".reference() something that is a copy of the object, so the object can be
// emptied and the reference is still there, so the reference never goes empty, it's an exact
// copy"*. `b = a` on an object gives b the SAME object (a spacesuit is a reference type,
// satellite_object.hpp), so this is the one way to have a second one.
//
// EXACT, ALL THE WAY DOWN. Every object the copied one holds -- in a field, in a list, in a
// map, at any depth -- is copied too, once each: two fields holding one object hold one copy,
// and an object that holds itself holds its copy. Nothing in the copy is the original's, so
// emptying the original, or changing it, never reaches the reference.
//
// WHAT IS NOT COPIED, BECAUSE `=` DOES NOT COPY IT EITHER:
//   - a list or a map holding no object: it is a value already, shared until one side writes
//     to it (satellite_list.hpp's copy-on-write), so a reference costs nothing for it;
//   - a file, a window, a thread or a program: each is one thing outside satellite, and every
//     name for it is a name for that one;
//   - a POINTER keeps pointing where it pointed -- EXCEPT at an object the copy also copied,
//     where it points at that object's copy instead, so a copied tree's pointers back to its
//     own parents point into the copy and not into the original (satellite_pointer.hpp).
//
// THE AUTHOR'S LOCK IS KEPT (object_lock.hpp): each original is read under its lock when the
// lock is on, as any statement that reads it would be; and each copy's lock is on or off as the
// original's was.
//
// NO DEPTH LIMIT: the objects are walked from a list of work, never by the copy calling itself,
// so a chain of a million objects each holding the next is copied like any other.

#include "satellite_object.hpp"

namespace satellite004 {

// The copy through `copy`, and success -- or wait_never_ends (S728) when an original's lock
// could never be taken, and then `copy` is null.
signed long long int exact_copy_of(const UserDefinedHandle &object, UserDefinedHandle &copy);

} // namespace satellite004
