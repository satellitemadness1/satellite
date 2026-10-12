// satellite-numbers/words/1.6.4.1.cpp -- satellite.variable.number.shift_left(n), built into satl
//
// satellite.variable.number.shift_left(n)   1 6 4 1
// The number times 2 to the power n. A number here has no width, so nothing falls off the top and a shift left
// only makes it larger; a negative n is refused. One argument: the receiver is 003's own first one.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.shift_left(3)` reaches the function below through the function table --
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
    const std::string call = name + ".satellite.shift_left";
    const satellite_number *number = the_number(receiver, call, token::shift_left_method_token, context);
    if (number == nullptr)
        return Value();
    const satellite_number *places = number_argument(arguments[0], call, "a number", context);
    if (places == nullptr)
        return Value();
    if (places->negative()) {
        context.refuse(not_a_position, call + " was given " + places->to_text() + ", and a shift is never negative");
        return Value();
    }
    // A SHIFT IS A MULTIPLICATION BY A POWER OF TWO -- 003 DESIGN 8.6 gave the spelling that meaning on
    // 2026-08-31 -- so it is the fast path, with the power worked out first.
    satellite_number scale;
    const signed long long int code = satellite_number::power(satellite_number(2ull), *places, scale);
    if (code != success) {
        context.refuse(code, call + ": 2 to the power of " + places->to_text() + " could not be worked out");
        return Value();
    }
    satellite_number answer;
    const signed long long int held = number_fast_path::multiply(*number, scale, answer);
    if (held != success) {
        context.refuse(held, call + ": the shift could not be worked out");
        return Value();
    }
    return Value::of_number(std::move(answer));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_1(LibraryRow *row)
{
    row->name = "satellite.variable.number.shift_left(n)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
