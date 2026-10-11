// satellite-numbers/words/1.6.1.23.cpp -- satellite.variable.string.reverse, built into satl
//
// satellite.variable.string.reverse   1 6 1 23
// The characters in the other order, as a new string; the name keeps its own. One command on every
// type that has an order (the author, 2026-09-18): container_calls.cpp's reverse_of.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.reverse()` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what expression.cpp's reverse
// branch ran until that day, copied here for a string -- every other kind still runs it there.
// NUMBERED THE SAME DAY: a string answered `s.reverse()` before it had a number, so it took the next
// free one under satellite.variable.string.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/bytecode/container_calls.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    bool handled = false;
    std::string why;
    Value answer = reverse_of(receiver, handled, why);
    if (!handled) {
        context.refuse(types_do_not_meet,
                       name + ".satellite.reverse() was written on " + receiver.kind_name() +
                           (why.empty() ? std::string(", and that has no order to reverse")
                                        : ", and " + why));
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_23(LibraryRow *row)
{
    row->name = "satellite.variable.string.reverse";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 23;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
