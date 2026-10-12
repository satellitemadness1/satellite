// satellite-numbers/words/1.6.18.9.cpp -- satellite.variable.window.on(on), built into satl
//
// satellite.variable.window.on(on)   1 6 18 9
// Whether a checkbox or a switch is turned on: c.satellite.on(1) turns it on and (0) off, and c.satellite.on,
// bare, asks the desk (GTK-3).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `c.satellite.on(1)` reaches the function below through the function table --
// bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first -- and the code
// in it is what answer_a_question (bytecode/window_questions.cpp) ran for its bare read and call_window_method
// (bytecode/window_methods.cpp) for its write until that day, moved here whole. A satl built without a window
// compiles none of it, and every number's function there is window_method_parts::no_window (window_readers.hpp).

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
    const std::string what = name + ".satellite.on";
    // `.satellite.on` WITH NO BRACKETS ASKS WHETHER A THING IS TURNED ON, and it is the
    // same shape as `.text`: a person clicking a checkbox changes the widget and
    // tells satellite nothing, so the answer is the widget's and not the
    // handle's. A piece that is neither on nor off is REFUSED rather than
    // answered false -- a label has no such question.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        bool on = false;
        std::string why;
        if (!window_on_of(*piece, on, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        return Value::of_bool(on);
    }
    satellite_window *window = ready_to_do(token::on_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    bool on = false;
    if (!on_of(arguments[0], on, what, context))
        return Value();
    went = window_set_on(*window, on, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_9(LibraryRow *row)
{
    row->name = "satellite.variable.window.on(on)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 9;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
