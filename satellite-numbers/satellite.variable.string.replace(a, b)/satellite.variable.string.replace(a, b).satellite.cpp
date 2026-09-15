// satellite-004/satellite-numbers/satellite.variable.string.replace(a, b)/satellite.variable.string.replace(a, b).satellite.cpp
//
// satellite.variable.string.replace(a, b)   1 6 1 12
// Every appearance of a swapped for b, left to right, never rescanning what was written in (so replace("a", "aa") ends). 003 refuses an empty a.
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.6.1.12.so by
// build_libraries.py. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string &self, const StringArguments &arguments, StringAnswer &answer)
{
    if (arguments.strings.size() < 2)
        return error;
    const std::u32string &from = arguments.strings[0].text, &to = arguments.strings[1].text;
    if (from.empty())
        return empty_search_text;
    answer.kind = StringAnswer::Kind::string;
    std::u32string &out = answer.text.text;
    out.clear();
    size_t from_at = 0;
    for (;;) {
        const size_t at = self.text.find(from, from_at);
        if (at == std::u32string::npos)
            break;
        out.append(self.text, from_at, at - from_at);
        out.append(to);
        from_at = at + from.size();
    }
    out.append(self.text, from_at, std::u32string::npos);
    return success;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.variable.string.replace(a, b)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 12;
    row->depth = 4;
    row->scenarios.string_method = method;
    return satellite004::success;
}
