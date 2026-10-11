// satellite-numbers/words/1.4.2.17.cpp -- satellite.container.list.remove_last(), built into satl
//
// satellite.container.list.remove_last()   1 4 2 17
// Takes the last item out, CHANGING the list on the name. An empty list has none to take.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.remove_last()` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_container_method ran for a
// list until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &, Value *home, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &name, ExpressionContext &context)
{
    satelliteList *body = list_to_change(home, name, "remove_last", context);
    if (body == nullptr)
        return Value();
    if (body->items.empty()) {
        context.refuse(line_past_the_end, name + ".satellite.remove_last: the list is empty");
        return Value();
    }
    body->items.pop_back();
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_17(LibraryRow *row)
{
    row->name = "satellite.container.list.remove_last()";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 17;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
