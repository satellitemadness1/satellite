// satellite-numbers/words/1.6.4.19.cpp -- satellite.variable.number.binary, built into satl
//
// satellite.variable.number.binary   1 6 4 19
// Its base 2 digits, as a string, the sign kept and no b: -1234 is "-10011010010". .bin and .as_binary are its
// second spellings.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.binary` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number -- and the code in it is what expression.cpp's conversion step,
// which every kind shares, ran until that day, copied here for a number -- every other kind still runs it there.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/fast_paths.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    // AN INFINITY HAS NO BINARY TO BE MADE OF IT -- it is larger than every number (SATELLITE_INFINITY.md
    // Part 3) -- and a name declared satellite.variable.infinity holds one or a plain number (Q24), so the
    // walker's infinity guard is asked here first, in its words; a plain number runs on.
    if (receiver.is_infinity()) {
        context.refuse(types_do_not_meet, name + ".satellite.binary was written on an infinity, and an infinity is "
                                                 "larger than every number -- there is no binary to make of it");
        return Value();
    }
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
        context.refuse(code, "satellite.variable.number.binary is not built yet");
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_19(LibraryRow *row)
{
    row->name = "satellite.variable.number.binary";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 19;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
