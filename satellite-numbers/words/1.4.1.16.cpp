// satellite-numbers/words/1.4.1.16.cpp -- satellite.container.map.remove_last(), built into satl
//
// satellite.container.map.remove_last()   1 4 1 16
// Takes the newest entry out, CHANGING the map on the name. An empty map is refused.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.remove_last()` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and
// missing brackets first -- and the code in it is what call_container_method ran for a map until that day, moved
// here whole. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND: what satellite.container.index declares, the arguments
// variable and an info's items are maps too, and run it.
// The number was given that day, the next free under satellite.container.map (words/words_004.tsv).

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
    satelliteIndex *body = map_to_change(home, name, "remove_last", context);
    if (body == nullptr)
        return Value();
    if (body->entries.empty()) {
        context.refuse(line_past_the_end, name + ".satellite.remove_last: the index is empty");
        return Value();
    }
    take_entry_out(*body, body->entries.size() - 1);
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_16(LibraryRow *row)
{
    row->name = "satellite.container.map.remove_last()";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 16;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
