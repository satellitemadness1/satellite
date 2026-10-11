// satellite-numbers/words/1.4.2.23.cpp -- satellite.container.list.sum, built into satl
//
// satellite.container.list.sum   1 4 2 23
// Every item added with +, left to right, by the object model's own add; a list of nothing sums to 0,
// and a string item is refused (.join is a string's). Taken from 003, 2026-09-23.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.sum` reaches the function below through the
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
    // `.sum` -- EVERY ITEM ADDED WITH +, left to right, by the object model's own
    // add: a list of floats sums as a float and a fraction stays a fraction, exactly
    // as `a + b + c` written out would. A LIST OF NOTHING SUMS TO 0 -- not a
    // sentinel but the sum of no numbers, 003's one row where "nothing to answer
    // with" has an answer.
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> &items = *items_of(receiver, borrowed);
    if (items.empty())
        return a_count(0);
    for (std::size_t at = 0; at < items.size(); ++at) {
        if (!a_number_kind(items[at])) {
            context.refuse(types_do_not_meet,
                           name + ".satellite.sum adds numbers, and item " + std::to_string(at + 1) + " is " +
                               items[at].kind_name() +
                               (items[at].is_string() ? " -- .satellite.join(separator) makes one string of them" : ""));
            return Value();
        }
    }
    Value total = items.front();
    for (std::size_t at = 1; at < items.size(); ++at) {
        Value next;
        std::string why;
        const signed long long int added = total.add(items[at], next, why);
        if (added != success) {
            context.refuse(added, name + ".satellite.sum: item " + std::to_string(at + 1) +
                                      " cannot be added to the items before it -- " + why);
            return Value();
        }
        total = std::move(next);
    }
    return total;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_23(LibraryRow *row)
{
    row->name = "satellite.container.list.sum";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 23;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
