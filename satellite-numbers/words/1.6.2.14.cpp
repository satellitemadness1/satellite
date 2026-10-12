// satellite-numbers/words/1.6.2.14.cpp -- satellite.variable.file.replace(a, b), built into satl
//
// satellite.variable.file.replace(a, b)   1 6 2 14
// replace(n, x) makes line n into x; replace(a, b) changes the first a found, inside one line, into b.
// ONE NUMBER FOR BOTH SHAPES (NM-8, 2026-10-11): a method is one number, as a string's replace is one
// (1 6 1 12), and the first argument's KIND picks the shape as it runs, as it always has.
// SATELLITE_FILE_OPERATIONS.md 3.3 had proposed two rows, 14 and 15; two would make the walker choose a
// number by looking at an argument before running it, which is what this function already does inside.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.replace(n, x)` and
// `f.satellite.replace(a, b)` reach the function below through the function table -- bytecode/file_calls.cpp's
// run_file_number, which has refused a wrong count and missing brackets first -- and the code in it is what
// call_file_method ran until that day, moved here whole.
// The number is new that day: the next free under 1 6 2, in the order SATELLITE_FILE_OPERATIONS.md 3.3 gave a
// file's words, with replace as one.

#include "../number_row.hpp"
#include "../../satellite/bytecode/file_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_variable_file/satellite_file.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::file_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool, const std::string &name, ExpressionContext &context)
{
    // TWO SHAPES, ONE WORD (the author, 2026-09-18: "we will do .replace(line_number,
    // "string") or .replace will optionally take this: .replace("string", "with
    // string") so it will have two syntaxes"). The first argument's KIND chooses.
    satellite_file &file = *receiver.as_file();
    std::string text, other;
    std::size_t n = 0;
    const Said said{name, "replace"};
    if (arguments[0].is_number()) {
        if (!number_of(arguments[0], n, said, context) || !text_of(arguments[1], text, said, context))
            return Value();
        return Value::of_bool(file.replace_line(n, text));
    }
    if (!text_of(arguments[0], text, said, context) || !text_of(arguments[1], other, said, context))
        return Value();
    return Value::of_bool(file.replace_text(text, other));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_14(LibraryRow *row)
{
    row->name = "satellite.variable.file.replace(a, b)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 14;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
