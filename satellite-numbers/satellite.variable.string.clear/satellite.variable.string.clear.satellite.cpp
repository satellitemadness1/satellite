// satellite-004/satellite-numbers/satellite.variable.string.clear/satellite.variable.string.clear.satellite.cpp
//
// satellite.variable.string.clear   1 6 1 15
// Empties the string it is called on.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.15.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &/* arguments */, StringAnswer &answer)
{
    self.text.clear();
    answer.kind = StringAnswer::Kind::nothing;
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.clear";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 15;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
