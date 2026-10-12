// satellite-numbers/words/1.6.24.5.cpp -- satellite.variable.bash.code(), built into satl
//
// satellite.variable.bash.code()   1 6 24 5
// THE SECOND NAME OF 1 6 24 4, join() -- ".code() .return() ... will be for the return" (the author,
// 2026-10-01): the same wait and the same exit code. Its own word and its own number, running join()'s one
// body: its token is its own since the review of steps 2-5, so a spacesuit's own field named code stays a
// name of its own.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `c.satellite.code()` on a
// bash line reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a bash line's number, 1 6 24 n, over 1 6 23 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::join, what call_program_method ran for
// code() until that day, moved out whole: one body for a program's word and a bash line's.

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

signed long long int describe_1_6_24_5(LibraryRow *row)
{
    row->name = "satellite.variable.bash.code()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 24;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
