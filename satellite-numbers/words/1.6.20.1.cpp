// satellite-numbers/words/1.6.20.1.cpp -- satellite.variable.fraction.numerator, built into satl
//
// satellite.variable.fraction.numerator   1 6 20 1
// The number above a fraction's line: a whole number when nothing follows its point (1/3's is 1), the float
// when something does (1.5/2's is 1.5).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `q.satellite.numerator` reaches the function below
// through the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/fraction_values.cpp's fraction_method ran for a fraction's .numerator until that day, moved here
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
        context.refuse(satl_line_not_understood, name + ".satellite.numerator takes no argument");
        return Value();
    }
    return scalar_method_parts::part_as_value(receiver.as_fraction()->numerator);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_20_1(LibraryRow *row)
{
    row->name = "satellite.variable.fraction.numerator";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 20;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
