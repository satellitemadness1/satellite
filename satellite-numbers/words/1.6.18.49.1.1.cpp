// satellite-numbers/words/1.6.18.49.1.1.cpp -- satellite.variable.window.widgets.display.file(name), built into satl
//
// satellite.variable.window.widgets.display.file(name)   1 6 18 49 1 1
// A file display -- the files of a directory -- made in the window or the widget the chain began on and
// reached after as window.a_name: its name in quotes, or its name, a width and a height (2026-10-09, sized
// 2026-10-10). ONE WORD OF THREE PARTS, `.satellite.` once before it (the author's final syntax, 2026-10-10
// ~21:15), and the family is display, singular (his rename of that evening).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `main_window.files_display.satellite.widgets.display.file("list_of_files", 100%,
// 100%)` reaches the function below through the function table -- bytecode/window_methods.cpp's run_window_number,
// which has refused a program's own thread first -- and the code in it is what call_window_method
// (bytecode/window_methods.cpp) ran until that day, moved here whole. A satl built without a window compiles none
// of it, and every number's function there is window_method_parts::no_window (window_readers.hpp).

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
    const std::string what = name + ".satellite.widgets.display.file";
    satellite_window *window = ready_to_do(token::widgets_display_file_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    // THE AUTHOR'S WIDGETS (2026-10-09) AND SECTIONS (2026-10-10): made in this window or widget, and
    // ANSWERED, so a chain goes on with the piece made (window_method_parts::a_piece_made says the rest).
    return a_piece_made(satellite_window::file_display, *window, arguments, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_49_1_1(LibraryRow *row)
{
    row->name = "satellite.variable.window.widgets.display.file(name)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 49;
    row->numbers[4] = 1;
    row->numbers[5] = 1;
    row->depth = 6;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
