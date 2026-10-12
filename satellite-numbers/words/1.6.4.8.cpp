// satellite-numbers/words/1.6.4.8.cpp -- satellite.variable.number.ceil, built into satl
//
// satellite.variable.number.ceil   1 6 4 8
// Rounds up, toward the larger number -- and a satellite.variable.number is a whole number, so it answers the
// number unchanged: n.satellite.ceil() is n.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.ceil()` reaches the function below through the function table --
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
    // A WHOLE NUMBER IS ALREADY CEILED. satellite.variable.number holds whole numbers, and 003 DESIGN 8.6 puts
    // floor, ceil, round and trunc in its "exact and bounded" class: the ceiling of a value with no fraction is
    // that value -- so it answers itself. That is the rule and not a stub: a program written for a float can be
    // moved to a number without its arithmetic quietly changing meaning.
    const satellite_number *number = the_number(receiver, name + ".satellite.ceil", token::ceil_token, context);
    if (number == nullptr)
        return Value();
    return Value::of_number(*number);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_8(LibraryRow *row)
{
    row->name = "satellite.variable.number.ceil";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 8;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
