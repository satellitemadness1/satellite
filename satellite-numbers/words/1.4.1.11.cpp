// satellite-numbers/words/1.4.1.11.cpp -- satellite.container.map.first, built into satl
//
// satellite.container.map.first   1 4 1 11
// The first KEY, in the order the keys were put in -- not a value. An empty map has none, and says so.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.first` reaches the function below through the
// function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
// brackets first -- and the code in it is what call_container_method ran for a map until that day, moved here
// whole. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND: what satellite.container.index declares, the arguments
// variable and an info's items are maps too, and run it.
// The number was given that day, the next free under satellite.container.map (words/words_004.tsv).

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &name, ExpressionContext &context)
{
    // `.first` AND `.last` ON A MAP ARE ITS FIRST AND LAST KEY IN INSERTION ORDER, which is the
    // only order a dict has -- and the order it prints in, so what you read here
    // is what you saw. A KEY, NOT A VALUE: the value is m[m.satellite.first].
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
    if (items->empty()) {
        context.refuse(line_past_the_end, name + ".satellite.first: the index is empty");
        return Value();
    }
    return items->front();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_11(LibraryRow *row)
{
    row->name = "satellite.container.map.first";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 11;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
