// satellite-numbers/words/1.6.11.12.cpp -- satellite.variable.hex.reverse, built into satl
//
// satellite.variable.hex.reverse   1 6 11 12
// A hex's digits in the other order, as a new hex, its width and its sign kept: x00FF is xFF00 (the author,
// 2026-09-22, asked for .reverse() on "binary numbers hex"). container_calls.cpp's reverse_of.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `h.satellite.reverse()` reaches the function below
// through the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is what
// expression.cpp's reverse branch ran until that day, copied here for a hex -- the kinds not numbered yet still
// run it there.
//
// NUMBERED THE SAME DAY: a hex answered `h.reverse()` before it had a number, so it took the next free one
// under satellite.variable.hex.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/bytecode/container_calls.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
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

signed long long int describe_1_6_11_12(LibraryRow *row)
{
    row->name = "satellite.variable.hex.reverse";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 12;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
