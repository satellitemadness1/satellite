// satellite-numbers/words/1.6.18.1.cpp -- satellite.variable.window.append(piece, across, down), built into satl
//
// satellite.variable.window.append(piece, across, down)   1 6 18 1
// A piece put in the window, a row, a column, a grid, a scroll, a split or a set of tabs: the piece alone into
// a row or a column, and the piece and where its centre goes into a window or a grid's cell -- 1 argument or 3
// (GTK-7). It answers the receiver, so a chain goes on.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (NM-8; the author, 2026-10-10: "we have to give everything a number and then
// make it run those numbers"): `w.satellite.append(b, 400, 300)` reaches the function below through the function
// table -- bytecode/window_methods.cpp's run_window_number, which has refused a program's own thread first -- and
// the code in it is what call_window_method (bytecode/window_methods.cpp) ran until that day, moved here whole. A
// satl built without a window compiles none of it, and every number's function there is
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
    const std::string what = name + ".satellite.append";
    satellite_window *window = ready_to_do(token::append_token, which, arguments, had_parentheses, what, context);
    if (window == nullptr)
        return Value();
    std::string why;
    bool went = false;
    const WindowHandle *piece = arguments[0].window_handle();
    if (piece == nullptr) {
        context.refuse(types_do_not_meet, what + " takes a piece to put in the window -- " +
                                              the_words_that_make_a_piece() + " makes one -- and was "
                                              "given " + arguments[0].kind_name());
        return Value();
    }
    // ONE ARGUMENT OR THREE, and which is right is decided by what the
    // RECEIVER turned out to be -- window_append() is what knows.
    const bool by_place = arguments.size() == 3;
    long long int x = 0, y = 0;
    if (by_place && (!place_of(arguments[1], x, what + "'s across", context) ||
                     !place_of(arguments[2], y, what + "'s down", context)))
        return Value();
    went = window_append(*window, *piece, by_place, x, y, why);
    // A PIECE THAT CLOSED WITH ITS WINDOW GETS THE CODE THAT SAYS CLOSED.
    // The tail below reads the RECEIVER, which is open, and would print
    // types_do_not_meet under a sentence about closing -- the code and the
    // sentence disagreeing, which `.save` was fixed for too. TOLD
    // APART BY THE SENTENCE, as `.save` is, and not by the piece being
    // closed: a closed piece handed to the wrong shape of `.append` is
    // refused for the shape first, and that is a kind that does not meet
    // (a fresh reader, 2026-10-03).
    if (!went && why.find("closed with the window it was in") != std::string::npos) {
        context.refuse(window_is_closed, what + " could not be done -- " + why);
        return Value();
    }
    return done(went, why, *window, which, what, context);
}

} // namespace

#endif

namespace satellite004::built_in {

signed long long int describe_1_6_18_1(LibraryRow *row)
{
    row->name = "satellite.variable.window.append(piece, across, down)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 18;
    row->numbers[3] = 1;
    row->depth = 4;
#if SATELLITE_HAS_WINDOW
    row->scenarios.method_call = method;
#else
    row->scenarios.method_call = window_method_parts::no_window;
#endif
    return satellite004::success;
}

} // namespace satellite004::built_in
