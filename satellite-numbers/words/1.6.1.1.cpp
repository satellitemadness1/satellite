// satellite-numbers/words/1.6.1.1.cpp -- satellite.variable.string.size, built into satl
//
// satellite.variable.string.size   1 6 1 1
// How many characters the string holds (003: counts what is there; empty is 0). A character is a
// character, never a byte: "héllo".size is 5.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.size` reaches the function below through the function table -- bytecode/string_calls.cpp's
// run_string_number -- and the code in it is what bytecode/string_calls.cpp's switch ran until that
// day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    const satellite_string &text = *receiver.as_string();
    return Value::of_number(satellite_number(static_cast<unsigned long long int>(text.size())));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_1(LibraryRow *row)
{
    row->name = "satellite.variable.string.size";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
