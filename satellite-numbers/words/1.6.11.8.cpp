// satellite-numbers/words/1.6.11.8.cpp -- satellite.variable.hex.number, built into satl
//
// satellite.variable.hex.number   1 6 11 8
// A hex's worth, a number: x00FF is 255. .to_number and .as_number are the lexer's second spellings of it -- so
// 1 6 11 1, satellite.variable.hex.to_number, is this number's second name.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `h.satellite.number` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/hexadecimal_values.cpp's hexadecimal_method ran for a hex's .number until that day, moved here
// whole.

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
                       name + ".satellite.number takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    return Value::of_number(receiver.as_hexadecimal()->worth);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_11_8(LibraryRow *row)
{
    row->name = "satellite.variable.hex.number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 8;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
