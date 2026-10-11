// satellite-numbers/words/1.4.2.26.cpp -- satellite.container.list.search(pattern), built into satl
//
// satellite.container.list.search(pattern)   1 4 2 26
// Where an item CONTAINS the pattern, counting from 1, where .index_of wants it to BE it; 0 when
// nothing does. A list searches the text an item reads as, so {12, 345}.search(4) finds 345 at 2.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.search(x)` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_container_method ran for a
// list until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/fast_paths.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    // `.search(x)` -- WHERE AN ITEM CONTAINS x, where `.index_of` wants it to BE
    // x. A file searches the text of a line; a list searches the text an item
    // reads as, so `{12, 345}.search(4)` finds 345 at 2.
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
    satellite_string needle;
    std::string why;
    if (arguments.front().to_string(needle, why) != success) {
        context.refuse(types_do_not_meet,
                       name + ".satellite.search looks for what an item READS as, and was given " +
                           arguments.front().kind_name() + ", which has no text");
        return Value();
    }
    for (std::size_t at = 0; at < items->size(); ++at) {
        satellite_string text;
        if ((*items)[at].to_string(text, why) != success)
            continue;                       // an item with no text contains nothing
        Value found;
        if (str_find_str(Value::of_string(text), Value::of_string(needle), found) == success)
            return a_count(at + 1);
    }
    return a_count(0);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_26(LibraryRow *row)
{
    row->name = "satellite.container.list.search(pattern)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 26;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
