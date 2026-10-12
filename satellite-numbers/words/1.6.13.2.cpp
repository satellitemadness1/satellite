// satellite-numbers/words/1.6.13.2.cpp -- satellite.variable.thread.join(), built into satl
//
// satellite.variable.thread.join()   1 6 13 2
// Waits for the thread to end, and answers what its capsule handed back -- nothing, if it handed back
// nothing or was stopped. Before start() it is S723; a second join answers the same again, with S724 said
// once as a notice (003's Q2); a wait that would never end is S728; and a thread that failed stops the
// joiner with its own code. 003's number, kept when 004 numbered its words (2026-09-14).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `t.satellite.join()`
// reaches the function below through the function table -- bytecode/thread_calls.cpp's run_thread_number,
// which has refused a count that is not nothing and a name that holds no thread first -- and the body it
// runs is thread_calls.cpp's thread_method_parts::join, what call_thread_method ran for join() until that
// day, moved out of its anonymous namespace whole.

#include "../number_row.hpp"
#include "../../satellite/bytecode/thread_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    return thread_method_parts::join(*receiver.thread_handle(), name, "join", context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_13_2(LibraryRow *row)
{
    row->name = "satellite.variable.thread.join()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 13;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
