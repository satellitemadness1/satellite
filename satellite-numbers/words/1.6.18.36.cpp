// satellite-numbers/words/1.6.18.36.cpp -- satellite.variable.window.arc(across, down, radius, from_degrees, to_degrees), built into satl
//
// satellite.variable.window.arc(across, down, radius, from_degrees, to_degrees)   1 6 18 36
// Part of a circle on a canvas, clockwise from three o'clock: filled it is a slice, outlined an arc
// (2026-09-22).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `c.satellite.arc(50, 50, 20, 0, 90)` reaches the function below through the function
// table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first -- and
// the code in it is what call_window_method (bytecode/window_methods.cpp) ran until that day, moved here whole. A
// satl built without a window compiles none of it, and every number's function there is
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
    const std::string what = name + ".satellite.arc";
    satellite_window *window = ready_to_do(token::arc_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    // WHAT A PROGRAM DRAWS ON A CANVAS (GTK-15). Every number is a place -- a
    // whole number that may be negative and must fit a screen -- read by the
    // one reader every place here goes through; what may NOT be negative is
    // the canvas's own business and is refused there, by name.
    long long int x = 0, y = 0, radius = 0, from = 0, to = 0;
    if (!place_of(arguments[0], x, what + "'s across", context) ||
        !place_of(arguments[1], y, what + "'s down", context) ||
        !place_of(arguments[2], radius, what + "'s radius", context) ||
        !place_of(arguments[3], from, what + "'s from", context, "a number of degrees") ||
        !place_of(arguments[4], to, what + "'s to", context, "a number of degrees"))
        return Value();
    went = window_arc(*window, x, y, radius, from, to, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_36(LibraryRow *row)
{
    row->name = "satellite.variable.window.arc(across, down, radius, from_degrees, to_degrees)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 36;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
