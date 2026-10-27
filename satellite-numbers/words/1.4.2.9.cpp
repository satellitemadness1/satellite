// satellite-numbers/words/1.4.2.9.cpp -- satellite.container.list.index_of(x), built into satl
//
// satellite.container.list.index_of(x)   1 4 2 9
// Where x is, counting from 1, and 0 when it is nowhere.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.index_of(x)` reaches the function below
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
    // `.index_of(x)` -- WHERE x IS, COUNTING FROM 1, and 0 when it is nowhere.
    // 0 is a real answer here rather than a refusal, and that is a file's own
    // rule kept: 0 is not a position, so it cannot be confused with one.
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
    for (std::size_t at = 0; at < items->size(); ++at)
        if (same_item((*items)[at], arguments.front()))   // as == asks it (ERRORS4 010)
            return a_count(at + 1);
    return a_count(0);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_9(LibraryRow *row)
{
    row->name = "satellite.container.list.index_of(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
