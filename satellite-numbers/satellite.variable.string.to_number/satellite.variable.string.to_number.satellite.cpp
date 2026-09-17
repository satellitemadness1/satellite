// satellite-004/satellite-numbers/satellite.variable.string.to_number/satellite.variable.string.to_number.satellite.cpp
//
// satellite.variable.string.to_number   1 6 1 13
// The text read as a number. Waits for satellite_number (PLAN M4).
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.13.so by
// build_libraries.py. Behaviour ported from 003 06's
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

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.to_number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 13;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
