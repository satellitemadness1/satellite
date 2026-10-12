// satellite-numbers/words/1.6.4.22.cpp -- satellite.variable.number.reverse, built into satl
//
// satellite.variable.number.reverse   1 6 4 22
// Its digits back to front, the sign kept, and a number again: -1234 is -4321, and 120 is 21 -- 021 IS 21. One
// command on every type that has an order (the author, 2026-09-18): container_calls.cpp's reverse_of.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.reverse()` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number -- and the code in it is what expression.cpp's reverse branch ran
// until that day, copied here for a number -- every other kind still runs it there.
// NUMBERED THE SAME DAY: a number answered `n.reverse()` before it had a number, so it took the next free one
// under satellite.variable.number, after add.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
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

signed long long int describe_1_6_4_22(LibraryRow *row)
{
    row->name = "satellite.variable.number.reverse";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 22;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
