// satellite-numbers/words/1.6.4.5.cpp -- satellite.variable.number.clamp(a, low, high), built into satl
//
// satellite.variable.number.clamp(a, low, high)   1 6 4 5
// The number held between low and high, both ends kept: below low it is low, above high it is high. It is max and
// min in one call. 003 wrote it clamp(a, low, high), with the receiver as a; as a method it takes TWO arguments,
// n.satellite.clamp(0, 100).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.clamp(low, high)` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// written for it that day: number_methods.cpp, which held the rest of a number's methods, had no clamp
// (MILESTONES.md M17's gap).
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
    const std::string call = name + ".satellite.clamp";
    const satellite_number *number = the_number(receiver, call, token::clamp_token, context);
    if (number == nullptr)
        return Value();
    const satellite_number *low = number_argument(arguments[0], call, "a number for its low end", context);
    if (low == nullptr)
        return Value();
    const satellite_number *high = number_argument(arguments[1], call, "a number for its high end", context);
    if (high == nullptr)
        return Value();
    // LOW ABOVE HIGH IS REFUSED, as a range written backwards is (S412). 003 answered low there and said why
    // it could not refuse: "the alternative -- refusing -- needs a span, which is the caller's" -- and 004
    // has the call's own name to give, so a range with nothing between its ends is said to be one, and no
    // end of it is picked behind the program's back.
    if (number_fast_path::compare(*low, *high) > 0) {
        context.refuse(positions_backwards, call + " was given a low end of " + low->to_text() + " and a high end of " +
                                                high->to_text() + ", so there is nothing between them to hold it in");
        return Value();
    }
    if (number_fast_path::compare(*number, *low) < 0)
        return Value::of_number(*low);
    if (number_fast_path::compare(*number, *high) > 0)
        return Value::of_number(*high);
    return Value::of_number(*number);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_5(LibraryRow *row)
{
    row->name = "satellite.variable.number.clamp(a, low, high)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
