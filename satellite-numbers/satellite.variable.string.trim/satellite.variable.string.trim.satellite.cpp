// satellite-004/satellite-numbers/satellite.variable.string.trim/satellite.variable.string.trim.satellite.cpp
//
// satellite.variable.string.trim   1 6 1 11
// The text without space, tab, carriage return or newline at either end, as in 003.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.11.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &/* arguments */, StringAnswer &answer)
{
    auto blank = [](char32_t c) { return c == U' ' || c == U'\t' || c == U'\r' || c == U'\n'; };
    size_t first = 0, last = self.text.size();
    while (first < last && blank(self.text[first]))
        first++;
    while (last > first && blank(self.text[last - 1]))
        last--;
    answer.kind = StringAnswer::Kind::string;
    answer.text.text = self.text.substr(first, last - first);
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.trim";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 11;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
