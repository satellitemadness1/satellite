// satellite-numbers/words/1.4.1.9.cpp -- satellite.container.map.values, built into satl
//
// satellite.container.map.values   1 4 1 9
// Every value, as a list, in the same order as .keys.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.values` reaches the function below through the
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
#include "../../satellite/satellite_object/satellite_list.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    // `.keys` AND `.values`, AS LISTS, IN INSERTION ORDER -- and the two line up
    // index for index, so `k.keys[2]` and `k.values[2]` are one entry. That is
    // the whole reason they are not two independently-ordered answers.
    std::vector<satelliteObject> out;
    const IndexHandle *keys = receiver.as_index();
    const satelliteIndex *held = keys == nullptr ? nullptr : keys->get();
    if (held != nullptr) {
        out.reserve(held->entries.size());
        for (const satelliteMapEntry &entry : held->entries)
            out.push_back(entry.value);
    }
    return Value::of_list(make_list(std::move(out)));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_9(LibraryRow *row)
{
    row->name = "satellite.container.map.values";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
