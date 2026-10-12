// satellite-numbers/words/1.6.18.48.2.cpp -- satellite.variable.window.scroll.horizontal(on), built into satl
//
// satellite.variable.window.scroll.horizontal(on)   1 6 18 48 2
// Whether a window or a console scrolls sideways -- a console with it on is 2048 characters wide and wraps
// nothing: written with brackets, read bare for the answer (2026-10-07). One word of two parts, as
// scroll.vertical is.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `w.satellite.scroll.horizontal(false)` reaches the function below through the
// function table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first
// -- and the code in it is what answer_a_question (bytecode/window_questions.cpp) ran for its bare read and
// call_window_method (bytecode/window_methods.cpp) for its write until that day, moved here whole. A satl built
// without a window compiles none of it, and every number's function there is window_method_parts::no_window
// (window_readers.hpp).

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
    const std::string what = name + ".satellite.scroll.horizontal";
    // `.satellite.scroll.vertical` AND `.satellite.scroll.horizontal` READ BARE ANSWER THE SWITCH -- what was
    // set on this window, or the arguments' row where nothing was.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no window here");
            return Value();
        }
        return Value::of_bool(window_scrolls(*piece, false));
    }
    satellite_window *window = ready_to_do(token::scroll_horizontal_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    bool on = false;
    if (!on_of(arguments[0], on, what, context))
        return Value();
    went = window_set_scrolling(*window, false, on, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_48_2(LibraryRow *row)
{
    row->name = "satellite.variable.window.scroll.horizontal(on)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 48;
    row->numbers[4] = 2;
    row->depth = 5;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
