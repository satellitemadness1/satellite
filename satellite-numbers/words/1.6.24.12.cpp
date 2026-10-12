// satellite-numbers/words/1.6.24.12.cpp -- satellite.variable.bash.pass(text), built into satl
//
// satellite.variable.bash.pass(text)   1 6 24 12
// Types a string -- or each item of a list -- into the bash line as it runs, a line each: "typed input
// while the program is running" (the author, 2026-10-01). It answers true when it was running and could
// still take input, and false once it had ended, once join() or end() closed its input, or once it closed
// its input itself. A value with no text is refused. Before any start(): S741.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `c.satellite.pass(text)`
// on a bash line reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a bash line's number, 1 6 24 n, over 1 6 23 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::pass, what call_program_method ran for
// pass() until that day, moved out whole: one body for a program's word and a bash line's.

#include "../number_row.hpp"
#include "../../satellite/bytecode/program_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    return program_method_parts::pass(*receiver.program_handle(), arguments[0], name, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_24_12(LibraryRow *row)
{
    row->name = "satellite.variable.bash.pass(text)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 24;
    row->numbers[3] = 12;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
