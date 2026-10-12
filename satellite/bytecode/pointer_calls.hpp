#pragma once
// satellite/bytecode/pointer_calls.hpp -- WHAT EVERY OBJECT ANSWERS, whatever its spacesuit
// declares, and how a pointer reaches its object (the author, 2026-10-02).
//
//     box p = a.satellite.pointer()      points at a's object and does not keep it living
//     box r = a.satellite.reference()    an exact copy of a's object, its own from then on
//     p.satellite.ok()                   whether p still points at an object; true of an object itself
//     a.satellite.lock()                 its lock switch on, and .satellite.unlock() off (object_lock.hpp)
//
// The author: *".pointer() be something that points at the object, and .reference()
// something that is a copy of the object, so the object can be emptied and the reference is
// still there"*. satellite_object/satellite_pointer.hpp is the pointer and
// satellite_object/object_copy.hpp the copy; this file is the five words.
//
// THE FIVE ARE NUMBERED AND WRITTEN UNDER SATELLITE (NM-8, 2026-10-11): satellite.spacesuit.pointer(),
// .reference(), .ok(), .lock() and .unlock() are 1 10 1 to 1 10 5, each a file under
// satellite-numbers/words/ that run_object_number reaches through the function table. The author's A of
// 2026-10-11 named .pointer() and .reference() -- they "become .satellite.pointer() /
// .satellite.reference()" -- and "everything builtin needs the word satellite still" is the other three.
//
// HIS OWN CAPSULES KEEP THEIR `.` (his C of 2026-10-10: only the language's own words take satellite).
// So after a bare `.`, a name his spacesuit declares is HIS capsule, whatever it is called: `d.lock()`
// runs a spacesuit's own lock() -- until this day the language's took its place from outside (ERRORS4
// #005) -- and `d.satellite.lock()` is always the language's. One of the five written with a bare `.`,
// on an object whose spacesuit declares no capsule of that name, is refused, naming the new spelling.
// Inside the spacesuit a capsule's bare name -- `lock()` with no object before it -- is his, as ever.
//
// EVERY OTHER MEMBER OF A POINTER IS ITS OBJECT'S: `p.call_take()` runs the capsule on the
// object p points at, held for the whole call, and `p.satellite.lock()` locks that object. A pointer
// whose object is gone answers only pointer() and ok() -- p.satellite.ok() is false,
// p.satellite.pointer() is p again -- and anything else, reference() included, stops with S260
// OBJECT_IS_GONE.

#include "expression.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// THE THREE A POINTER IS ASKED BEFORE ITS OBJECT IS HELD: a pointer whose object is gone still
// answers ok() and pointer(), and reference() says itself that the object is gone.
inline bool every_object_answers(token::Code method)
{
    return method == token::pointer_token || method == token::reference_token || method == token::ok_token;
}

// THE FIVE WORDS EVERY OBJECT HAS, BY THEIR SPELLING -- these three, and .lock() and .unlock()
// (object_lock.hpp). A name read as text -- satellite.thread.new's capsule call, read by
// dotted_names_at -- is one of them when it is spelled so, and is then the language's unless the
// object's spacesuit declares a capsule of that name (the review of 2026-10-02: a thread ran the
// spacesuit's own pointer() where every other line meant the language's; since 2026-10-11 a bare
// `.` means his own everywhere, and `.satellite.` the language's).
inline bool every_object_word(const std::string &spelling)
{
    return spelling == "pointer" || spelling == "reference" || spelling == "ok" || spelling == "lock" ||
           spelling == "unlock";
}

// THE SENTENCE FOR ONE OF THEM HANDED TO satellite.thread.new, before the run and at it -- and for any
// other word written under satellite there (NM-8): `s.satellite.upper()` is the language's too.
inline std::string not_a_capsule_for_a_thread(const std::string &written, const std::string &word)
{
    return "satellite.thread.new runs a capsule of your own on a thread, and " + written + "() is not one -- " +
           word + (every_object_word(word) ? "() is every object's own word" : "() is the language's own word");
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

// THE NUMBER ONE OF THE FIVE RUNS -- the code of satellite.spacesuit.<it>(), 1 10 n (the author,
// 2026-10-11: "we have to give everything a number and then make it run those numbers") -- or 0 for
// anything else, which an object has only as a capsule of its spacesuit. method_periods.hpp asks it for
// an object and for a pointer at one.
token::Code object_number_of(token::Code method);

// ONE OF THE FIVE, RUN BY ITS NUMBER: `object.satellite.ok()` on an object or a pointer at one. `at` is
// on the `(` and is left past the `)`; `receiver` is how the line wrote what is before the period, and
// `dot` is where a refusal points. Anything in the brackets is refused first, as it always was; a pointer
// is asked pointer(), ok() and reference() BEFORE its object is held; anything else holds the object
// for the call, and is refused when there is none -- a pointer's that is gone (S260), a field's that was
// never given one. `number` is object_number_of(method), already asked, never 0.
Value run_object_number(token::Code method, token::Code number, const std::vector<std::bitset<16>> &row,
                        std::size_t &at, const Value &object, const std::string &receiver, std::size_t dot,
                        ExpressionContext &context);

// THE SENTENCE FOR A POINTER WHOSE OBJECT IS GONE, the same wherever one is used.
std::string object_is_gone_because(const std::string &written, const std::string &receiver);

} // namespace satellite004
