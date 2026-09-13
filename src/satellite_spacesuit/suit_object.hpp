#pragma once

// `satellite.spacesuit` -- M26, and the body behind satellite_value/value.hpp's
// twelfth arm.
//
// DESIGN §7 CALLS THIS "THE MOST IMPORTANT DECISION IN THE LANGUAGE" AND IT IS
// ABOUT THE OTHER HALF OF IT. §7.2's frames gave every capsule call its own
// storage; this is the storage that is NOT a call's -- an object's, shared by
// every method called on it and outliving all of them.
//
// A FIELD IS A SLOT AND A METHOD IS A PATH, which is resolve.hpp's sentence and
// is the whole of why PLAN §8 calls M26 "the largest feature in this list by
// everything except path count" and it still fits in a short file. Neither half
// needed a mechanism: DESIGN §7.2's storage decided before anything runs is
// what a field is, and §7.6's capsules in a table of their own is what a method
// is. What M26 adds is one vector of values and a layout to index it by.
//
// REFERENCE SEMANTICS, WHICH IS DESIGN §13 UNDER "Decided" AND §7.4's RECEIPT.
// §7.4: "a spacesuit is a reference type, so reusing the old slot would leave
// every handle already taken to the first instance pointing at the second --
// and a list built by that idiom would read back as n copies of its last
// element WITH NO ERROR ANYWHERE." So two names for one suit are one suit, the
// handle is NOT const, and this is the third arm in the variant on that side of
// the line after the file (M19) and the thread (M23).
//
// THE CYCLE LEAK IS REAL, IT IS KNOWN, AND THE AUTHOR SETTLED WHAT TO DO ABOUT
// IT ON 2026-09-12. DESIGN §12 conceded the argument before this milestone
// existed: refcounting sufficed while every value was immutable, "a spacesuit
// instance is mutable, so two objects can name each other and neither is ever
// freed. This is a real leak, it is the price of reference semantics, and it is
// not fixable by being careful."
//
// The decision: **refcount now, with `.pointer()` as the weak reference, and a
// cycle collector as its own milestone.** Two things make that honest rather
// than a deferral:
//
//   1. `.pointer()` IS §12's "weak-reference field" answer, arriving under a
//      better name -- a back-reference taken through it does not keep its
//      target alive, so a cycle is breakable in the language today.
//   2. What is missing is only that a program has to KNOW to use it. The
//      complete answer is not a tracing collector over everything, which would
//      be a stop-the-world pass DESIGN §1.1 could object to; it is TRIAL
//      DELETION over the objects refcounting has already flagged as suspicious
//      -- an object can only be in a garbage cycle if its count was decremented
//      and did not reach zero. That is a small, bounded, explicable pass, and
//      it is a milestone rather than a paragraph.
//
// MILESTONES/M26.md §4 carries this as the thing left open, with the author's
// decision attached, so nobody discovers it.

#include "satellite_thread/access_list.hpp"
#include "satellite_value/value.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace satellite::suit {

// WHAT EVERY OBJECT OF ONE SPACESUIT SHARES: how many fields it has, what they
// are called, and which of them a program outside may reach.
//
// ONE LAYOUT PER SUIT PER RUN, HELD BY THE COMPILED PROGRAM and pointed at by
// every object of it. It is the same arrangement `evaluator/closure.hpp` gives
// a capsule -- decided once, before anything runs, read many times -- and it is
// why an object is a vector of values and nothing else.
//
// THE NAMES ARE HERE FOR THE SENTENCES AND NOT FOR THE WALK. Nothing looks a
// field up by name at run time; resolve turned every field read into an index
// before the program started, which is DESIGN §7.1's entire point one level up
// from where it was made. What the names are for is S0516's refusal and the
// renderer, both of which are read by a person.
struct Layout {
    std::string name;
    std::vector<std::string> field_names;
    std::vector<bool> field_is_public;

    size_t fields() const { return field_names.size(); }
};

// ONE INSTANCE.
//
// NOT const BEHIND ITS HANDLE, which is the file's and the thread's side of
// satellite_value/value.hpp's line and is DESIGN §7.4's requirement rather than
// a convenience: a spacesuit is a reference type, so a method that changes a
// field changes it for every name that holds the object. Writing it
// `shared_ptr<const SuitObject>` with mutable fields inside would be the same
// semantics with a `const` that lied about them.
//
// THE LAYOUT IS A BARE POINTER AND THAT IS SAFE FOR THE ARENA'S REASON. It
// points into the compiled program, which outlives every value in the run --
// the same relationship `evaluator/machine.hpp` has with `Compiled`, and the
// same one M23's thread handle has. A shared_ptr here would be a refcount on
// something that cannot die first.
struct SuitObject {
    const Layout *layout = nullptr;
    std::vector<Value> fields;

    // THE HOLD -- THREAD.md D1 and D2. Two threads sharing one object wrote
    // one 40-byte Value at once and freed a string twice, and a mutating
    // method's take-out (operations_dispatch.cpp) left a field holding nothing
    // for another thread to read. So every field read and write takes this,
    // and a mutating method on a field keeps it from the take-out through the
    // write-back -- which is also what keeps the in-place fast path: nobody
    // else can reach the body while its count is one.
    //
    // TAKEN ONLY ONCE THE RUN HAS A SECOND THREAD (Globals::shared()), so a
    // program with no threads pays nothing but that one relaxed load.
    //
    // RECURSIVE FOR ONE CALLER: the renderer. A handler holding this object may
    // render the object itself -- a refusal quoting its argument -- and the
    // same thread must not wait for itself.
    mutable std::recursive_mutex hold;

    // THIS OBJECT ON A THREAD'S ACCESS LIST -- THREAD.md T2. Held for a whole
    // method call (Machine::enter / unwind), so every method is one step to
    // every other thread. `hold` above stays for the renderer, which reads an
    // object from outside any method.
    thread::Access access;

    // DESTROYED ONE LEVEL AT A TIME -- THREAD.md D19, value.hpp's Burial. A
    // chain of objects each holding the next is a linked list, and freeing a
    // long one recursed once per object.
    ~SuitObject()
    {
        Burial burial;
        for (Value &field : fields)
            burial.add(field);
    }
};

// HOW MANY OBJECT HOLDS THIS THREAD HAS OPEN ACROSS A HANDLER. Only the
// renderer reads it: a thread holding one object that renders another tries
// that one's hold instead of waiting for it, because the thread holding the
// other may be rendering this one. Two holds, taken in opposite orders, would
// be a hang -- THREAD.md §1's second rule.
inline thread_local int holds_open = 0;

// One hold kept across a handler, counted for the renderer above.
class HandlerHold {
public:
    explicit HandlerHold(const SuitObject *object)
    {
        if (object == nullptr)
            return;
        lock_ = std::unique_lock<std::recursive_mutex>(object->hold);
        holds_open++;
    }
    ~HandlerHold()
    {
        if (lock_.owns_lock())
            holds_open--;
    }
    HandlerHold(const HandlerHold &) = delete;
    HandlerHold &operator=(const HandlerHold &) = delete;

private:
    std::unique_lock<std::recursive_mutex> lock_;
};

} // namespace satellite::suit
