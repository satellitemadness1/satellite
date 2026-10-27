// satellite-numbers/words/1.4.2.20.cpp -- satellite.container.list.insert(n, x), built into satl
//
// satellite.container.list.insert(n, x)   1 4 2 20
// Puts x in at item n, counting from 1, CHANGING the list on the name; n may be one past the last
// item, which is appending. What goes in must fit the list<type> the name was declared.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.insert(n, x)` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_container_method ran for a
// list until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"
#include "../../satellite/bytecode/type_shape.hpp"
#include "../../satellite/bytecode/word_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *home, const TypeShape *shape, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    satelliteList *body = list_to_change(home, name, "insert", context);
    if (body == nullptr)
        return Value();
    const std::size_t held = body->items.size();
    // WHAT GOES IN MUST FIT WHAT THE NAME PROMISED, AS ITS PLACE HOLDS IT -- 1.4.2.1.cpp, .append's, says why.
    std::string unfit;
    Value item = arguments[1];
    const TypeShape *type = shape != nullptr ? &arm_holding(*shape, receiver) : nullptr;
    const bool typed = type != nullptr && type->word == word::fixed_code<1, 4, 2> && !type->parameters.empty();
    if (typed)
        stored_as(type->parameters[0], item);
    if (typed && !value_fits(type->parameters[0], item, unfit)) {
        context.refuse(types_do_not_meet, name + ".satellite.insert: " + unfit);
        return Value();
    }
    unsigned long long int position = 0;
    std::string written;
    if (!position_argument(arguments.front(), false, name, "insert", position, written, context))
        return Value();
    // `.insert` MAY NAME ONE PAST THE LAST ITEM -- inserting at size + 1 is
    // appending, and refusing it would make a loop that fills a list from the
    // end stop one short for no reason.
    const unsigned long long int most = held + 1;
    if (position == 0) {
        context.refuse(counts_from_one, name + ".satellite.insert(0, ...): satellite counts from 1, not 0, so a new item goes in "
                                               "at 1 (the front) to " + std::to_string(most) + " (the end)");
        return Value();
    }
    if (position > most) {
        context.refuse(line_past_the_end,
                       name + ".satellite.insert(" + written + "): " +
                           (held == 0 ? std::string("the list is empty")
                                      : "the list holds " + std::to_string(held) + (held == 1 ? " item" : " items") +
                                            ", counting from 1") +
                           ", so a new one goes in at 1 to " + std::to_string(most));
        return Value();
    }
    body->items.insert(body->items.begin() + static_cast<std::ptrdiff_t>(position - 1), std::move(item));
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_20(LibraryRow *row)
{
    row->name = "satellite.container.list.insert(n, x)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 20;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
