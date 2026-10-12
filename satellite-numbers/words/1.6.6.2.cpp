// satellite-numbers/words/1.6.6.2.cpp -- satellite.variable.bool.add(x), built into satl
//
// satellite.variable.bool.add(x)   1 6 6 2
// `.add` IS `+`: a bool joined with text, as + joins them -- t.satellite.add("x") answers what t + "x" does
// (satellite_object.cpp; anything + cannot join is refused in +'s own words). One argument, in brackets.
//
// NUMBERED 2026-10-11 (the author's A that evening: "yes" -- a bool's .add(x) its own numbered word, written
// t.satellite.add("x"), the old t.add("x") refused before anything runs). Until then a bool numbered only its
// .string, and `t.add("x")` ran with a bare period through expression.cpp's last branch, the one for find and
// add, which every kind not numbered shared -- the code below is that branch's, copied for a bool, as a
// percentage's add (1.6.16.2.cpp) was. A bool had no rows under satellite.variable.bool past .string, and took
// the next free one, 1 6 6 2.

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
    // `.add` IS `+`, and it is the object model's own add -- so it answers what the operator answers, text on a
    // bool and text, and refuses what the operator refuses, without this file knowing why.
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

signed long long int describe_1_6_6_2(LibraryRow *row)
{
    row->name = "satellite.variable.bool.add(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 6;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
