// satellite-numbers/words/1.4.1.6.cpp -- satellite.container.map.clear, built into satl
//
// satellite.container.map.clear   1 4 1 6
// Takes every entry out, CHANGING the map on the name. Refused when there is no name to change.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.clear` reaches the function below through the
// function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
// brackets first -- and the code in it is what call_container_method ran for a map until that day, moved here
// whole. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND: what satellite.container.index declares, the arguments
// variable and an info's items are maps too, and run it.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::map_method_parts;

Value method(const Value &, Value *home, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &name, ExpressionContext &context)
{
    satelliteIndex *body = map_to_change(home, name, "clear", context);
    if (body == nullptr)
        return Value();
    body->entries.clear();
    body->where.clear();
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_6(LibraryRow *row)
{
    row->name = "satellite.container.map.clear";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 6;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
