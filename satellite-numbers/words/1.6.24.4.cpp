// satellite-numbers/words/1.6.24.4.cpp -- satellite.variable.bash.join(), built into satl
//
// satellite.variable.bash.join()   1 6 24 4
// Waits for the run that was going when it was reached, and answers its exit code: 127 for no such program,
// 126 for one that may not be run, 128 + the signal for one a signal ended. It closes its input, and counts
// as joined: S742 at the end of the run is for a start() nothing joined. Before any start(): S741.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `c.satellite.join()` on a
// bash line reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a bash line's number, 1 6 24 n, over 1 6 23 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::join, what call_program_method ran for
// join() until that day, moved out whole: one body for a program's word and a bash line's.

#include "../number_row.hpp"
#include "../../satellite/bytecode/program_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &context)
{
    return program_method_parts::join(*receiver.program_handle(), context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_24_4(LibraryRow *row)
{
    row->name = "satellite.variable.bash.join()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 24;
    row->numbers[3] = 4;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
