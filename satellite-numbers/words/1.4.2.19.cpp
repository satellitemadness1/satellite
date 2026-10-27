// satellite-numbers/words/1.4.2.19.cpp -- satellite.container.list.remove(x), built into satl
//
// satellite.container.list.remove(x)   1 4 2 19
// Takes out the first item that IS x, CHANGING the list on the name -- and refuses when there is none,
// because a program that removes nothing should say so. .contains(x) asks first.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.remove(x)` reaches the function below
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
    satelliteList *body = list_to_change(home, name, "remove", context);
    if (body == nullptr)
        return Value();
    if (body->items.empty()) {
        context.refuse(line_past_the_end, name + ".satellite.remove: the list is empty");
        return Value();
    }
    // `.remove(x)` -- THE FIRST ITEM THAT IS x, and a refusal when there is
    // none. Quietly doing nothing is the alternative, and it is worse: a
    // program that removes the wrong thing tells you, and a program that
    // removes nothing does not. `.contains(x)` is how you ask first.
    for (std::size_t at = 0; at < body->items.size(); ++at) {
        if (same_item(body->items[at], arguments.front())) {   // as == asks it (ERRORS4 010)
            body->items.erase(body->items.begin() + static_cast<std::ptrdiff_t>(at));
            return *home;
        }
    }
    context.refuse(text_not_found, name + ".satellite.remove: there is no such item in it -- " + name +
                                       ".satellite.contains(x) asks before removing");
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_19(LibraryRow *row)
{
    row->name = "satellite.container.list.remove(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 19;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
