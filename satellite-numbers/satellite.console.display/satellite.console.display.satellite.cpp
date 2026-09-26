// satellite.console.display  `1 5 1` -- its likely scenarios, compiled as a
// library with no main.
//
// SINCE 2026-09-26 A DISPLAY DOES NOT WRITE THROUGH HERE. The printing satellite makes every
// line -- a plain display's from the value the interpreter moves to it, a styled one from the
// bytes the interpreter makes -- and a display thread writes it (satellite/display/,
// SCRATCH.md/FAST_PRINTING.md step 3). These scenarios still describe the word, and
// call_word still asks that the word has a library; the bytes they wrote are the bytes the
// printing satellite writes now, which check.sh holds it to.
//
// Each scenario answers a machine code: 0 when the value reached the output,
// display_error when the output refused it.
//
// '\n' AND NOT std::endl: endl also flushes, which makes one write() per line
// -- the reason satl's display made 699,262 write() calls for 1,000,000 lines
// (measured 2026-09-14). The catch is that std::cout reports a refused write
// when it actually writes, up to ~4 KB later; the interpreter checks again
// after its final flush so the last few lines are not missed.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

#include <cstdio>
#include <iostream>
#include <string>

namespace {

using satellite004::display_error;
using satellite004::success;

// the most likely scenario: display a string
signed long long int satellite_console_display_most_likely(const std::string &input_str, bool display_endline)
{
    std::cout << input_str;
    if (display_endline == true)
        std::cout << '\n';
    if (!std::cout)
        return(display_error);
    return(success);
}

signed long long int satellite_console_display_count(unsigned long long int value, bool display_endline)
{
    std::cout << value;
    if (display_endline == true)
        std::cout << '\n';
    if (!std::cout)
        return(display_error);
    return(success);
}

signed long long int satellite_console_display_flag(bool value, bool display_endline)
{
    std::cout << (value ? "true" : "false");
    if (display_endline == true)
        std::cout << '\n';
    if (!std::cout)
        return(display_error);
    return(success);
}

signed long long int satellite_console_display_size(long double value, const std::string &unit, bool display_endline)
{
    char digits[64];
    std::snprintf(digits, sizeof digits, "%.18Lg", value);
    std::cout << digits << ' ' << unit;
    if (display_endline == true)
        std::cout << '\n';
    if (!std::cout)
        return(display_error);
    return(success);
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.console.display";
    row->numbers[0] = 1;
    row->numbers[1] = 5;
    row->numbers[2] = 1;
    row->depth = 3;
    row->scenarios.text = satellite_console_display_most_likely;
    row->scenarios.count = satellite_console_display_count;
    row->scenarios.flag = satellite_console_display_flag;
    row->scenarios.size = satellite_console_display_size;
    return(success);
}
