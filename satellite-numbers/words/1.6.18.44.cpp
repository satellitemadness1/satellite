// satellite-numbers/words/1.6.18.44.cpp -- satellite.variable.window.typed(capsule), built into satl
//
// satellite.variable.window.typed(capsule)   1 6 18 44
// The capsule to run when a person finishes a line in a console -- and console.satellite.typed, bare, the last
// line they finished (GTK-17).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `console.satellite.typed(when_typed)` reaches the function below through the
// function table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first
// -- and the code in it is what answer_a_question (bytecode/window_questions.cpp) ran for its bare read and
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
    const std::string what = name + ".satellite.typed";
    // `.satellite.typed` READ BARE IS THE LAST LINE A PERSON FINISHED IN A CONSOLE
    // (GTK-17), `.key`'s shape exactly: the desk read it off the pty, the
    // event carried it, and the interpreter wrote it onto the piece before
    // the capsule ran -- one writer. Empty until a line has been finished.
    // ONLY A CONSOLE has lines typed into it; anything else is refused by
    // name, because "" from a button would not be "not yet", it would be never.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece != nullptr && piece->piece != satellite_window::console) {
            context.refuse(types_do_not_meet, what + " -- " + std::string(piece->piece_name()) +
                                                  " is not a console, and only a console has lines "
                                                  "typed into it");
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece == nullptr ? std::string() : piece->last_typed, out, bad_offset);
        return out;
    }
    satellite_window *window = ready_to_do(token::typed_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    std::string capsule;
    if (!text_of(arguments[0], capsule, what, context))
        return Value();
    went = console_typed(*window, capsule, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_44(LibraryRow *row)
{
    row->name = "satellite.variable.window.typed(capsule)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 44;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
