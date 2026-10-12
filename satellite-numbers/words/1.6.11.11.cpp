// satellite-numbers/words/1.6.11.11.cpp -- satellite.variable.hex.add(x), built into satl
//
// satellite.variable.hex.add(x)   1 6 11 11
// `.add` IS `+`: a hex's worth with x added -- a NUMBER, as `x00FF + x01` is (object_hexadecimal.cpp):
// x00FF.satellite.add(x01) is 256. One argument, in brackets.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `h.satellite.add(x)` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// expression.cpp's last branch, the one for find and add, ran until that day, copied here for a hex -- the
// kinds not numbered yet still run it there.
//
// NUMBERED THE SAME DAY: a hex answered `h.add(x)` before it had a number, so it took the next free one under
// satellite.variable.hex.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool had_parentheses,
             const std::string &name, ExpressionContext &context)
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
    // `.add` IS `+`, and it is the object model's own add -- so it answers what the operator answers, a number on a hex,
    // without this file knowing why.
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

signed long long int describe_1_6_11_11(LibraryRow *row)
{
    row->name = "satellite.variable.hex.add(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 11;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
