// satellite-numbers/words/1.4.2.27.cpp -- satellite.container.list.by_name, built into satl
//
// satellite.container.list.by_name   1 4 2 27
// The items in order by what each reads as, A to Z, always (the author, 2026-09-18), as a new list --
// names.sort().by_name(), and .reverse() after it for Z to A.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.sort().by_name()` reaches the function
// below through the function table -- bytecode/container_calls.cpp's run_list_number, which has
// refused a wrong count and missing brackets first -- and the code in it is what call_container_method
// ran for a list until that day, moved here whole.
// NUMBERED THE SAME DAY: a list answered `names.sort().by_name()` before it had a number, so it took
// the next free one under satellite.container.list.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"
#include "../../satellite/satellite_object/string_and_string_compare.hpp"
#include <algorithm>

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &name, ExpressionContext &context)
{
    std::vector<satelliteObject> borrowed;
    std::vector<satelliteObject> items = *items_of(receiver, borrowed);
    std::string why;
    std::vector<satellite_string> text;
    if (!text_of_each(items, text, why)) {
        context.refuse(types_do_not_meet, name + ".satellite.by_name: " + why);
        return Value();
    }
    // THE TEXT IS SORTED BESIDE THE ITEMS rather than looked up again,
    // so each item is converted once (see text_of_each).
    std::vector<std::size_t> order(items.size());
    for (std::size_t at = 0; at < order.size(); ++at) order[at] = at;
    std::stable_sort(order.begin(), order.end(),
                     [&text](std::size_t l, std::size_t r) {
                         return string_and_string_compare(text[l], text[r]) < 0;
                     });
    std::vector<satelliteObject> sorted;
    sorted.reserve(items.size());
    for (const std::size_t which : order) sorted.push_back(items[which]);
    items.swap(sorted);
    return Value::of_list(make_list(std::move(items)));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_27(LibraryRow *row)
{
    row->name = "satellite.container.list.by_name";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 27;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
