// satellite-numbers/words/1.6.18.54.2.2.1.cpp -- satellite.variable.window.window.display.text.input(name), built into satl
//
// satellite.variable.window.window.display.text.input(name)   1 6 18 54 2 2 1
// A one-line text input, made, placed and named as a file display is (2026-10-09): it fills its section, or goes
// across the top of a window with no sections or of a file display, starting with that display's folder.
//
// WRITTEN .satellite.window.display.text.input SINCE SEC-0 (SATELLITE_WINDOW_SECTIONS.md rule 6: "today's
// widgets.display.input, under its new name, keeping its name in quotes"; the author, 2026-10-25: every display takes
// its name first). It takes its name and no size, and its name is a name, declared in the capsule that wrote it
// (rule 4; window_method_parts::a_piece_made declares it). The code below is 1.6.18.49.1.2.cpp's, the number
// `.satellite.widgets.display.input` ran from NM-8 (2026-10-11) until this one took it over: that spelling is refused
// now before anything runs, naming this one (rule 8), so 1 6 18 49 1 2 runs nothing. window.display.text, 1 6 18 54
// 2 2, runs nothing until SEC-3 makes it the text display.
//
// It reaches the function below through the function table -- bytecode/window_methods.cpp's run_window_number, which
// has refused a program's own thread first. A satl built without a window compiles none of it, and every number's
// function there is window_method_parts::no_window (window_readers.hpp).

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
    const std::string what = name + ".satellite.window.display.text.input";
    // A DISPLAY GIVEN A SIZE is told the size is a section's (rule 8), as in 1.6.18.54.2.1.cpp.
    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood,
                       a_display_takes_no_size(token::window_display_text_input_token, what, "", ""));
        return Value();
    }
    satellite_window *window =
        ready_to_do(token::window_display_text_input_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    // MADE IN THIS WINDOW, SECTION OR FILE DISPLAY, NAMED, AND ANSWERED, so a chain goes on with the input made
    // (window_method_parts::a_piece_made says the rest).
    return a_piece_made(satellite_window::text_box, *window, arguments, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_54_2_2_1(LibraryRow *row)
{
    row->name = "satellite.variable.window.window.display.text.input(name)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 54;
    row->numbers[4] = 2;
    row->numbers[5] = 2;
    row->numbers[6] = 1;
    row->depth = 7;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
