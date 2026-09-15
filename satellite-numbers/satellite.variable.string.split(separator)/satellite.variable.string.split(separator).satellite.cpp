// satellite-004/satellite-numbers/satellite.variable.string.split(separator)/satellite.variable.string.split(separator).satellite.cpp
//
// satellite.variable.string.split(separator)   1 6 1 10
// The pieces between separators, as a list of strings. 003's rules: an EMPTY separator splits into characters; empty pieces are kept ("a,,b" is three pieces), because a dropped piece loses a CSV column.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.10.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &arguments, StringAnswer &answer)
{
    if (arguments.strings.size() < 1)
        return error;
    const std::u32string &separator = arguments.strings[0].text;
    answer.kind = StringAnswer::Kind::strings;
    answer.list.clear();
    if (separator.empty()) {
        for (char32_t c : self.text)
            answer.list.push_back(satellite_string{std::u32string(1, c)});
        return success;
    }
    size_t at = 0;
    for (;;) {
        const size_t next = self.text.find(separator, at);
        if (next == std::u32string::npos)
            break;
        answer.list.push_back(satellite_string{self.text.substr(at, next - at)});
        at = next + separator.size();
    }
    answer.list.push_back(satellite_string{self.text.substr(at)});
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.split(separator)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 10;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
