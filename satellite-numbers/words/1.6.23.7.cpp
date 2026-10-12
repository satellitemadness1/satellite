// satellite-numbers/words/1.6.23.7.cpp -- satellite.variable.program.start(how), built into satl
//
// satellite.variable.program.start(how)   1 6 23 7
// start("hide") -- "the output will be displayed unless my_program.start("hide") is called" (the author,
// 2026-10-01): the program started with its output and its errors thrown away, and answered as start()
// answers. "hide" is the one word it takes; any other is refused, S110. start() and start(how) are one
// spelling, and run_program_number runs this number when the brackets hold one thing.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"):
// `p.satellite.start("hide")` on a program reaches the function below through the function table --
// bytecode/program_calls.cpp's run_program_number, which has refused a wrong count and a name that holds no
// program yet first, and chosen a program's number, 1 6 23 n, over 1 6 24 n by the value itself -- and the
// body it runs is program_calls.cpp's program_method_parts::start_how, what call_program_method ran for
// start() until that day, moved out whole: one body for a program's word and a bash line's.

#include "../number_row.hpp"
#include "../../satellite/bytecode/program_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    return program_method_parts::start_how(*receiver.program_handle(), arguments[0], name, context);
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_23_7(LibraryRow *row)
{
    row->name = "satellite.variable.program.start(how)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 23;
    row->numbers[3] = 7;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
