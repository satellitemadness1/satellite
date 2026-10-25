// satellite-numbers/words/1.6.18.54.2.1.cpp -- satellite.variable.window.window.display.file(name), built into satl
//
// satellite.variable.window.window.display.file(name)   1 6 18 54 2 1
// A file display -- the files of a folder, folders first, starting in the home folder (2026-10-09) -- that fills the
// section it is made in, or goes across the top of a window with no sections or of a file display.
//
// WRITTEN .satellite.window.display.file SINCE SEC-0 (SATELLITE_WINDOW_SECTIONS.md, the author's design of
// 2026-10-20: `section_name_here.satellite.window.display.file("my_file_list_name")`). It TAKES ITS NAME AND NO SIZE
// (the author, 2026-10-25: "a display always fills the section"), and its name is a name, declared in the capsule
// that wrote it (rules 4 and 6; window_method_parts::a_piece_made declares it). The code below is 1.6.18.49.1.1.cpp's,
// the number `.satellite.widgets.display.file` ran from NM-8 (2026-10-11) until this one took it over: that spelling
// is refused now before anything runs, naming this one (rule 8), so 1 6 18 49 1 1 runs nothing.
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
    const std::string what = name + ".satellite.window.display.file";
    // A DISPLAY GIVEN A SIZE, as `.file("files", 100%, 100%)` was until SEC-0, is told that the size is a section's
    // (rule 8) -- before the run by the checker, and here where the checker could not see the line.
    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood, a_display_takes_no_size(token::window_display_file_token, what, "", ""));
        return Value();
    }
    satellite_window *window = ready_to_do(token::window_display_file_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    // MADE IN THIS WINDOW, SECTION OR FILE DISPLAY, NAMED, AND ANSWERED, so a chain goes on with the display made
    // (window_method_parts::a_piece_made says the rest).
    return a_piece_made(satellite_window::file_display, *window, arguments, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_54_2_1(LibraryRow *row)
{
    row->name = "satellite.variable.window.window.display.file(name)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 54;
    row->numbers[4] = 2;
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
