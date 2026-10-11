// satellite-numbers/words/1.6.1.9.cpp -- satellite.variable.string.upper, built into satl
//
// satellite.variable.string.upper   1 6 1 9
// The text in capitals, every language's letters and not only a..z (string_case.hpp); a new string is
// answered and the name keeps its own. .uppercase() and .up() are the lexer's second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.upper()` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what expression.cpp's own
// branch for upper and lower ran until that day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/string_case.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, name + ".satellite.upper() takes nothing in its brackets");
        return Value();
    }
    satellite_string changed;
    const signed long long int made = string_case(*receiver.as_string(), true, changed);
    if (made != success) {
        context.refuse(made, name + ".satellite.upper() could not change that text");
        return Value();
    }
    return Value::of_string(std::move(changed));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_9(LibraryRow *row)
{
    row->name = "satellite.variable.string.upper";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
