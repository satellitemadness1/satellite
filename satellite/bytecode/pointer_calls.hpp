#pragma once
// satellite/bytecode/pointer_calls.hpp -- WHAT EVERY OBJECT ANSWERS, whatever its spacesuit
// declares, and how a pointer reaches its object (the author, 2026-10-02).
//
//     box p = a.pointer()       points at a's object and does not keep it living
//     box r = a.reference()     an exact copy of a's object, its own from then on
//     p.ok()                    whether p still points at an object; true of an object itself
//
// The author: *".pointer() be something that points at the object, and .reference()
// something that is a copy of the object, so the object can be emptied and the reference is
// still there"*. satellite_object/satellite_pointer.hpp is the pointer and
// satellite_object/object_copy.hpp the copy; this file is the three methods.
//
// EVERY OTHER MEMBER OF A POINTER IS ITS OBJECT'S: `p.call_take()` runs the capsule on the
// object p points at, held for the whole call, and `p.lock()` locks that object. A pointer
// whose object is gone answers only these three -- p.ok() is false, p.pointer() is p again,
// and anything else, p.reference() included, stops with S260 OBJECT_IS_GONE.
//
// THE LANGUAGE'S WORDS COME FIRST, as .lock() and .unlock() do (object_lock.hpp): a
// spacesuit's own capsule named pointer, reference or ok is still called by its bare name
// inside the spacesuit, and `x.pointer()` from anywhere is the language's.

#include "expression.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

inline bool every_object_answers(token::Code method)
{
    return method == token::pointer_token || method == token::reference_token || method == token::ok_token;
}

// THE OBJECT A VALUE STANDS FOR -- itself, or the one its pointer points at -- held for as
// long as the answer is kept. Null when there is none: the value is not an object, or its
// pointer's object is gone. Inline, because every capsule called on an object asks it.
inline UserDefinedHandle object_behind(const Value &value)
{
    if (const UserDefinedHandle *object = value.as_user_defined())
        return *object;
    if (const ObjectPointer *pointer = value.as_pointer())
        return object_of(*pointer);
    return UserDefinedHandle();
}

// `obj.pointer()`, `obj.reference()`, `obj.ok()` on an object or a pointer at one. `at` is on
// the `(` and is left past the `)`; `receiver` is how the line wrote what is before the dot,
// and `dot` is where a refusal points.
Value every_object_method(const std::vector<std::bitset<16>> &row, std::size_t &at, token::Code method,
                          const Value &value, const std::string &receiver, std::size_t dot,
                          ExpressionContext &context);

// THE SENTENCE FOR A POINTER WHOSE OBJECT IS GONE, the same wherever one is used.
std::string object_is_gone_because(const std::string &written, const std::string &receiver);

} // namespace satellite004
