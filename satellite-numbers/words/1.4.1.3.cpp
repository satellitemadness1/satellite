// satellite-numbers/words/1.4.1.3.cpp -- satellite.container.map.has(k), built into satl
//
// satellite.container.map.has(k)   1 4 1 3
// Whether the key k is in the map, asked of its hash table -- `.contains(k)`, and `.contain` and `.has`, the
// author's second and third spellings: one token, so all three run this number. A value that can never be
// a key is a key it does not hold, and answers false.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.contains(k)` reaches the function below through
// the function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
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

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &, ExpressionContext &)
{
    // AN INDEX ASKS ITS HASH TABLE, which is the entire reason it has one.
    // Copying every key into a vector and walking it made a hash-table
    // lookup cost O(n) plus a full copy -- on the container whose whole
    // point is that asking is cheap.
    if (const IndexHandle *keys = receiver.as_index()) {
        const satelliteIndex *held = keys->get();
        std::string key_name;
        if (held == nullptr || !key_name_of(arguments.front(), key_name))
            return Value::of_bool(false);   // a key it could never hold is a key it does not hold
        return Value::of_bool(value_at(*held, key_name) != nullptr);
    }
    return Value::of_bool(false);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_3(LibraryRow *row)
{
    row->name = "satellite.container.map.has(k)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
