// satellite-numbers/words/1.6.13.3.cpp -- satellite.variable.thread.stop(), built into satl
//
// satellite.variable.thread.stop()   1 6 13 3
// Asks the thread to stop at its next statement -- never in the middle of one -- and answers the thread.
// The author's, 2026-09-23.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `t.satellite.stop()`
// reaches the function below through the function table -- bytecode/thread_calls.cpp's run_thread_number,
// which has refused a count that is not nothing and a name that holds no thread first -- and the code in it
// is what call_thread_method ran for stop() until that day, moved here whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/thread_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

#include <atomic>

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    // IT ASKS, AND ENDS NOTHING ITSELF: the thread reads stop_asked between two statements and stops there,
    // S726, and a join after it answers nothing. It answers the thread, so a chain goes on from it.
    const ThreadHandle &which = *receiver.thread_handle();
    which->stop_asked.store(true, std::memory_order_relaxed);
    return Value::of_thread(which);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_13_3(LibraryRow *row)
{
    row->name = "satellite.variable.thread.stop()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 13;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
