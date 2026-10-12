// satellite-numbers/words/1.6.16.1.cpp -- satellite.variable.percentage.string, built into satl
//
// satellite.variable.percentage.string   1 6 16 1
// A percentage as text, as written: 50%. .to_string and .str are the lexer's second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `p.satellite.string` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// expression.cpp's conversion step, which every kind shared, ran until that day, copied here for a percentage
// -- the kinds not numbered yet still run it there.
//
// NUMBERED THE SAME DAY: a percentage had no rows under satellite.variable.percentage, and took the next free
// ones, 1 6 16 1 and 2, for what it answered.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/fast_paths.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    // A CONVERSION MAY BE WRITTEN WITH OR WITHOUT PARENTHESES, and takes nothing in them -- said in
    // the words every type's conversion says it in (expression.cpp).
    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood,
                       name + ".satellite.to_string takes one argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, "to_string is a conversion and takes no argument");
        return Value();
    }
    Value answer;
    const signed long long int code = object_to_string(receiver, answer);
    if (code == types_do_not_meet) {
        context.refuse(code, std::string("to_string was written on ") + receiver.kind_name() +
                                 ", and there is no conversion from that");
        return Value();
    }
    if (code == int_error) {
        context.refuse(code, name + ".satellite.to_string: that text is not a whole number this can read");
        return Value();
    }
    if (code != success) {
        context.refuse(code, "satellite.variable.percentage.string is not built yet");
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_16_1(LibraryRow *row)
{
    row->name = "satellite.variable.percentage.string";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 16;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
