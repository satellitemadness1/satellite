// satellite-004/satellite-numbers/satellite.variable.string.find(x)/satellite.variable.string.find(x).satellite.cpp
//
// satellite.variable.string.find(x)   1 6 1 3
// Where the text first appears, counting from 0. 003 REFUSES a missing text (S0716) rather than answering -1, so a program asks `contains` first.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.3.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &arguments, StringAnswer &answer)
{
    if (arguments.strings.size() < 1)
        return error;
    const size_t at = self.text.find(arguments.strings[0].text);
    if (at == std::u32string::npos)
        return text_not_found;
    answer.kind = StringAnswer::Kind::count;
    answer.count = at;
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.find(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 3;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
