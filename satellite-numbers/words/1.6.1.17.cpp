// satellite-numbers/words/1.6.1.17.cpp -- satellite.variable.string.resolved, built into satl
//
// satellite.variable.string.resolved   1 6 1 17
// 003 answered its six live escapes (\\threads, \\home ...) here. 004 has no live escapes yet, so the
// string answers itself.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.resolved` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what
// bytecode/string_calls.cpp's switch ran until that day, moved here whole.
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
    // 003's live escapes (\threads, \home ...) were values and 004 has none (its
    // escapes are characters, worked out as the program is read), so there is nothing
    // left to resolve and the string answers itself -- the library's answer too.
    return receiver;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_17(LibraryRow *row)
{
    row->name = "satellite.variable.string.resolved";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 17;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
