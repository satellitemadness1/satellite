// satellite-numbers/words/1.6.1.20.cpp -- satellite.variable.string.number, built into satl
//
// satellite.variable.string.number   1 6 1 20
// The text read as a number. Waits for satellite_number (PLAN M4).
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_20, is the one
// word_table.cpp calls for 1 6 1 20 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &/* arguments */, StringAnswer &answer)
{
    (void)self;
    (void)answer;
    return not_built_yet;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_20(LibraryRow *row)
{
    row->name = "satellite.variable.string.number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 20;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
