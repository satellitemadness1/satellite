// satellite-numbers/words/1.6.1.5.cpp -- satellite.variable.string.substring(start, end), built into satl
//
// satellite.variable.string.substring(start, end)   1 6 1 5
// Characters start to end, both counting from 1 and both kept: "hello".substring(2, 4) is ell. 003
// counted from 0 with the end left out, so the END number is the one both write. 003's checks, in
// 003's order -- a negative, then backwards, then past the end -- with 0 said as the one position a
// count from 1 does not have. An empty piece is substring(n + 1, n).
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.substring(start, end)` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what
// bytecode/string_calls.cpp's piece_of ran until that day, moved here whole.
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
    const satellite_string &text = *receiver.as_string();
    const std::string call = name + ".satellite.substring";
    unsigned long long int start = 0, end = 0;
    std::string start_written, end_written;
    if (!position_argument(arguments[0], start, start_written, call, context) ||
        !position_argument(arguments[1], end, end_written, call, context))
        return Value();
    const std::string spelled = call + "(" + start_written + ", " + end_written + ")";
    if (start == 0) {   // S413
        context.refuse(counts_from_one, spelled + ": satellite counts from 1, not 0, so the first character is 1");
        return Value();
    }
    if (start - 1 > end) {
        // end < start - 1 here, so end + 1 is a real count and cannot wrap.
        context.refuse(positions_backwards, spelled + " starts after it ends -- an empty piece is written with "
                                                     "its end one before its start, as " + call + "(" +
                                                     std::to_string(end + 1) + ", " + end_written + ")");
        return Value();
    }
    if (end > text.size()) {
        context.refuse(position_past_the_end,
                       spelled + ": there is no character " + end_written + " -- " + what_it_holds(text));
        return Value();
    }
    satellite_string out;
    text.substring(start - 1, end, out);
    return Value::of_string(std::move(out));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_5(LibraryRow *row)
{
    row->name = "satellite.variable.string.substring(start, end)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
