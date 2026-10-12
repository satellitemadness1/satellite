// satellite-numbers/words/1.6.23.9.cpp -- satellite.variable.program.exit(), built into satl
//
// satellite.variable.program.exit()   1 6 23 9
// THE SECOND NAME OF 1 6 23 8, end() -- ".end() .exit() and .quit() and shutdown() all do the same thing"
// (the author, 2026-10-01). Its own word and its own number, running end()'s one body: its token is its own
// since the review of steps 2-5, so a spacesuit's own field named exit stays a name of its own.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `p.satellite.exit()` on a
// program reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a program's number, 1 6 23 n, over 1 6 24 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::end, what call_program_method ran for
// exit() until that day, moved out whole: one body for a program's word and a bash line's.

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

signed long long int describe_1_6_23_9(LibraryRow *row)
{
    row->name = "satellite.variable.program.exit()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 23;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
