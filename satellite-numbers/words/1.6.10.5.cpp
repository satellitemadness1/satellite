// satellite-numbers/words/1.6.10.5.cpp -- satellite.variable.float.hex, built into satl
//
// satellite.variable.float.hex   1 6 10 5
// A float's base 16 text -- NUMBERED AND NOT BUILT: it waits for the finale, where the new types are mixed
// together, and says so (S210) in float_to_hexadecimal's own sentence.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `f.satellite.hex` reaches the function below through the
// function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/float_values.cpp's float_method ran for a float's .hex until that day, moved here whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_float.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const std::string spelling = name + ".satellite.hex";
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, spelling + " is a conversion and takes no argument");
        return Value();
    }
    std::string why;
    satellite_string ignored;
    const signed long long int code = float_to_hexadecimal(*receiver.as_float(), ignored, why);
    context.refuse(code, spelling + ": " + why);
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_10_5(LibraryRow *row)
{
    row->name = "satellite.variable.float.hex";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 10;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
