// satellite-numbers/words/1.6.4.21.cpp -- satellite.variable.number.add(x), built into satl
//
// satellite.variable.number.add(x)   1 6 4 21
// `.add` IS `+`: the number and x through the object model's own add, so 5.satellite.add(3) is 8,
// 5.satellite.add(1.5) is 6.5 and 5.satellite.add("x") is the text 5x -- exactly as the operator does it. The
// number it is called on is not changed.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.add(x)` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number -- and the code in it is what expression.cpp's last branch, the
// one for find and add, ran until that day, copied here for a number -- every other kind still runs it there.
// NUMBERED THE SAME DAY: a number answered `n.add(x)` before it had a number, so it took the next free one under
// satellite.variable.number.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    // ONE ARGUMENT, IN BRACKETS -- said as every type's add says it (expression.cpp).
    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood,
                       name + ".satellite.add takes one argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    if (!had_parentheses) {
        context.refuse(satl_line_not_understood, name + ".satellite.add takes an argument and needs a ( after it");
        return Value();
    }
    const Value argument = arguments.empty() ? Value() : arguments.front();
    // `.add` IS `+`, and it is the object model's own add -- so it
    // joins two strings and sums two numbers without this file
    // knowing which, exactly as the operator does.
    Value answer;
    std::string why;
    const signed long long int code = receiver.add(argument, answer, why);
    if (code != success && code != text_not_found) {
        context.refuse(code, why);
        return Value();
    }
    // NOT FOUND IS A REFUSAL AND NOT -1 (003's S0716, machine code 15).
    if (code == text_not_found) {
        context.refuse(code, name + ".satellite.add did not find it");
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_21(LibraryRow *row)
{
    row->name = "satellite.variable.number.add(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 21;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
