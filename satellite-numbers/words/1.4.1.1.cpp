// satellite-numbers/words/1.4.1.1.cpp -- satellite.container.map.set(k, v), built into satl
//
// satellite.container.map.set(k, v)   1 4 1 1
// Files v under the key k -- `m[k] = v`, said in a word -- CHANGING the map on the name, and answers nothing.
// The key and the value must fit the map<key, value> the name was declared as, in the words m[k] = v uses.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `m.satellite.set(k, v)` reaches the function below through
// the function table -- bytecode/container_calls.cpp's run_map_number, which has refused a wrong count and missing
// brackets first. From 2026-09-26 until that day the word had no token, and expression.cpp's one_operand matched it
// by its name, straight on a name that held a map, and called write_through_index -- the function `m[k] = v` is
// written by, which this calls still, so the two cannot come to say different things. ONE MAP'S NUMBER FOR EVERY
// VALUE OF ITS KIND: what satellite.container.index declares, the arguments variable and an info's items are maps
// too, and run it.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own
// until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/container_calls.hpp"
#include "../../satellite/bytecode/expression.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_index.hpp"
#include "../../satellite/bytecode/type_shape.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::map_method_parts;

Value method(const Value &, Value *home, const TypeShape *shape, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    // `m.set(k, v)` IS `m[k] = v`, THROUGH THE SAME FUNCTION (the author found it refused on 2026-09-26,
    // ERRORS2 1b A/D, and his programs call it): a key of the wrong type and a value that does not fit are
    // refused in the words the brackets use, and a new key goes in last.
    if (map_to_change(home, name, "set", context) == nullptr)
        return Value();
    static const TypeShape kAnything;
    write_under_a_key(*home, arguments[0], arguments[1], name, shape != nullptr ? *shape : kAnything, context);
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_4_1_1(LibraryRow *row)
{
    row->name = "satellite.container.map.set(k, v)";
    row->numbers[0] = 1;
    row->numbers[1] = 4;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
