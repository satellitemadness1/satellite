// satellite-004/satellite-numbers/satellite.variable.string.ends_with(x)/satellite.variable.string.ends_with(x).satellite.cpp
//
// satellite.variable.string.ends_with(x)   1 6 1 7
// True when the string finishes with the text.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.7.so by
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
    const std::u32string &tail = arguments.strings[0].text;
    answer.kind = StringAnswer::Kind::flag;
    answer.flag = tail.size() <= self.text.size() &&
                  self.text.compare(self.text.size() - tail.size(), tail.size(), tail) == 0;
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.ends_with(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 7;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
