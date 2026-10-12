// satellite-numbers/words/1.6.13.1.cpp -- satellite.variable.thread.start(), built into satl
//
// satellite.variable.thread.start()   1 6 13 1
// Begins the capsule satellite.thread.new was handed, on a thread of its own, and answers the thread at
// once -- so `t.satellite.start().satellite.join()` goes on. A thread runs once: a second start() is S722,
// and a machine that will not make one more thread is S725. 003's number, kept when 004 numbered its words
// (2026-09-14).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `t.satellite.start()`
// reaches the function below through the function table -- bytecode/thread_calls.cpp's run_thread_number,
// which has refused a count that is not nothing and a name that holds no thread first -- and the body it
// runs is thread_calls.cpp's thread_method_parts::start, what call_thread_method ran for start() until that
// day, moved out of its anonymous namespace whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/thread_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    return thread_method_parts::start(*receiver.thread_handle(), name, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_13_1(LibraryRow *row)
{
    row->name = "satellite.variable.thread.start()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 13;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
