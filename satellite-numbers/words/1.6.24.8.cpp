// satellite-numbers/words/1.6.24.8.cpp -- satellite.variable.bash.end(), built into satl
//
// satellite.variable.bash.end()   1 6 24 8
// Stops the run that was going -- it and everything under it, asked with SIGTERM and killed five seconds
// later if it has not gone -- and answers its exit code: 143, 137, or its own if it had already ended. It
// closes its input, and counts as joined. Before any start(): S741.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `c.satellite.end()` on a
// bash line reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a bash line's number, 1 6 24 n, over 1 6 23 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::end, what call_program_method ran for end()
// until that day, moved out whole: one body for a program's word and a bash line's.

#include "../number_row.hpp"
#include "../../satellite/bytecode/program_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    return program_method_parts::end(*receiver.program_handle());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_24_8(LibraryRow *row)
{
    row->name = "satellite.variable.bash.end()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 24;
    row->numbers[3] = 8;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
