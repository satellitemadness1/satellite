// satellite-numbers/words/1.6.18.50.cpp -- satellite.variable.window.directory(path), built into satl
//
// satellite.variable.window.directory(path)   1 6 18 50
// Which directory a file display shows -- written with brackets, read bare for the one it shows. A directory
// that is not there is no refusal: the display stays, and the program runs (2026-10-09).
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `files.satellite.directory("/home/you/")` reaches the function below through the
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
    const std::string what = name + ".satellite.directory";
    // `.satellite.directory` READ BARE IS THE ONE A FILE DISPLAY SHOWS, the interpreter's own copy -- written
    // only after the desk showed it, so no crossing is needed to ask.
    if (!had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr || piece->piece != satellite_window::file_display) {
            context.refuse(types_do_not_meet, what + " -- " +
                                                  std::string(piece != nullptr ? piece->piece_name() : "nothing") +
                                                  " shows no directory; a file display does: "
                                                  "my_section.satellite.window.display.file(\"files\")");
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece->directory, out, bad_offset);
        return out;
    }
    satellite_window *window = ready_to_do(token::directory_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    // A DIRECTORY THAT IS NOT THERE IS NO REFUSAL (his "yes"): the display stays, and the program runs.
    std::string path;
    if (!text_of(arguments[0], path, what, context))
        return Value();
    bool shown = false;
    went = window_set_directory(*window, path, shown, why);
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_50(LibraryRow *row)
{
    row->name = "satellite.variable.window.directory(path)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 50;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
