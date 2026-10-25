// satellite-numbers/words/1.6.18.54.1.cpp -- satellite.variable.window.window.section(name, width, height), built into satl
//
// satellite.variable.window.window.section(name, width, height)   1 6 18 54 1
// A section of a window or of a section, as wide and as tall as a share of what holds it or as pixels, laid out
// left to right (the author, 2026-10-10: "to program GUIs in satellite you must learn the section code scheme").
//
// WRITTEN .satellite.window.section SINCE SEC-0 (SATELLITE_WINDOW_SECTIONS.md, the author's design of 2026-10-20:
// `main_window.satellite.window.section("section_name_here", fifty_percent, 100%)`), and ITS NAME IS A NAME: the line
// declares it in the capsule that wrote it, so the lines after it write it bare -- section_name_here.satellite... --
// as well as main_window.section_name_here (rule 4; window_method_parts::a_piece_made declares it). The code below is
// 1.6.18.52.cpp's, the number `.satellite.section` ran from NM-8 (2026-10-11) until this one took it over: that
// spelling is refused now before anything runs, naming this one (rule 8), so 1 6 18 52 runs nothing.
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
    const std::string what = name + ".satellite.window.section";
    satellite_window *window = ready_to_do(token::window_section_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    // MADE IN THIS WINDOW OR SECTION, NAMED, AND ANSWERED, so a chain goes on with the section made
    // (window_method_parts::a_piece_made says the rest).
    return a_piece_made(satellite_window::a_section, *window, arguments, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_54_1(LibraryRow *row)
{
    row->name = "satellite.variable.window.window.section(name, width, height)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 54;
    row->numbers[4] = 1;
    row->depth = 5;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
