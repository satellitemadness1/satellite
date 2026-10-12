// satellite-numbers/words/1.6.24.10.cpp -- satellite.variable.bash.quit(), built into satl
//
// satellite.variable.bash.quit()   1 6 24 10
// THE THIRD NAME OF 1 6 24 8, end() -- ".end() .exit() and .quit() and shutdown() all do the same thing"
// (the author, 2026-10-01). Its own word and its own number, running end()'s one body: its token is its own
// since the review of steps 2-5, so a spacesuit's own capsule named quit stays a name of its own.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything,
// before we do anything ... everything builtin needs the word satellite still"): `c.satellite.quit()` on a
// bash line reaches the function below through the function table -- bytecode/program_calls.cpp's
// run_program_number, which has refused a wrong count, a name that holds no program yet and a run never
// started (S741) first, and chosen a bash line's number, 1 6 24 n, over 1 6 23 n by the value itself -- and
// the body it runs is program_calls.cpp's program_method_parts::end, what call_program_method ran for
// quit() until that day, moved out whole: one body for a program's word and a bash line's.

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

signed long long int describe_1_6_24_10(LibraryRow *row)
{
    row->name = "satellite.variable.bash.quit()";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 24;
    row->numbers[3] = 10;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
