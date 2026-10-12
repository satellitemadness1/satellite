// satellite-numbers/words/1.6.10.4.cpp -- satellite.variable.float.binary, built into satl
//
// satellite.variable.float.binary   1 6 10 4
// A float's base 2 text -- NUMBERED AND NOT BUILT: it waits for the finale, where the new types are mixed
// together, and says so (S210) in float_to_binary's own sentence.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `f.satellite.binary` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/float_values.cpp's float_method ran for a float's .binary until that day, moved here whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_float.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const std::string spelling = name + ".satellite.binary";
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, spelling + " is a conversion and takes no argument");
        return Value();
    }
    std::string why;
    satellite_string ignored;
    const signed long long int code = float_to_binary(*receiver.as_float(), ignored, why);
    context.refuse(code, spelling + ": " + why);
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_10_4(LibraryRow *row)
{
    row->name = "satellite.variable.float.binary";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 10;
    row->numbers[3] = 4;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
