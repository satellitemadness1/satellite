// satellite-numbers/words/1.6.20.2.cpp -- satellite.variable.fraction.denominator, built into satl
//
// satellite.variable.fraction.denominator   1 6 20 2
// The number below a fraction's line, a whole number or a float as for its numerator: 1/3's is 3.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `q.satellite.denominator` reaches the function below
// through the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/fraction_values.cpp's fraction_method ran for a fraction's .denominator until that day, moved here
// whole.
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
        context.refuse(satl_line_not_understood, name + ".satellite.denominator takes no argument");
        return Value();
    }
    return scalar_method_parts::part_as_value(receiver.as_fraction()->denominator);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_20_2(LibraryRow *row)
{
    row->name = "satellite.variable.fraction.denominator";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 20;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
