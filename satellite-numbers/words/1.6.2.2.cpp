// satellite-numbers/words/1.6.2.2.cpp -- satellite.variable.file.open, built into satl
//
// satellite.variable.file.open   1 6 2 2
// Opens a closed handle again and reads the disk afresh (satellite_file::reopen). A handle already
// open answers true.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.open()` reaches the function
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

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    return Value::of_bool(receiver.as_file()->reopen());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_2(LibraryRow *row)
{
    row->name = "satellite.variable.file.open";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
