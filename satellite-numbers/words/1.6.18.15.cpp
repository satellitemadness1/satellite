// satellite-numbers/words/1.6.18.15.cpp -- satellite.variable.window.resize(wide, tall), built into satl
//
// satellite.variable.window.resize(wide, tall)   1 6 18 15
// The window's size now, a width and a height in pixels (GTK-8). The same token is an infinity's .resize(n),
// which is not built (INF-7): one token, a number a kind.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `w.satellite.resize(1024, 768)` reaches the function below through the function
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
    const std::string what = name + ".satellite.resize";
    satellite_window *window = ready_to_do(token::resize_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    long long int wide = 0, tall = 0;
    if (!place_of(arguments[0], wide, what + "'s width", context) ||
        !place_of(arguments[1], tall, what + "'s height", context))
        return Value();
    went = window_resize(*window, wide, tall, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_15(LibraryRow *row)
{
    row->name = "satellite.variable.window.resize(wide, tall)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 15;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
