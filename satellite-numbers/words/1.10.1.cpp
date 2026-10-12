// satellite-numbers/words/1.10.1.cpp -- satellite.spacesuit.pointer(), built into satl
//
// satellite.spacesuit.pointer()   1 10 1
// A pointer at the same object, which does not keep it living (satellite_object/satellite_pointer.hpp),
// typed as its spacesuit: `box p = a.satellite.pointer()`. On a pointer, that same pointer, gone or not.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we have to give everything a number and then make it
// run those numbers"; and that day, his A: .pointer() becomes .satellite.pointer()): `a.satellite.pointer()`
// reaches the function below through the function table -- bytecode/pointer_calls.cpp's run_object_number,
// which has refused anything in its brackets first, and asks a pointer this BEFORE its object is held -- and
// the code in it is what every_object_method ran for pointer() until that day, moved here whole.
// The word is new that day: every object has had it since 2026-10-02, with no number until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/pointer_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    // pointer(): at the same object, and a pointer's pointer is the pointer itself -- gone or not.
    if (const ObjectPointer *pointer = receiver.as_pointer())
        return Value(*pointer);
    const UserDefinedHandle object = object_behind(receiver);
    return Value(ObjectPointer{object, object->layout});
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_10_1(LibraryRow *row)
{
    row->name = "satellite.spacesuit.pointer()";
    row->numbers[0] = 1;
    row->numbers[1] = 10;
    row->numbers[2] = 1;
    row->depth = 3;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
