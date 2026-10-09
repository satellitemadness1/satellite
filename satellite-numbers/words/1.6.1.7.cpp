// satellite-numbers/words/1.6.1.7.cpp -- satellite.variable.string.ends_with(x), built into satl
//
// satellite.variable.string.ends_with(x)   1 6 1 7
// True when the string finishes with the text.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_6_1_7, is the one
// word_table.cpp calls for 1 6 1 7 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. Behaviour ported from 003 06's
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

namespace satellite004::built_in {

signed long long int describe_1_6_1_7(LibraryRow *row)
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

} // namespace satellite004::built_in
