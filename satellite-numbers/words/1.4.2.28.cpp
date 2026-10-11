// satellite-numbers/words/1.4.2.28.cpp -- satellite.container.list.by_value, built into satl
//
// satellite.container.list.by_value   1 4 2 28
// The items in order by what each is worth, smallest first, always (the author, 2026-09-18), as a new
// list -- names.sort().by_value(); items with no order between them are refused, never guessed at.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.sort().by_value()` reaches the function
// below through the function table -- bytecode/container_calls.cpp's run_list_number, which has
// refused a wrong count and missing brackets first -- and the code in it is what call_container_method
// ran for a list until that day, moved here whole.
// NUMBERED THE SAME DAY: a list answered `names.sort().by_value()` before it had a number, so it took
// the next free one under satellite.container.list.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"
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
    const signed long long int comparable = all_comparable(items, "by_value orders", why);
    if (comparable != success) {
        context.refuse(comparable, name + ".satellite.by_value: " + why);
        return Value();
    }
    std::stable_sort(items.begin(), items.end(),
                     [](const satelliteObject &l, const satelliteObject &r) {
                         int order = 0;
                         std::string ignored;
                         return l.compare(r, order, ignored) == success && order < 0;
                     });
    return Value::of_list(make_list(std::move(items)));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_28(LibraryRow *row)
{
    row->name = "satellite.container.list.by_value";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 28;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
