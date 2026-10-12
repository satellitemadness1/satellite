// satellite-numbers/words/1.6.18.49.1.2.cpp -- satellite.variable.window.widgets.display.input(name), built into satl
//
// satellite.variable.window.widgets.display.input(name)   1 6 18 49 1 2
// A one-line text input, made and reached the same way as a file display: its name, or its name and a size
// (2026-10-09). One word of three parts, as widgets.display.file is.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `main_window.satellite.widgets.display.input("folder")` reaches the function below
// through the function table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own
// thread first -- and the code in it is what call_window_method (bytecode/window_methods.cpp) ran until that day,
// moved here whole. A satl built without a window compiles none of it, and every number's function there is
// window_method_parts::no_window (window_readers.hpp).

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
    const std::string what = name + ".satellite.widgets.display.input";
    satellite_window *window = ready_to_do(token::widgets_display_input_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    // THE AUTHOR'S WIDGETS (2026-10-09) AND SECTIONS (2026-10-10): made in this window or widget, and
    // ANSWERED, so a chain goes on with the piece made (window_method_parts::a_piece_made says the rest).
    return a_piece_made(satellite_window::text_box, *window, arguments, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_49_1_2(LibraryRow *row)
{
    row->name = "satellite.variable.window.widgets.display.input(name)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 49;
    row->numbers[4] = 1;
    row->numbers[5] = 2;
    row->depth = 6;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
