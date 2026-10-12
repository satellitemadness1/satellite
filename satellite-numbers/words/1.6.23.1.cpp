// satellite-numbers/words/1.6.23.1.cpp -- satellite.variable.program.start(), built into satl
//
// satellite.variable.program.start()   1 6 23 1
// Starts it, and answers at once -- the program itself, so a chain goes on from it; its output shows in
// satl's console, between satl's own lines, as it comes. Still running: S740. A run that could not start is
// not a refusal: ok() answers false, and error() says why.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `p.satellite.start()` on a
// program reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count and a name that holds no program yet first, and
// chosen a program's number, 1 6 23 n, over 1 6 24 n by the value itself -- and the body it runs is
// program_calls.cpp's program_method_parts::start, what call_program_method ran for start() until that day,
// moved out whole: one body for a program's word and a bash line's.

#include "../number_row.hpp"
#include "../../satellite/bytecode/program_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    return program_method_parts::start(*receiver.program_handle(), name, false, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_23_1(LibraryRow *row)
{
    row->name = "satellite.variable.program.start()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 23;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
