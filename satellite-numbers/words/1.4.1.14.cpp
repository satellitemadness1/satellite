// satellite-numbers/words/1.4.1.14.cpp -- satellite.container.map.reverse, built into satl
//
// satellite.container.map.reverse   1 4 1 14
// The KEYS, back to front, AS A LIST -- not a map (what the walker answered before the word had a number,
// and what programs ran: {"b": 2, "a": 1}.satellite.reverse() is {"a", "b"}). The map is not changed.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.reverse()` reaches the function below through
// the function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
// brackets first -- and the code in it is what call_container_method ran for a map until that day, moved here
// whole. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND: what satellite.container.index declares, the arguments
// variable and an info's items are maps too, and run it.
// The number was given that day, the next free under satellite.container.map (words/words_004.tsv).

#include <algorithm>

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    // THE KEYS, BACK TO FRONT, AS A LIST (the integrator's ruling of 2026-10-11: what the walker answers,
    // which is what programs ran -- method_periods.hpp's kind table said a map until that day, and no
    // spelling of m.reverse().size ran). A map's order is the order its keys were put in, so the keys
    // are what there is to turn round.
    std::vector<satelliteObject> borrowed;
    std::vector<satelliteObject> items = *items_of(receiver, borrowed);
    std::reverse(items.begin(), items.end());
    return Value::of_list(make_list(std::move(items)));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_14(LibraryRow *row)
{
    row->name = "satellite.container.map.reverse";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 14;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
