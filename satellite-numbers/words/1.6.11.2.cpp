// satellite-numbers/words/1.6.11.2.cpp -- satellite.variable.hex.width, built into satl
//
// satellite.variable.hex.width   1 6 11 2
// How many bits wide a hex is, four to the digit: x00FF is 16. With or without (), and nothing in them.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `h.satellite.width` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/hexadecimal_values.cpp's hexadecimal_method ran for a hex's .width until that day, moved here whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_hexadecimal.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood,
                       name + ".satellite.width takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    // `.width` COUNTS BITS, FOUR TO THE DIGIT -- x00FF is 16 -- which is the author's call in 003 (DESIGN 8.5,
    // 2026-09-09: "width() COUNTS BITS ON BOTH AND digits() COUNTS DIGITS"), so that a width means the same
    // thing on a binary and a hex.
    return Value::of_number(satellite_number(receiver.as_hexadecimal()->width) * satellite_number(4ull));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_11_2(LibraryRow *row)
{
    row->name = "satellite.variable.hex.width";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
