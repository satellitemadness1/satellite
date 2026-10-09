// satellite-numbers/words/1.6.1.17.cpp -- satellite.variable.string.resolved, built into satl
//
// satellite.variable.string.resolved   1 6 1 17
// 003 answered its six live escapes (\\threads, \\home ...) here. 004 has no live escapes yet, so the string answers itself.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_17, is the one
// word_table.cpp calls for 1 6 1 17 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &/* arguments */, StringAnswer &answer)
{
    answer.kind = StringAnswer::Kind::string;
    answer.text = self;
    return success;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_17(LibraryRow *row)
{
    row->name = "satellite.variable.string.resolved";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 17;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
