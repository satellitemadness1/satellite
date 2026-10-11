// satellite-numbers/words/1.6.1.10.cpp -- satellite.variable.string.split(separator), built into satl
//
// satellite.variable.string.split(separator)   1 6 1 10
// The pieces between separators, as a satellite.container.list of strings. 003's rules: an EMPTY
// separator splits into characters; empty pieces are kept ("a,,b" is three pieces), because a dropped
// piece loses a CSV column.
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.split(separator)` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what
// bytecode/string_calls.cpp's switch ran until that day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/satellite_list.hpp"
#include "../../satellite/satellite_object/string_pieces.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::string_method_parts;

Value method(const Value &receiver, Value *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const satellite_string &text = *receiver.as_string();
    const std::string call = name + ".satellite.split";
    // TEXT GOING IN: a string, or a number's digits (S020).
    satellite_string made;
    const satellite_string *first = text_argument(arguments[0], made, call, context);
    if (first == nullptr)
        return Value();
    std::vector<satellite_string> parts = string_pieces::split(text, *first);
    std::vector<Value> items;
    items.reserve(parts.size());
    for (satellite_string &part : parts)
        items.push_back(Value::of_string(std::move(part)));
    return Value::of_list(make_list(std::move(items)));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_10(LibraryRow *row)
{
    row->name = "satellite.variable.string.split(separator)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 10;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
