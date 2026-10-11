// satellite-numbers/words/1.6.1.16.cpp -- satellite.variable.string.at(n), built into satl
//
// satellite.variable.string.at(n)   1 6 1 16
// The one character at position n, counting from 1, as a string of one character (003: there is no
// character type). The same answer and the same refusals as s[n]; 0 and past the end are refused.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.at(n)` reaches the function below through the function table -- bytecode/string_calls.cpp's
// run_string_number -- and the code in it is what bytecode/string_calls.cpp's switch ran until that
// day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::string_method_parts;

Value method(const Value &receiver, Value *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const std::string call = name + ".satellite.at";
    return one_character(*receiver.as_string(), arguments[0], call + "(", ")", 0, false, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_16(LibraryRow *row)
{
    row->name = "satellite.variable.string.at(n)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 16;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
