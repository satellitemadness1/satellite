// satellite-numbers/words/1.6.1.12.cpp -- satellite.variable.string.replace(a, b), built into satl
//
// satellite.variable.string.replace(a, b)   1 6 1 12
// Every appearance of a swapped for b, left to right, never rescanning what was written in (so replace("a", "aa") ends). 003 refuses an empty a.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_12, is the one
// word_table.cpp calls for 1 6 1 12 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &arguments, StringAnswer &answer)
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

namespace satellite004::built_in {

signed long long int describe_1_6_1_12(LibraryRow *row)
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

} // namespace satellite004::built_in
