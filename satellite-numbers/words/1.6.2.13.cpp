// satellite-numbers/words/1.6.2.13.cpp -- satellite.variable.file.insert(n, x), built into satl
//
// satellite.variable.file.insert(n, x)   1 6 2 13
// x becomes line n, counting from 1, and the lines below move down; n = size + 1 is an append, and any
// later n answers false. The author's seek + write, with no hidden cursor (SATELLITE_FILE_OPERATIONS.md 3.4).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.insert(n, x)` reaches the
// function below through the function table -- bytecode/file_calls.cpp's run_file_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what call_file_method ran until that day,
// moved here whole.
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
    std::string text;
    std::size_t n = 0;
    const Said said{name, "insert"};
    if (!number_of(arguments[0], n, said, context) || !text_of(arguments[1], text, said, context)) return Value();
    return Value::of_bool(receiver.as_file()->insert(n, text));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_13(LibraryRow *row)
{
    row->name = "satellite.variable.file.insert(n, x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 13;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
