// satellite-numbers/words/1.6.1.11.cpp -- satellite.variable.string.trim, built into satl
//
// satellite.variable.string.trim   1 6 1 11
// The text without space, tab, carriage return or newline at either end, as in 003; a new string is
// answered and the name keeps its own.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.trim` reaches the function below through the function table -- bytecode/string_calls.cpp's
// run_string_number -- and the code in it is what bytecode/string_calls.cpp's switch ran until that
// day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/string_pieces.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    const satellite_string &text = *receiver.as_string();
    return Value::of_string(string_pieces::trimmed(text));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_11(LibraryRow *row)
{
    row->name = "satellite.variable.string.trim";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 11;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
