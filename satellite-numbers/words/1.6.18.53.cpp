// satellite-numbers/words/1.6.18.53.cpp -- satellite.variable.window.ok, built into satl
//
// satellite.variable.window.ok   1 6 18 53
// Whether a window is still on the screen; a question, read bare or with (). The same token is a file's
// .ok, a program's .ok() and an object's: a number a kind.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `w.satellite.ok` reaches the function below through the function table --
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
             bool, const std::string &, ExpressionContext &)
{
    const WindowHandle &which = *receiver.window_handle();
    // `.satellite.ok` READS WITH OR WITHOUT ITS BRACKETS, because it is a QUESTION and not a
    // doing -- `f.ok` is how a file is asked the same thing (SATELLITE_FILE_OPERATIONS).
    satellite_window *asked = which.get();
    return Value::of_bool(asked != nullptr && asked->on_the_screen);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_53(LibraryRow *row)
{
    row->name = "satellite.variable.window.ok";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 53;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
