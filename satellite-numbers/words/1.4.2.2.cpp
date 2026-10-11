// satellite-numbers/words/1.4.2.2.cpp -- satellite.container.list.size, built into satl
//
// satellite.container.list.size   1 4 2 2
// How many items the list holds.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.size` reaches the function below through
// the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a wrong
// count and missing brackets first -- and the code in it is what call_container_method ran for a list
// until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    std::vector<satelliteObject> borrowed;
    return a_count(items_of(receiver, borrowed)->size());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_2(LibraryRow *row)
{
    row->name = "satellite.container.list.size";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
