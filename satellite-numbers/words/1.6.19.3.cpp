// satellite-numbers/words/1.6.19.3.cpp -- satellite.variable.color.number, built into satl
//
// satellite.variable.color.number   1 6 19 3
// What a colour's six digits are worth, a number: x00FF00 is 65280. .to_number and .as_number are the lexer's
// second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `c.satellite.number` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/color_values.cpp's color_method ran for a colour's .number until that day, moved here whole.
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
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood,
                       name + ".satellite.number takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    return Value::of_number(satellite_number(static_cast<unsigned long long int>(receiver.as_color()->rgb)));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_19_3(LibraryRow *row)
{
    row->name = "satellite.variable.color.number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 19;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
