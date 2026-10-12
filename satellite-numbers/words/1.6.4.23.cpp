// satellite-numbers/words/1.6.4.23.cpp -- satellite.variable.number.bytes, built into satl
//
// satellite.variable.number.bytes   1 6 4 23
// How many bytes the number takes in memory: its limbs, side by side, 8 bytes each -- so it is 8 until the number
// outgrows one unsigned long long. satellite_number counts them when asked.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.bytes()` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// number_methods.cpp's -- written 2026-09-16 for this word and never wired (in no make file, called by nothing)
// -- moved here whole the day that file was deleted.
// NUMBERED THE SAME DAY: number_methods.cpp had bytes and words.tsv had no row for it (MILESTONES.md M17's gap),
// so it took the next free number under satellite.variable.number, after add and reverse.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::number_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    const satellite_number *number = the_number(receiver, name + ".satellite.bytes", token::bytes_token, context);
    if (number == nullptr)
        return Value();
    return Value::of_number(number->bytes());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_23(LibraryRow *row)
{
    row->name = "satellite.variable.number.bytes";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 23;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
