// satellite-numbers/words/1.6.2.9.cpp -- satellite.variable.file.path, built into satl
//
// satellite.variable.file.path   1 6 2 9
// The path the file was opened on, as it was given -- never a reason to stop (a_string_always).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.path` reaches the function
// below through the function table -- bytecode/file_calls.cpp's run_file_number, which has refused a wrong
// count and missing brackets first -- and the code in it is what call_file_method ran until that day, moved
// here whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); nothing ran it until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/file_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_variable_file/satellite_file.hpp"

namespace {

using namespace satellite004;
using namespace satellite004::file_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    return a_string_always(receiver.as_file()->path());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_9(LibraryRow *row)
{
    row->name = "satellite.variable.file.path";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
