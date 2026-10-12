// satellite-numbers/words/1.4.1.2.cpp -- satellite.container.map.get(k), built into satl
//
// satellite.container.map.get(k)   1 4 1 2
// The value under the key k -- `m[k]`, said in a word. A key it does not hold is refused (S501).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.get(k)` reaches the function below through the
// function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
// brackets first. From 2026-09-26 until that day the word had no token, and expression.cpp's one_operand matched it
// by its name, straight on a name that held a map, and called index_into -- the function `m[k]` is read by, which
// this calls still, so the two cannot come to say different things. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND:
// what satellite.container.index declares, the arguments variable and an info's items are maps too, and run it.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/bytecode/expression.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    // `m.get(k)` IS `m[k]`, THROUGH THE SAME FUNCTION (the author found it refused on 2026-09-26, ERRORS2
    // 1b A/D, and his programs call it): a missing key is refused in the words m[k] uses.
    return read_under_a_key(receiver, arguments[0], name, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_2(LibraryRow *row)
{
    row->name = "satellite.container.map.get(k)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
