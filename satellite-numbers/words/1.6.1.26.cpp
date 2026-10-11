// satellite-numbers/words/1.6.1.26.cpp -- satellite.variable.string.add(x), built into satl
//
// satellite.variable.string.add(x)   1 6 1 26
// `.add` IS `+`: the string joined with x, as a new string -- the object model's own add, so a number
// given is joined as its digits exactly as the operator does it (the conversion set).
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.add(x)` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what expression.cpp's last
// branch, the one for find and add, ran until that day, copied here for a string -- every other kind
// still runs it there.
// NUMBERED THE SAME DAY: a string answered `s.add(x)` before it had a number, so it took the next free
// one under satellite.variable.string.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const std::vector<Value> &arguments, bool had_parentheses,
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

signed long long int describe_1_6_1_26(LibraryRow *row)
{
    row->name = "satellite.variable.string.add(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 26;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
