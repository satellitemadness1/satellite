// satellite-004/satellite-numbers/satellite.variable.string.append(x)/satellite.variable.string.append(x).satellite.cpp
//
// satellite.variable.string.append(x)   1 6 1 14
// Adds text to the end, CHANGING the string it is called on (003's one mutating string method with clear).
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.14.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &arguments, StringAnswer &answer)
{
    if (arguments.strings.size() < 1)
        return error;
    self.text += arguments.strings[0].text;
    answer.kind = StringAnswer::Kind::nothing;
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.append(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 14;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
