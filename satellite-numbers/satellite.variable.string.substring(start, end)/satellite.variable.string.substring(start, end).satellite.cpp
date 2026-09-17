// satellite-004/satellite-numbers/satellite.variable.string.substring(start, end)/satellite.variable.string.substring(start, end).satellite.cpp
//
// satellite.variable.string.substring(start, end)   1 6 1 5
// The piece from start (included) to end (not included). 003's checks, in 003's order: a negative position, start after end, end past the last character. end == size is the whole tail and legal.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.5.so by
// build_libraries.py. Behaviour ported from 003 06's
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

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
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
