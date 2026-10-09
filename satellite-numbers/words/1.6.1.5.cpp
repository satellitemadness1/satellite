// satellite-numbers/words/1.6.1.5.cpp -- satellite.variable.string.substring(start, end), built into satl
//
// satellite.variable.string.substring(start, end)   1 6 1 5
// The piece from start (included) to end (not included). 003's checks, in 003's order: a negative position, start after end, end past the last character. end == size is the whole tail and legal.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_5, is the one
// word_table.cpp calls for 1 6 1 5 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &arguments, StringAnswer &answer)
{
    if (arguments.positions.size() < 2)
        return error;
    const signed long long int start = arguments.positions[0], end = arguments.positions[1];
    if (start < 0 || end < 0)
        return not_a_position;
    if (start > end)
        return positions_backwards;
    if (static_cast<unsigned long long int>(end) > self.text.size())
        return position_past_the_end;
    answer.kind = StringAnswer::Kind::string;
    answer.text.text = self.text.substr(start, end - start);
    return success;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_5(LibraryRow *row)
{
    row->name = "satellite.variable.string.substring(start, end)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 5;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
