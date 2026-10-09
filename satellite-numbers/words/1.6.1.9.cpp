// satellite-numbers/words/1.6.1.9.cpp -- satellite.variable.string.upper, built into satl
//
// satellite.variable.string.upper   1 6 1 9
// The text with a..z made A..Z; the original is unchanged. Only a..z, as in 003.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_9, is the one
// word_table.cpp calls for 1 6 1 9 at start-up. Until then this was a .so, built by
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
    for (char32_t &c : answer.text.text)
        if (c >= U'a' && c <= U'z')
            c = c - U'a' + U'A';
    return success;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_1_9(LibraryRow *row)
{
    row->name = "satellite.variable.string.upper";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 9;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
