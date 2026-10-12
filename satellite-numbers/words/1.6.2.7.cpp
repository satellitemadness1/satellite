// satellite-numbers/words/1.6.2.7.cpp -- satellite.variable.file.exists, built into satl
//
// satellite.variable.file.exists   1 6 2 7
// Whether the handle's path is still a regular file -- asked of the absolute path kept when it was
// opened, so a directory change since then does not matter.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.exists` reaches the function
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
    return Value::of_bool(receiver.as_file()->path_exists());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_7(LibraryRow *row)
{
    row->name = "satellite.variable.file.exists";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 7;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
