// satellite-numbers/words/1.6.18.11.cpp -- satellite.variable.window.chosen(item), built into satl
//
// satellite.variable.window.chosen(item)   1 6 18 11
// Which item of a choice or a one-of is picked, as TEXT, and which piece of a set of tabs is in front: written
// with brackets, read bare (GTK-5).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `c.satellite.chosen("green")` reaches the function below through the function table
// -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first -- and the
// code in it is what answer_a_question (bytecode/window_questions.cpp) ran for its bare read and call_window_method
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
    const std::string what = name + ".satellite.chosen";
    // `.satellite.chosen` WITH NO BRACKETS ASKS WHICH ITEM IS PICKED, as TEXT. Nothing
    // picked is "" and not a refusal: a choice a person has not touched is an
    // ordinary state of a choice.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        std::string picked, why;
        if (!window_chosen_of(*piece, picked, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(picked, out, bad_offset);
        return out;
    }
    satellite_window *window = ready_to_do(token::chosen_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    std::string pick;
    if (!text_of(arguments[0], pick, what, context))
        return Value();
    went = window_set_chosen(*window, pick, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_11(LibraryRow *row)
{
    row->name = "satellite.variable.window.chosen(item)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 11;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
