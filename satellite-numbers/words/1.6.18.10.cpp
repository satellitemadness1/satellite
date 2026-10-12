// satellite-numbers/words/1.6.18.10.cpp -- satellite.variable.window.value(x), built into satl
//
// satellite.variable.window.value(x)   1 6 18 10
// The number a slider or a number box is at, or the percentage a progress bar shows: written with brackets,
// read bare -- a number from a slider, a percentage from a bar (GTK-4).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `s.satellite.value(5)` reaches the function below through the function table --
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
    const std::string what = name + ".satellite.value";
    // `.satellite.value` WITH NO BRACKETS ASKS WHAT NUMBER A PIECE IS AT, and what comes
    // back depends on the piece: a slider and a number box answer a NUMBER, and
    // a progress bar answers a PERCENTAGE. That is not two methods wearing one
    // name -- it is one question whose answer has the kind the piece has.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        long long int got = 0;
        std::string why;
        if (!window_value_of(*piece, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        if (piece->piece == satellite_window::progress)
            return a_percentage_of(got);
        satellite_number answer(got < 0 ? static_cast<unsigned long long int>(-got)
                                        : static_cast<unsigned long long int>(got),
                                got < 0);
        return Value::of_number(std::move(answer));
    }
    satellite_window *window = ready_to_do(token::value_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    // THE KIND IT TAKES IS THE PIECE'S, and the wrong one is REFUSED rather
    // than converted. A bare 50 on a progress bar could mean 50% or it could
    // mean half of one -- and a guess between those two is an answer that is
    // wrong and does not say so, so the refusal names the spelling instead.
    long long int to = 0;
    if (window->piece == satellite_window::progress) {
        const satellite_percentage *asked = arguments[0].as_percentage();
        if (asked == nullptr) {
            context.refuse(types_do_not_meet,
                           what + " takes a percentage, written with its sign on: " + name +
                               ".satellite.value(50%) -- and was given " + arguments[0].kind_name());
            return Value();
        }
        if (!millionths_of(*asked, to)) {
            context.refuse(not_a_position,
                           what + " was given a percentage no bar can be set to");
            return Value();
        }
    } else if (!place_of(arguments[0], to, what, context, "a whole number")) {
        return Value();
    }
    went = window_set_value(*window, to, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_10(LibraryRow *row)
{
    row->name = "satellite.variable.window.value(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 10;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
