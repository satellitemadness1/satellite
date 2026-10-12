// satellite-numbers/words/1.6.18.40.cpp -- satellite.variable.window.down, built into satl
//
// satellite.variable.window.down   1 6 18 40
// How far down a piece the last click on it landed, in pixels from its top edge, and 0 until one has; a
// question (GTK-15).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `c.satellite.down` reaches the function below through the function table --
// bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first -- and the code
// in it is what answer_a_question (bytecode/window_questions.cpp) ran until that day, moved here whole. A satl
// built without a window compiles none of it, and every number's function there is window_method_parts::no_window
// (window_readers.hpp).

#include "../number_row.hpp"
#include "../../satellite/bytecode/window_readers.hpp"
#include "../../satellite/machine/machine_codes.hpp"

#if SATELLITE_HAS_WINDOW

namespace {

using namespace satellite004;
using namespace satellite004::window_method_parts;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &,
             bool, const std::string &name, ExpressionContext &context)
{
    const WindowHandle &which = *receiver.window_handle();
    const std::string what = name + ".satellite.down";
    // `.satellite.across` AND `.satellite.down` ARE QUESTIONS TOO (GTK-15's leftover, 2026-09-22):
    // where the last click on this piece landed, in its own pixels, which on a
    // canvas are the pixels `.line` draws in. 0 and 0 until a click has
    // happened, as `.key` is "" until a key has. THE HANDLE'S OWN, written by
    // THIS thread off the event as `.key` is, so no desk is asked and a closed
    // piece still answers where it was last clicked.
    //
    // A BUTTON AND A MENU ARE REFUSED BY NAME (a fresh reader, 2026-09-22):
    // neither can ever be `.clicked` -- a button is pressed, a menu's items
    // are picked -- so 0 from them would not be "not yet", it would be never,
    // which is the answer that is wrong and does not say so. Everything else
    // answers 0 until a click has landed, which is only noticed on a piece
    // told `.clicked(a_capsule)`; and a press RELEASED off the piece lands
    // outside it -- negative, or past `.width` -- which is the truth of where
    // it was let go.
    satellite_window *piece = which.get();
    if (piece == nullptr) {
        context.refuse(window_is_closed, what + ": there is no piece here");
        return Value();
    }
    if (piece->piece == satellite_window::button || !piece->is_drawn()) {
        context.refuse(types_do_not_meet,
                       what + " -- " + std::string(piece->piece_name()) +
                           (piece->piece == satellite_window::button
                                ? " is pressed and never clicked, so no click lands on it"
                                : " is not clicked, its items are picked") +
                           "; .satellite.across and .satellite.down are where the last click landed on a piece "
                           "told .satellite.clicked(a_capsule)");
        return Value();
    }
    const long long int got = piece->last_down;
    satellite_number answer(got < 0 ? static_cast<unsigned long long int>(-got)
                                    : static_cast<unsigned long long int>(got),
                            got < 0);
    return Value::of_number(std::move(answer));
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_40(LibraryRow *row)
{
    row->name = "satellite.variable.window.down";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 40;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
