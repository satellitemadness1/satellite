// satellite-004/satellite-numbers/satellite.variable.string.lower/satellite.variable.string.lower.satellite.cpp
//
// satellite.variable.string.lower   1 6 1 8
// The text with A..Z made a..z; the original is unchanged. Only A..Z, as in 003 (case for every alphabet needs Unicode's case tables: a separate decision).
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.8.so by
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
        if (c >= U'A' && c <= U'Z')
            c = c - U'A' + U'a';
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.lower";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 8;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
