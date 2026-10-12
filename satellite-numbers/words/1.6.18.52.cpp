// satellite-numbers/words/1.6.18.52.cpp -- satellite.variable.window.section(name, width, height), built into satl
//
// satellite.variable.window.section(name, width, height)   1 6 18 52
// A section of a window or of a section, as wide and as tall as a share of what holds it or as pixels, laid
// out left to right and reached after as window.a_name (the author, 2026-10-10: "to program GUIs in satellite
// you must learn the section code scheme").
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `main_window.satellite.section("files_display", 100%, 250)` reaches the function
// below through the function table -- bytecode/window_methods.cpp's run_window_number, which has refused a
// program's own thread first -- and the code in it is what call_window_method (bytecode/window_methods.cpp) ran
// until that day, moved here whole. A satl built without a window compiles none of it, and every number's function
// there is window_method_parts::no_window (window_readers.hpp).

#include "../number_row.hpp"
#include "../../satellite/bytecode/window_readers.hpp"
#include "../../satellite/machine/machine_codes.hpp"

#if SATELLITE_HAS_WINDOW

namespace {

using namespace satellite004;
using namespace satellite004::window_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments,
             bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const WindowHandle &which = *receiver.window_handle();
    const std::string what = name + ".satellite.section";
    satellite_window *window = ready_to_do(token::section_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    // THE AUTHOR'S WIDGETS (2026-10-09) AND SECTIONS (2026-10-10): made in this window or widget, and
    // ANSWERED, so a chain goes on with the piece made (window_method_parts::a_piece_made says the rest).
    return a_piece_made(satellite_window::a_section, *window, arguments, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_52(LibraryRow *row)
{
    row->name = "satellite.variable.window.section(name, width, height)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 52;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
