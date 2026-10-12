// satellite-numbers/words/1.6.2.5.cpp -- satellite.variable.file.read_all, built into satl
//
// satellite.variable.file.read_all   1 6 2 5
// The whole file as one string, every line with its own ending, INCLUDING the changes not yet saved.
// Needs an open file: run_file_number stops the program first, S503, when it is not -- an answer made up for a
// file that is not open would be wrong and not say so.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.read_all` reaches the
// function below through the function table -- bytecode/file_calls.cpp's run_file_number, which has refused a
// wrong count, missing brackets and a file that is not open first -- and the code in it is what
// call_file_method ran until that day, moved here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); nothing ran it until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/file_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_variable_file/satellite_file.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::file_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &context)
{
    return a_string(receiver.as_file()->read_all(), context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_5(LibraryRow *row)
{
    row->name = "satellite.variable.file.read_all";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
