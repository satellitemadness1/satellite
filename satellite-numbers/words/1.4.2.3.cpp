// satellite-numbers/words/1.4.2.3.cpp -- satellite.container.list.sort(), built into satl
//
// satellite.container.list.sort()   1 4 2 3
// A COPY of the list, ordered by nothing yet: the .by_name() or .by_value() after it orders it, and
// .reverse() after either goes the other way (the author, 2026-09-18). A word in the brackets is a
// map's, so a list refuses one.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "so .list.satellite.append() has it's own number,
// and it actually runs that number when we do it"): `names.sort()` reaches the function below through
// the function table -- bytecode/container_calls.cpp's run_list_number, which has refused a wrong
// count and missing brackets first -- and the code in it is what call_container_method ran for a list
// until that day, moved here whole.
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
    // `.sort()` ON A LIST TAKES A COPY AND ORDERS NOTHING. The ordering is the
    // `.by_name()` or `.by_value()` that follows, which is the author's own
    // spelling -- and the copy is what stops
    // `display(names.sort().by_name())` quietly reordering names. "key" and
    // "value" in the brackets are a map's (the author, 2026-09-26), and any word
    // at all is refused on a list, in sort_word_refused's sentence.
    if (!arguments.empty()) {
        const satellite_string *word = arguments.front().as_string();
        if (word == nullptr) {
            context.refuse(types_do_not_meet, name + ".satellite.sort" + container_takes(token::sort_token, 0) +
                                                  ", and was given " + arguments.front().kind_name());
            return Value();
        }
        context.refuse(types_do_not_meet, sort_word_refused(word->to_utf8(), false, name));
        return Value();
    }
    return receiver;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_2_3(LibraryRow *row)
{
    row->name = "satellite.container.list.sort()";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 2;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.list_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
