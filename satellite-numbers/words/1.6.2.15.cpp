// satellite-numbers/words/1.6.2.15.cpp -- satellite.variable.file.index_of(x), built into satl
//
// satellite.variable.file.index_of(x)   1 6 2 15
// The first line that IS exactly x, counting from 1, or 0 when no line is -- 0 is never a line. The
// author's seek("an entire line").
// Needs an open file: run_file_number stops the program first, S503, when it is not -- an answer made up for a
// file that is not open would be wrong and not say so.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.index_of(x)` reaches the
// function below through the function table -- bytecode/file_calls.cpp's run_file_number, which has refused a
// wrong count, missing brackets and a file that is not open first -- and the code in it is what
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
    std::string text;
    if (!text_of(arguments[0], text, Said{name, "index_of"}, context)) return Value();
    return a_number(receiver.as_file()->index_of(text));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_15(LibraryRow *row)
{
    row->name = "satellite.variable.file.index_of(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 15;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
