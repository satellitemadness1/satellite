// satellite-numbers/words/1.6.1.12.cpp -- satellite.variable.string.replace(a, b), built into satl
//
// satellite.variable.string.replace(a, b)   1 6 1 12
// Every appearance of a swapped for b, left to right, never rescanning what was written in (so
// replace("a", "aa") ends). 003 refuses an empty a, and so does this (S421).
//
// RUN BY ITS NUMBER SINCE 2026-10-10 (the author: "we have to give numbers to things that don't have
// numbers yet, like .find() and .replace and .append() ... and it actually runs that number when we do
// it"): `s.replace(a, b)` reaches the function below through the function table --
// bytecode/string_calls.cpp's run_string_number -- and the code in it is what
// bytecode/string_calls.cpp's switch ran until that day, moved here whole.
// The 32-bit copy of 003's behaviour this file held from 2026-09-14 (a .so until 2026-10-07) is gone:
// where a method had two copies, the author kept the one programs ran.

#include "../number_row.hpp"
#include "../../satellite/bytecode/string_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/string_pieces.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::string_method_parts;

Value method(const Value &receiver, Value *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const satellite_string &text = *receiver.as_string();
    const std::string call = name + ".satellite.replace";
    // TEXT GOING IN, BOTH: a string, or a number's digits (S020).
    satellite_string made_first, made_second;
    const satellite_string *first = text_argument(arguments[0], made_first, call, context);
    if (first == nullptr)
        return Value();
    const satellite_string *second = text_argument(arguments[1], made_second, call, context);
    if (second == nullptr)
        return Value();
    if (first->empty()) {
        context.refuse(empty_search_text, call + " was given \"\" to look for, and an empty text is found "
                                                 "everywhere -- there is nothing to replace");
        return Value();
    }
    return Value::of_string(string_pieces::replace_all(text, *first, *second));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_12(LibraryRow *row)
{
    row->name = "satellite.variable.string.replace(a, b)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 12;
    row->depth = 4;
    row->scenarios.string_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
