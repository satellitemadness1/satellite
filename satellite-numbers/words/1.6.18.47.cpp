// satellite-numbers/words/1.6.18.47.cpp -- satellite.variable.window.rows, built into satl
//
// satellite.variable.window.rows   1 6 18 47
// How many lines fit down a console right now; a question (GTK-17).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `console.satellite.rows` reaches the function below through the function table --
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
    const std::string what = name + ".satellite.rows";
    // `.satellite.columns` AND `.satellite.rows` ARE QUESTIONS (GTK-17), read with or without their
    // brackets as `.width` and `.height` are: how many characters fit across a
    // console and how many lines fit down it, right now, asked of the terminal.
    satellite_window *piece = which.get();
    if (piece == nullptr) {
        context.refuse(window_is_closed, what + ": there is no piece here");
        return Value();
    }
    long long int got = 0;
    std::string why;
    if (!console_cells_of(*piece, true, got, why)) {
        context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                       what + " -- " + why);
        return Value();
    }
    return Value::of_number(satellite_number(static_cast<unsigned long long int>(got)));
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_47(LibraryRow *row)
{
    row->name = "satellite.variable.window.rows";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 47;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
