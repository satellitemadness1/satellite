// satellite-numbers/words/1.6.10.2.cpp -- satellite.variable.float.string, built into satl
//
// satellite.variable.float.string   1 6 10 2
// The float as text, as display shows it: 2.5. .to_string and .str are the lexer's second spellings of it -- so
// 1 6 10 1, satellite.variable.float.to_string, is this number's second name.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `f.satellite.string` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/float_values.cpp's float_method ran for a float's .string until that day, moved here whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_float.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    // A CONVERSION TAKES NOTHING IN ITS BRACKETS, said as a float's always said it.
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, name + ".satellite.to_string is a conversion and takes no argument");
        return Value();
    }
    std::string why;
    satellite_string text;
    float_to_string(*receiver.as_float(), text, why);
    return Value::of_string(std::move(text));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_10_2(LibraryRow *row)
{
    row->name = "satellite.variable.float.string";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 10;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
