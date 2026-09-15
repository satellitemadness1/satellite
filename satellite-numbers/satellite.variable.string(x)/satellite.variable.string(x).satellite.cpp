// satellite-004/satellite-numbers/satellite.variable.string(x)/satellite.variable.string(x).satellite.cpp
//
// satellite.variable.string(x)   1 6 1 18
// string(x): the value as text. A string answers itself; any other kind waits for 004's value model.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.18.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &arguments, StringAnswer &answer)
{
    (void)self;
    if (arguments.strings.size() != 1 || !arguments.positions.empty())
        return not_built_yet;
    answer.kind = StringAnswer::Kind::string;
    answer.text = arguments.strings[0];
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 18;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
