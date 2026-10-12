// satellite-numbers/words/1.6.4.4.cpp -- satellite.variable.number.abs(a), built into satl
//
// satellite.variable.number.abs(a)   1 6 4 4
// The number without its sign: -7 is 7, and 7 is 7. 003 wrote it abs(a), with the receiver as a; as a method it
// takes nothing.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.abs()` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// number_methods.cpp's -- written 2026-09-16 for this word and never wired (in no make file, called by nothing)
// -- moved here whole the day that file was deleted.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::number_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    const satellite_number *number = the_number(receiver, name + ".satellite.abs", token::abs_token, context);
    if (number == nullptr)
        return Value();
    return Value::of_number(number->negative() ? -*number : *number);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_4(LibraryRow *row)
{
    row->name = "satellite.variable.number.abs(a)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 4;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
