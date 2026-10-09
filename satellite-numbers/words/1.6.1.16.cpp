// satellite-numbers/words/1.6.1.16.cpp -- satellite.variable.string.at(n), built into satl
//
// satellite.variable.string.at(n)   1 6 1 16
// The one character at position n, as a string of one character (003: there is no character type). Past the end is refused.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_16, is the one
// word_table.cpp calls for 1 6 1 16 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &arguments, StringAnswer &answer)
{
    if (arguments.positions.size() < 1)
        return error;
    const signed long long int at = arguments.positions[0];
    if (at < 0)
        return not_a_position;
    if (static_cast<unsigned long long int>(at) >= self.text.size())
        return position_past_the_end;
    answer.kind = StringAnswer::Kind::string;
    answer.text.text = std::u32string(1, self.text[at]);
    return success;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_16(LibraryRow *row)
{
    row->name = "satellite.variable.string.at(n)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 16;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
