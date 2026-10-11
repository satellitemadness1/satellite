// satellite-numbers/words/1.4.2.25.cpp -- satellite.container.list.min, built into satl
//
// satellite.container.list.min   1 4 2 25
// The smallest item, by .sort().by_value()'s own comparison -- so .min IS .sort().by_value().first,
// even between equal items. Taken from 003, 2026-09-23.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.min` reaches the function below through the
// function table -- bytecode/container_calls.cpp's run_list_number, which has refused a wrong count
// and missing brackets first -- and the code in it is what call_container_method ran for a list until
// that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &name, ExpressionContext &context)
{
    // `.max` AND `.min` -- THE ORDERING ASKED ONCE. Not a sort that keeps one end: a
    // sort is n log n and copies the list, and asking which item is largest is one
    // pass. Worth is `.sort().by_value()`'s own comparison, so `.max` IS
    // `.sort().by_value().last` and `.min` IS `.sort().by_value().first` -- EVEN
    // BETWEEN EQUAL ITEMS: a stable sort keeps equals in the order they came, so of
    // {2, 1, 2.0} the last is 2.0, and .max answers 2.0 (it answered 2 until the
    // review, 2026-09-23). So .max keeps the LATER of two equals and .min the earlier.
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> &items = *items_of(receiver, borrowed);
    if (items.empty()) {
        context.refuse(line_past_the_end, name + ".satellite.min: the list is empty, so nothing in it is the smallest");
        return Value();
    }
    std::string why;
    const signed long long int comparable = all_comparable(items, "min finds the smallest", why);
    if (comparable != success) {
        context.refuse(comparable, name + ".satellite.min: " + why);
        return Value();
    }
    // AND EVERY COMPARISON THE PASS MAKES IS STILL ASKED WHETHER IT ANSWERED. The
    // kinds were judged above; a refusal here would be one that depends on the
    // values, and skipping it is how a wrong item came back with exit 0 before.
    const satelliteObject *best = &items.front();
    for (std::size_t at = 1; at < items.size(); ++at) {
        int order = 0;
        std::string inner;
        const signed long long int code = items[at].compare(*best, order, inner);
        if (code != success) {
            context.refuse(code, name + ".satellite.min: item " + std::to_string(at + 1) +
                                     " has no order with the smallest before it: " + inner);
            return Value();
        }
        if (order < 0)
            best = &items[at];
    }
    return *best;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_25(LibraryRow *row)
{
    row->name = "satellite.container.list.min";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 25;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
