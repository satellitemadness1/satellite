// satellite-numbers/words/1.6.18.27.cpp -- satellite.variable.window.choose_a_file(capsule), built into satl
//
// satellite.variable.window.choose_a_file(capsule)   1 6 18 27
// Asks a person for a file; the capsule runs with its path in w.satellite.answer, or "" if they dismissed it
// (GTK-11).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `w.satellite.choose_a_file(when_chosen)` reaches the function below through the
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
    const std::string what = name + ".satellite.choose_a_file";
    satellite_window *window = ready_to_do(token::choose_a_file_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    std::string capsule;
    if (!text_of(arguments[0], capsule, what, context))
        return Value();
    went = window_choose_a_file(*window, capsule, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_27(LibraryRow *row)
{
    row->name = "satellite.variable.window.choose_a_file(capsule)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 27;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
