// satellite-numbers/words/1.6.11.9.cpp -- satellite.variable.hex.binary, built into satl
//
// satellite.variable.hex.binary   1 6 11 9
// A hex's bits as text, four a digit, its width kept: x00FF is 0000000011111111. .bin and .as_binary are the
// lexer's second spellings of it, and 1 6 11 5, satellite.variable.hex.to_binary, is its second name.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `h.satellite.binary` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/hexadecimal_values.cpp's hexadecimal_method ran for a hex's .binary until that day, moved here
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
    // A HEX'S OWN TEXT, never the conversion every type shares, which reaches a number first and knows
    // nothing of a width (object_hexadecimal.cpp).
    const std::string spelled = name + ".satellite.binary";
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood,
                       spelled + " takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    return scalar_method_parts::text_value(receiver.as_hexadecimal()->bits(), spelled, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_11_9(LibraryRow *row)
{
    row->name = "satellite.variable.hex.binary";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
