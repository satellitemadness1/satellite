// satellite-numbers/words/1.4.2.18.cpp -- satellite.container.list.remove_at(n), built into satl
//
// satellite.container.list.remove_at(n)   1 4 2 18
// Takes item n out, counting from 1, CHANGING the list on the name.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.remove_at(n)` reaches the function below
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

Value method(const Value &, Value *home, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    satelliteList *body = list_to_change(home, name, "remove_at", context);
    if (body == nullptr)
        return Value();
    const std::size_t held = body->items.size();
    unsigned long long int position = 0;
    std::string written;
    if (!position_argument(arguments.front(), false, name, "remove_at", position, written, context))
        return Value();
    if (position == 0) {
        context.refuse(counts_from_one,
                       name + ".satellite.remove_at(0): satellite counts from 1, not 0, so the first item is 1");
        return Value();
    }
    if (position > held) {
        context.refuse(line_past_the_end,
                       name + ".satellite.remove_at(" + written + "): " +
                           (held == 0 ? std::string("the list is empty")
                                      : "the list holds " + std::to_string(held) + (held == 1 ? " item" : " items") +
                                            ", counting from 1"));
        return Value();
    }
    body->items.erase(body->items.begin() + static_cast<std::ptrdiff_t>(position - 1));
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_18(LibraryRow *row)
{
    row->name = "satellite.container.list.remove_at(n)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 18;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
