// satellite-numbers/words/1.6.1.25.cpp -- satellite.variable.string.background(color), built into satl
//
// satellite.variable.string.background(color)   1 6 1 25
// The string answered in that colour behind it, or unchanged where no colour is drawn
// (console_style.hpp): one colour, six hex digits like xFF8800 or a satellite.variable.color.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.background(c)` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what expression.cpp's colour
// branch ran until that day, moved here whole.
// NUMBERED THE SAME DAY: a string answered `s.background(c)` before it had a number, so it took the
// next free one under satellite.variable.string.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/bytecode/console_calls.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const std::vector<Value> &arguments, bool had_parentheses,
             const std::string &name, ExpressionContext &context)
{
    return string_coloured(receiver, token::background_token, arguments, had_parentheses, name, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_25(LibraryRow *row)
{
    row->name = "satellite.variable.string.background(color)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 25;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
