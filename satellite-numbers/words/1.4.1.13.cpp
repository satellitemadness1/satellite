// satellite-numbers/words/1.4.1.13.cpp -- satellite.container.map.sort(by), built into satl
//
// satellite.container.map.sort(by)   1 4 1 13
// A new map with the entries in order: by key for .sort() and .sort("key"), by value for .sort("value") --
// the width first, then a to z and 0 to 9. ONE NUMBER FOR ALL THREE (2026-10-11): it takes nothing or one
// word, as it did before it had a number, and the word says which half is put in order. The name keeps its
// map; the answer is a copy, as a list's .sort() is.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.sort("value")` reaches the function below
// through the function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and
// missing brackets first -- and the code in it is what call_container_method ran for a map until that day, moved
// here whole. ONE MAP'S NUMBER FOR EVERY VALUE OF ITS KIND: what satellite.container.index declares, the arguments
// variable and an info's items are maps too, and run it.
// The number was given that day, the next free under satellite.container.map (words/words_004.tsv).

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"
#include "../../satellite/satellite_object/satellite_map_sort.hpp"
#include "../../satellite/satellite_variable_string/satellite_string.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::map_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    // ON A MAP IT ORDERS THE ENTRIES (the author, 2026-09-26): `.sort()` and
    // `.sort("key")` by key, `.sort("value")` by value -- the width first, then a to
    // z and 0 to 9. satellite_map_sort.hpp says how, and why the answer is the map,
    // a copy, as a list's is. The word may be any string, written or worked out.
    std::string by = "key";
    if (!arguments.empty()) {
        const satellite_string *word = arguments.front().as_string();
        if (word == nullptr) {
            context.refuse(types_do_not_meet,
                           name + ".satellite.sort" + container_takes(token::sort_token, 0) + ", and was given " +
                               arguments.front().kind_name());
            return Value();
        }
        by = word->to_utf8();
        const std::string refused = sort_word_refused(by, true, name);
        if (!refused.empty()) {
            context.refuse(satl_line_not_understood, refused);
            return Value();
        }
    }
    const IndexHandle *keys = receiver.as_index();
    const satelliteIndex *held = keys == nullptr ? nullptr : keys->get();
    if (held == nullptr)
        return receiver;
    // EVERY KIND OF KEY, OR OF VALUE, IS ASKED FOR AN ORDER FIRST, so a number
    // key beside a string key -- or two list values -- is refused and never
    // guessed at (satellite_map_sort.hpp: WHAT HAS NO ORDER IS NOT GUESSED AT).
    const bool by_key = by == "key";
    std::string why;
    const signed long long int comparable = entries_comparable(*held, by_key, why);
    if (comparable != success) {
        context.refuse(comparable,
                       name + ".satellite.sort" + (arguments.empty() ? "()" : "(\"" + by + "\")") + ": " + why);
        return Value();
    }
    return Value::of_index(by_key ? sorted_by_key(*held) : sorted_by_value(*held));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_13(LibraryRow *row)
{
    row->name = "satellite.container.map.sort(by)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 13;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
