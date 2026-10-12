// satellite-numbers/words/1.6.19.5.cpp -- satellite.variable.color.hex, built into satl
//
// satellite.variable.color.hex   1 6 19 5
// A colour's six digits as text, without the x: x00FF00 is 00FF00 -- never FF00, which is why a colour has a
// .hex of its own. .as_hex and .to_hex are the lexer's second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `c.satellite.hex` reaches the function below through the
// function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/color_values.cpp's color_method ran for a colour's .hex until that day, moved here whole.
//
// NUMBERED THE SAME DAY: a colour had no rows under satellite.variable.color, and took the next free ones,
// 1 6 19 1 to 5, for what it answered.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/bytecode/color_values.hpp"
#include "../../satellite/satellite_object/object_color.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    // A COLOUR'S OWN TEXT, never the conversion every type shares, which reaches a number first and would lose
    // the six-digit width.
    const std::string spelled = name + ".satellite.hex";
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood,
                       spelled + " takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    return scalar_method_parts::text_value(receiver.as_color()->digits(), spelled, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_19_5(LibraryRow *row)
{
    row->name = "satellite.variable.color.hex";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 19;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
