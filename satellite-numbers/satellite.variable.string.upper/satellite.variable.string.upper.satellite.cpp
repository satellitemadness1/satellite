// satellite-004/satellite-numbers/satellite.variable.string.upper/satellite.variable.string.upper.satellite.cpp
//
// satellite.variable.string.upper   1 6 1 9
// The text with a..z made A..Z; the original is unchanged. Only a..z, as in 003.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.9.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &/* arguments */, StringAnswer &answer)
{
    answer.kind = StringAnswer::Kind::string;
    answer.text = self;
    for (char32_t &c : answer.text.text)
        if (c >= U'a' && c <= U'z')
            c = c - U'a' + U'A';
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
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
