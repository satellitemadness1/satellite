// satellite-004/satellite-numbers/satellite.variable.string.hex/satellite.variable.string.hex.satellite.cpp
//
// satellite.variable.string.hex   1 6 1 22
// The text as hex. Waits for 004's hex type.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.22.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &/* arguments */, StringAnswer &answer)
{
    (void)self;
    (void)answer;
    return not_built_yet;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.hex";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 22;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
