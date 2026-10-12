// satellite-numbers/words/1.10.4.cpp -- satellite.spacesuit.lock(), built into satl
//
// satellite.spacesuit.lock()   1 10 4
// Turns the object's lock switch ON (satellite_object/object_lock.hpp, THREADS.md T2). It locks nothing
// itself: from then on every statement that writes the object holds it, one at a time. Answers nothing.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we have to give everything a number and then make it
// run those numbers"; "everything builtin needs the word satellite still"): `obj.satellite.lock()` reaches
// the function below through the function table -- bytecode/pointer_calls.cpp's run_object_number, which
// has refused anything in its brackets and holds the object (a pointer's too) for the call -- and the code
// in it is what call_member ran for .lock() until that day, moved here whole.
// THE SAME WORD ON A FILE IS NOT THIS ONE: a file's .lock() is its own (bytecode/expression.cpp's file
// branch), told apart by what the receiver holds.

#include "../number_row.hpp"
#include "../../satellite/bytecode/pointer_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_spacesuit.hpp"

#include <atomic>

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    // THE AUTHOR'S LOCK (satellite_object/object_lock.hpp): `.satellite.lock()` turns it on. It locks
    // nothing itself; a statement that writes the object does.
    object_behind(receiver)->lock.on.store(true, std::memory_order_release);
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_10_4(LibraryRow *row)
{
    row->name = "satellite.spacesuit.lock()";
    row->numbers[0] = 1;
    row->numbers[1] = 10;
    row->numbers[2] = 4;
    row->depth = 3;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
