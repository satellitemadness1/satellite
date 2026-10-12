// satellite-numbers/words/1.6.13.4.cpp -- satellite.variable.thread.wait(), built into satl
//
// satellite.variable.thread.wait()   1 6 13 4
// THE SECOND NAME OF 1 6 13 2, satellite.variable.thread.join() -- the same as join(), its second name (the
// author, 2026-09-23): the same wait and the same answer. Its own word and its own number, running join()'s
// one body, as its spelling has been a token of its own since it was built (wait_method_token; the batch
// mark's `wait` is another).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `t.satellite.wait()`
// reaches the function below through the function table -- bytecode/thread_calls.cpp's run_thread_number,
// which has refused a count that is not nothing and a name that holds no thread first -- and the body it
// runs is thread_calls.cpp's thread_method_parts::join, what call_thread_method ran for wait() until that
// day, moved out of its anonymous namespace whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/thread_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    return thread_method_parts::join(*receiver.thread_handle(), name, "wait", context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_13_4(LibraryRow *row)
{
    row->name = "satellite.variable.thread.wait()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 13;
    row->numbers[3] = 4;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
