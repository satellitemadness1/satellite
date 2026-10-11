// satellite-numbers/words/1.4.2.8.cpp -- satellite.container.list.contains(x), built into satl
//
// satellite.container.list.contains(x)   1 4 2 8
// True when an item IS x. .contain and .has are the lexer's second spellings of it.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.contains(x)` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_container_method ran for a
// list until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &, ExpressionContext &)
{
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
    for (const satelliteObject &item : *items)
        if (item == arguments.front())
            return Value::of_bool(true);
    return Value::of_bool(false);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_8(LibraryRow *row)
{
    row->name = "satellite.container.list.contains(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 8;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
