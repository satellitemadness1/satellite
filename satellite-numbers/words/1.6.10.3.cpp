// satellite-numbers/words/1.6.10.3.cpp -- satellite.variable.float.number, built into satl
//
// satellite.variable.float.number   1 6 10 3
// The whole number a float is, when nothing follows its point: 3.0 is 3. 2.5 is refused in the float's own
// sentence (S402) and never rounded. .to_number and .as_number are the lexer's second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `f.satellite.number` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// bytecode/float_values.cpp's float_method ran for a float's .number until that day, moved here whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_float.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    // A FLOAT'S OWN SENTENCE REACHES THE PERSON -- `12.5 is not a whole number` -- rather than "there is no
    // conversion from that", which is why a float has a .number of its own and not the shared one.
    const std::string spelling = name + ".satellite.number";
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, spelling + " is a conversion and takes no argument");
        return Value();
    }
    std::string why;
    satellite_number whole;
    const signed long long int code = float_to_number(*receiver.as_float(), whole, why);
    if (code == success)
        return Value::of_number(std::move(whole));
    context.refuse(code, spelling + ": " + why);
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_10_3(LibraryRow *row)
{
    row->name = "satellite.variable.float.number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 10;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
