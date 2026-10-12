// satellite-numbers/words/1.6.2.21.cpp -- satellite.variable.file.remove_last, built into satl
//
// satellite.variable.file.remove_last   1 6 2 21
// Takes out the last line. false on an empty file.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.remove_last()` reaches the
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

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    return Value::of_bool(receiver.as_file()->remove_last());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_21(LibraryRow *row)
{
    row->name = "satellite.variable.file.remove_last";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 21;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
