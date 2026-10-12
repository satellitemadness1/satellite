// satellite-numbers/words/1.6.4.3.cpp -- satellite.variable.number.min(a, b), built into satl
//
// satellite.variable.number.min(a, b)   1 6 4 3
// The smaller of the number and x: n.satellite.min(x). 003 wrote it min(a, b), with the receiver as a; as a
// method it takes ONE argument, and a refusal names it as the program wrote it. A list's .min is another word (1
// 4 2 25), which takes nothing.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.min(x)` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// number_methods.cpp's -- written 2026-09-16 for this word and never wired (in no make file, called by nothing)
// -- moved here whole the day that file was deleted.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_variable_number/number_arithmetic.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::number_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const std::string call = name + ".satellite.min";
    const satellite_number *number = the_number(receiver, call, token::min_token, context);
    if (number == nullptr)
        return Value();
    const satellite_number *other = number_argument(arguments[0], call, "a number", context);
    if (other == nullptr)
        return Value();
    // ONE COMPARISON, the one < and > read (number_fast_path::compare); of two that are equal it is the
    // number it was called on -- they are the same number.
    const int order = number_fast_path::compare(*number, *other);
    return Value::of_number(order <= 0 ? *number : *other);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_3(LibraryRow *row)
{
    row->name = "satellite.variable.number.min(a, b)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
