// satellite-numbers/words/1.10.3.cpp -- satellite.spacesuit.ok(), built into satl
//
// satellite.spacesuit.ok()   1 10 3
// A bool: true of an object, and of a pointer whose object is still there; false of a pointer whose
// object is gone -- the one way a program asks before it reaches through one.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we have to give everything a number and then make it
// run those numbers"; "everything builtin needs the word satellite still"): `p.satellite.ok()` reaches the
// function below through the function table -- bytecode/pointer_calls.cpp's run_object_number, which has
// refused anything in its brackets first, and asks a pointer this BEFORE its object is held -- and the
// code in it is what every_object_method ran for ok() until that day, moved here whole.
// THE SAME WORD ON A FILE, A PROGRAM OR A WINDOW IS NOT THIS ONE: each type's .ok() is its own number
// (a file's is 1 6 2 8), told apart by what the receiver holds.

#include "../number_row.hpp"
#include "../../satellite/bytecode/pointer_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    // ok(): is there an object here. A pointer whose object is gone is the one way to say no.
    return Value(object_behind(receiver) != nullptr);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_10_3(LibraryRow *row)
{
    row->name = "satellite.spacesuit.ok()";
    row->numbers[0] = 1;
    row->numbers[1] = 10;
    row->numbers[2] = 3;
    row->depth = 3;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
