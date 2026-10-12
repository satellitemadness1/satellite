// satellite-numbers/words/1.6.23.3.cpp -- satellite.variable.program.error(), built into satl
//
// satellite.variable.program.error()   1 6 23 3
// Why it could not start -- or, once it has ended, a code that was not 0 or the signal that ended it; ""
// when there is nothing to say. Before any start(): S741.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `p.satellite.error()` on a
// program reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a program's number, 1 6 23 n, over 1 6 24 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::error, what call_program_method ran for
// error() until that day, moved out whole: one body for a program's word and a bash line's.

#include "../number_row.hpp"
#include "../../satellite/bytecode/program_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &, ExpressionContext &)
{
    return program_method_parts::error(*receiver.program_handle());
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_23_3(LibraryRow *row)
{
    row->name = "satellite.variable.program.error()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 23;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
