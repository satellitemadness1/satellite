// satellite-numbers/words/1.4.1.4.cpp -- satellite.container.map.size, built into satl
//
// satellite.container.map.size   1 4 1 4
// How many entries -- read from the index's own count, never by copying every key out to measure the copy.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.size` reaches the function below through the
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
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    // AN INDEX ANSWERS FROM ITS OWN COUNT, never by copying every key out to
    // measure the copy. items_of() builds a vector for the methods that walk one,
    // and `.size` does not walk anything.
    const IndexHandle *keys = receiver.as_index();
    const satelliteIndex *held = keys == nullptr ? nullptr : keys->get();
    return a_count(held == nullptr ? 0 : held->entries.size());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_4(LibraryRow *row)
{
    row->name = "satellite.container.map.size";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 4;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
