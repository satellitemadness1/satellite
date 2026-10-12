// satellite-numbers/words/1.6.5.10.cpp -- satellite.variable.binary.binary, built into satl
//
// satellite.variable.binary.binary   1 6 5 10
// A binary's own digits as text, without the b and with its width: b0101 is 0101 -- never sent round the number
// hub, which has no width (object_convert.cpp). .bin and .as_binary are its second spellings.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `b.satellite.binary` reaches the function below through
// the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// expression.cpp's conversion step, which every kind shared, ran until that day, copied here for a binary --
// the kinds not numbered yet still run it there.

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
                       name + ".satellite.binary takes one argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, "binary is a conversion and takes no argument");
        return Value();
    }
    Value answer;
    const signed long long int code = object_to_binary(receiver, answer);
    if (code == types_do_not_meet) {
        context.refuse(code, std::string("binary was written on ") + receiver.kind_name() +
                                 ", and there is no conversion from that");
        return Value();
    }
    if (code == int_error) {
        context.refuse(code, name + ".satellite.binary: that text is not a whole number this can read");
        return Value();
    }
    if (code != success) {
        context.refuse(code, "satellite.variable.binary.binary is not built yet");
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_5_10(LibraryRow *row)
{
    row->name = "satellite.variable.binary.binary";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 5;
    row->numbers[3] = 10;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
