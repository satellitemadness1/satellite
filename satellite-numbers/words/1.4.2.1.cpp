// satellite-numbers/words/1.4.2.1.cpp -- satellite.container.list.append, built into satl
//
// satellite.container.list.append   1 4 2 1
// The item on the end, CHANGING the list on the name -- the one container method that leaves the chain
// on the name, so names.append("a").append("b") appends twice. What goes in must fit the list<type>
// the name was declared.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.append(x)` reaches the function below
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
    satelliteList *body = list_to_change(home, name, "append", context);
    if (body == nullptr)
        return Value();
    // WHAT GOES IN MUST FIT WHAT THE NAME PROMISED. The same test
    // write_through_index makes for `a[1] = x`, made here for `a.append(x)` --
    // two doors into one list, and locking one of them is worth nothing.
    // ONLY A LIST'S <type> DESCRIBES AN ITEM. A `multiple<A, B>` name
    // constrains ITSELF and not what is inside it -- write_through_index's
    // rule -- and reading its A as the item type refused
    // `multiple<list, number> m = {1}` then m.append(3), "it holds a number",
    // while m[1] = 3 went in (the review, 2026-09-23).
    // A MULTIPLE IS ASKED WHICH ARM THE LIST IS HELD AS (type_shape.hpp), so
    // `multiple<list<number>, number> m = {1}` then m.append("x") is refused.
    std::string unfit;
    const TypeShape *held = shape != nullptr ? &arm_holding(*shape, receiver) : nullptr;
    if (held != nullptr && held->word == word::fixed_code<1, 4, 2> && !held->parameters.empty() &&
        !value_fits(held->parameters[0], arguments.front(), unfit)) {
        context.refuse(types_do_not_meet, name + ".satellite.append: " + unfit);
        return Value();
    }
    body->items.push_back(arguments.front());
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_1(LibraryRow *row)
{
    row->name = "satellite.container.list.append";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
