// satellite-numbers/words/1.6.18.8.cpp -- satellite.variable.window.text(words), built into satl
//
// satellite.variable.window.text(words)   1 6 18 8
// The words on a piece -- and box.satellite.text, bare, asks the desk for what is there now. A window is
// refused (its words are its .satellite.title), and so are a picture and a section (GTK-1).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `box.satellite.text("x")` reaches the function below through the function table --
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
    const std::string what = name + ".satellite.text";
    // `.satellite.text` WITH NO BRACKETS READS THE WORDS ON A PIECE, and since GTK-2 it is
    // a real ASK rather than a look at the handle: a person typing in a text box
    // changes the widget and tells satellite nothing, so window_text_of() goes
    // to the desk, copies what is there and answers that.
    //
    // AND IT CAN REFUSE, which no other bare read here can: a WINDOW has no words
    // -- its words are its title -- and being handed an empty string for one
    // would be an answer that is wrong and does not say so.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        std::string words, why;
        if (!window_text_of(*piece, words, why)) {
            // TWO REFUSALS AND TWO CODES, because they are two different
            // mistakes. A WINDOW asked for `.text` is a kind that does not meet
            // -- the program wanted `.title`. A CLOSED text box is not a mistake
            // at all until it happens: the piece was right and the moment was
            // late, and window_is_closed is the code that says so. Sending both
            // through types_do_not_meet printed "this operator has no scenario
            // for the two kinds it was given" under a sentence about a window
            // that had closed, which is an explanation that does not fit.
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(words, out, bad_offset);
        return out;
    }
    satellite_window *window = ready_to_do(token::text_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    std::string text;
    if (!text_of(arguments[0], text, what, context))
        return Value();
    went = window_set_text(*window, text, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_8(LibraryRow *row)
{
    row->name = "satellite.variable.window.text(words)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 8;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
