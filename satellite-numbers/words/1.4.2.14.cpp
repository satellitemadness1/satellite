// satellite-numbers/words/1.4.2.14.cpp -- satellite.container.list.truncate(n), built into satl
//
// satellite.container.list.truncate(n)   1 4 2 14
// Keeps the first n items, CHANGING the list on the name; keeping more than there are changes nothing.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.truncate(n)` reaches the function below
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
    satelliteList *body = list_to_change(home, name, "truncate", context);
    if (body == nullptr)
        return Value();
    const std::size_t held = body->items.size();
    unsigned long long int position = 0;
    std::string written;
    // A COUNT OF ITEMS TO KEEP, as .reserve's is -- 0 is one -- so -1 is told "a count is 0 or more", never
    // "items count from 1" (ERRORS4 063).
    if (!position_argument(arguments.front(), true, name, "truncate", position, written, context))
        return Value();
    // KEEPING MORE THAN THERE ARE CHANGES NOTHING and is not an
    // error, which is what a file's truncate does. `truncate(0)`
    // empties it, and that is the one place 0 is a real answer
    // rather than a position.
    if (position < held)
        body->items.resize(static_cast<std::size_t>(position));
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_14(LibraryRow *row)
{
    row->name = "satellite.container.list.truncate(n)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 14;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
