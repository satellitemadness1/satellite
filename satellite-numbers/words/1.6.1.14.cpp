// satellite-numbers/words/1.6.1.14.cpp -- satellite.variable.string.append(x), built into satl
//
// satellite.variable.string.append(x)   1 6 1 14
// Adds text to the end, CHANGING the string it is called on (003's one mutating string method with
// clear). A number given is its digits (S020).
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.append(x)` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what
// bytecode/string_calls.cpp's switch ran until that day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::string_method_parts;

Value method(const Value &, Value *home, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const std::string call = name + ".satellite.append";
    // append AND clear CHANGE THE STRING, so they need the variable's own: a literal or a
    // piece of a chain is a string nothing holds, and changing it is a line that does
    // nothing (container_calls.cpp's words for `{1}.append(2)`). What they answer is not
    // used: the chain stays on the name, so `s.append("a").append("b")` appends twice.
    satellite_string *own = home == nullptr ? nullptr : home->as_string();
    if (own == nullptr) {
        context.refuse(satl_line_not_understood, call + " changes a string, and this one has no name to change");
        return Value();
    }
    satellite_string made;
    const satellite_string *added = text_argument(arguments[0], made, call, context);
    if (added != nullptr)
        own->append(*added);            // an argument is its own copy, never the string it goes onto
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_14(LibraryRow *row)
{
    row->name = "satellite.variable.string.append(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 14;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
