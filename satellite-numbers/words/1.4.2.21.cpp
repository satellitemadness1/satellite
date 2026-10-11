// satellite-numbers/words/1.4.2.21.cpp -- satellite.container.list.join(separator), built into satl
//
// satellite.container.list.join(separator)   1 4 2 21
// One string: every item as it reads, with the separator between and nothing at either end -- {1,
// 2}.join("-") is "1-2". Taken from 003, 2026-09-23.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.join(separator)` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_container_method ran for a
// list until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::container_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    // `.join(separator)` -- ONE STRING: every item as it reads, with the separator
    // between and nothing at either end. A string item is its own text and anything
    // else is what .string would make of it -- `{1, 2}.join("-")` is "1-2". The
    // separator must be a string, because what goes between text is text.
    const satellite_string *separator = arguments.front().as_string();
    if (separator == nullptr) {
        context.refuse(types_do_not_meet, name + ".satellite.join puts a string between the items, and was given " +
                                              arguments.front().kind_name());
        return Value();
    }
    std::vector<satelliteObject> borrowed;
    const std::vector<satelliteObject> &items = *items_of(receiver, borrowed);
    satellite_string out;
    for (std::size_t at = 0; at < items.size(); ++at) {
        if (at != 0)
            out.append(*separator);
        satellite_string text;
        std::string why;
        if (items[at].to_string(text, why) != success) {
            context.refuse(types_do_not_meet,
                           name + ".satellite.join: item " + std::to_string(at + 1) + " has no text to join -- " + why);
            return Value();
        }
        out.append(text);
    }
    return Value::of_string(std::move(out));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_21(LibraryRow *row)
{
    row->name = "satellite.container.list.join(separator)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 21;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
