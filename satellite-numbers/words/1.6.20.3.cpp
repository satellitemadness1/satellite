// satellite-numbers/words/1.6.20.3.cpp -- satellite.variable.fraction.string, built into satl
//
// satellite.variable.fraction.string   1 6 20 3
// A fraction as text, as display shows it: 1/3. .to_string and .str are the lexer's second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `q.satellite.string` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/fraction_values.cpp's fraction_method ran for a fraction's .string until that day, moved here whole.
//
// NUMBERED THE SAME DAY: a fraction had no rows under satellite.variable.fraction, and took the next free ones,
// 1 6 20 1 to 3, for what it answered.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_fraction.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, name + ".satellite.to_string takes no argument");
        return Value();
    }
    std::string why;
    satellite_string text;
    fraction_to_string(*receiver.as_fraction(), text, why);
    return Value::of_string(std::move(text));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_20_3(LibraryRow *row)
{
    row->name = "satellite.variable.fraction.string";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 20;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
