// satellite-numbers/words/1.4.1.7.cpp -- satellite.container.map.remove(k), built into satl
//
// satellite.container.map.remove(k)   1 4 1 7
// Takes the entry under the key k out, CHANGING the map on the name, the order of the rest kept -- and
// refuses a key it does not hold, because a program that removes nothing should say so.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.remove(k)` reaches the function below through
// the function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
// brackets first -- and the code in it is what call_container_method ran for a map until that day, moved here
// whole. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND: what satellite.container.index declares, the arguments
// variable and an info's items are maps too, and run it.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::map_method_parts;

Value method(const Value &, Value *home, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    satelliteIndex *body = map_to_change(home, name, "remove", context);
    if (body == nullptr)
        return Value();
    if (body->entries.empty()) {
        context.refuse(line_past_the_end, name + ".satellite.remove: the index is empty");
        return Value();
    }
    // `.remove(key)` -- BY KEY, because that is the only thing an index
    // is asked about. `.contains` reads the keys too, so the pair agree.
    std::string key_name;
    if (!key_name_of(arguments.front(), key_name)) {
        context.refuse(types_do_not_meet,
                       name + ".satellite.remove was given " + arguments.front().kind_name() +
                           " as a key, and a key must be a number, a string, a bool, a binary or a percentage");
        return Value();
    }
    for (std::size_t at = 0; at < body->entries.size(); ++at) {
        std::string here;
        if (key_name_of(body->entries[at].key, here) && here == key_name) {
            take_entry_out(*body, at);
            return *home;
        }
    }
    context.refuse(text_not_found, name + ".satellite.remove: there is no such key in it");
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_7(LibraryRow *row)
{
    row->name = "satellite.container.map.remove(k)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 7;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
