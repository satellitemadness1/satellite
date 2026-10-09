// satellite-numbers/words/1.6.1.11.cpp -- satellite.variable.string.trim, built into satl
//
// satellite.variable.string.trim   1 6 1 11
// The text without space, tab, carriage return or newline at either end, as in 003.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_11, is the one
// word_table.cpp calls for 1 6 1 11 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
// src/satellite_scalars/string_methods.cpp; written 2026-09-14.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

signed long long int method(satellite_string32 &self, const StringArguments &/* arguments */, StringAnswer &answer)
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

namespace satellite004::built_in {

signed long long int describe_1_6_1_11(LibraryRow *row)
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

} // namespace satellite004::built_in
