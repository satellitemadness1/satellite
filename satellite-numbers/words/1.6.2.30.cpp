// satellite-numbers/words/1.6.2.30.cpp -- satellite.variable.file.unlock, built into satl
//
// satellite.variable.file.unlock   1 6 2 30
// Turns the author's lock on the file OFF again. It answers nothing.
// //
// RUN BEFORE THE HOLD AND OUTSIDE IT, as lock is (1.6.2.29.cpp).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author: "we still need to get the word satellite onto
// everything ... everything builtin needs the word satellite still"): `f.satellite.unlock()` reaches the
// function below through the function table -- bytecode/file_calls.cpp's run_file_number, which has refused a
// wrong count and missing brackets first -- and the code in it is what expression.cpp's file branch ran until
// that day, moved here whole.
// The number is new that day, the next free under 1 6 2 after SATELLITE_FILE_OPERATIONS.md 3.3's words.

#include "../number_row.hpp"
#include "../../satellite/bytecode/file_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_variable_file/satellite_file.hpp"

#include <atomic>

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &, ExpressionContext &)
{
    receiver.as_file()->lock.on.store(false, std::memory_order_release);
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_2_30(LibraryRow *row)
{
    row->name = "satellite.variable.file.unlock";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 2;
    row->numbers[3] = 30;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
