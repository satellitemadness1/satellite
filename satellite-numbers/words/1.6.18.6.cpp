// satellite-numbers/words/1.6.18.6.cpp -- satellite.variable.window.pressed(capsule), built into satl
//
// satellite.variable.window.pressed(capsule)   1 6 18 6
// The capsule a button runs when it is pressed, named as it is written -- and b.satellite.pressed, bare, reads
// that capsule's name back (WIN-11).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `b.satellite.pressed(when_pressed)` reaches the function below through the function
// table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first -- and
// the code in it is what answer_a_question (bytecode/window_questions.cpp) ran for its bare read and
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
    const std::string what = name + ".satellite.pressed";
    // `.satellite.pressed` WITH NO BRACKETS READS THE CAPSULE'S NAME BACK, and with
    // brackets says what it is -- the same pair as `.title`, and for the author's
    // same reason: a person who can set a thing expects to be able to ask it.
    // Empty for a button that answers nobody, which is what a button is until
    // something says otherwise.
    //
    // READ HERE ON THE INTERPRETER'S THREAD while the desk may be reading it in
    // `clicked`, and that is safe because both are READS: the only writer is
    // window_pressed(), which is this same thread going through on_the_desk().
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        std::string named;
        if (piece != nullptr)
            named = piece->when_pressed;
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(named, out, bad_offset);
        return out;
    }
    satellite_window *window = ready_to_do(token::pressed_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    std::string capsule;
    if (!text_of(arguments[0], capsule, what, context))
        return Value();
    went = window_pressed(*window, capsule, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_6(LibraryRow *row)
{
    row->name = "satellite.variable.window.pressed(capsule)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 6;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
