// satellite-numbers/words/1.6.18.51.cpp -- satellite.variable.window.listen(capsule), built into satl
//
// satellite.variable.window.listen(capsule)   1 6 18 51
// A text input keeps its text up to date -- and, given a capsule, runs it on every key typed in it: 0
// arguments or 1, a doing written with its brackets (the author, 2026-10-09).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `folder.satellite.listen(when_typed)` reaches the function below through the
// function table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first
// -- and the code in it is what call_window_method (bytecode/window_methods.cpp) ran until that day, moved here
// whole. A satl built without a window compiles none of it, and every number's function there is
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
    const std::string what = name + ".satellite.listen";
    satellite_window *window = ready_to_do(token::listen_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    // `.satellite.listen()` AND `.satellite.listen(a_capsule)` -- the author, 2026-10-09: "a combination of A,
    // it runs a capsule if you give it a capsule, and C, it keeps the string text up to date". A doing, so it
    // has its brackets, as `.close()` has.
    if (!had_parentheses) {
        context.refuse(satl_line_not_understood, what + " is something an input DOES, so write it with its "
                                                        "brackets: " + what + "() or " + what + "(a_capsule)");
        return Value();
    }
    std::string capsule;
    if (!arguments.empty() && !text_of(arguments[0], capsule, what, context))
        return Value();
    went = window_listen(*window, capsule, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_51(LibraryRow *row)
{
    row->name = "satellite.variable.window.listen(capsule)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 51;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
