// satellite-numbers/words/1.4.2.15.cpp -- satellite.container.list.reserve(n), built into satl
//
// satellite.container.list.reserve(n)   1 4 2 15
// Room for n items, made once (003's list_reserve): no item moves and .size answers what it did, but
// the next n appends find the room already there. Room the machine will not give is out_of_memory
// (48), never a crash.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.reserve(n)` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_container_method ran for a
// list until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"
#include <limits>
#include <new>
#include <stdexcept>

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &, Value *home, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    satelliteList *body = list_to_change(home, name, "reserve", context);
    if (body == nullptr)
        return Value();
    unsigned long long int position = 0;
    std::string written;
    if (!position_argument(arguments.front(), true, name, "reserve", position, written, context))
        return Value();
    // `.reserve(n)` -- ROOM FOR n ITEMS, MADE ONCE, 003's list_reserve. No item
    // moves and .size answers what it did; what changes is that the next n
    // appends find the room already there. Room the machine will not give is
    // out_of_memory (48), said here -- never a crash, and never a quiet no.
    if (position > body->items.max_size()) {
        context.refuse(out_of_memory, name + ".satellite.reserve(" + written + "): no machine holds that many items");
        return Value();
    }
    try {
        body->items.reserve(static_cast<std::size_t>(position));
    } catch (const std::bad_alloc &) {
        context.refuse(out_of_memory,
                       name + ".satellite.reserve(" + written + "): the machine would not give satl the memory for that many");
        return Value();
    } catch (const std::length_error &) {
        context.refuse(out_of_memory, name + ".satellite.reserve(" + written + "): no machine holds that many items");
        return Value();
    }
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_15(LibraryRow *row)
{
    row->name = "satellite.container.list.reserve(n)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 15;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
