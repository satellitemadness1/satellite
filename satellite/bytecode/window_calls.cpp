// satellite/bytecode/window_calls.cpp -- satellite.window's words and a window's
// own methods. The header says why they are the object model's and not
// libraries, and why this is the one file that knows whether satl has a window.

#include "window_calls.hpp"

#include "word_codes.hpp"
#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_variable_window/window_desk.hpp"

#include <string>
#include <utility>

// SET BY make_support/047-window.mk. 0 means pkg-config found no gtk4 when this
// satl was built, and the window sources were not compiled at all -- so every
// word here still LEXES, still CHECKS, and refuses at the moment it would draw.
#ifndef SATELLITE_HAS_WINDOW
#define SATELLITE_HAS_WINDOW 0
#endif

namespace satellite004 {
namespace {

using token::Code;
namespace fast = number_fast_path;

Code window_word() { return word::code_of(1, 27); }
Code window_new_word() { return word::code_of(1, 27, 1); }
Code window_button_word() { return word::code_of(1, 27, 2); }

// Text going in. A NUMBER WHERE TEXT IS EXPECTED IS ITS DIGITS, which is
// file_calls.cpp's rule and the author's: `satellite.window.new(5, 80, 24)` is
// a window titled "5" rather than a refusal about kinds.
bool text_of(const Value &value, std::string &out, const std::string &what, ExpressionContext &context)
{
    if (value.is_string()) { out = value.text_utf8(); return true; }
    if (const satellite_number *number = value.as_number()) { out = fast::to_text(*number); return true; }
    context.refuse(types_do_not_meet, what + " takes text, and was given " + value.kind_name());
    return false;
}

// A SIZE. Negative is refused where it is written, rather than reaching GTK as
// an enormous count -- file_calls.cpp learned that one the hard way, where
// `f.truncate(-1)` quietly did nothing.
bool size_of(const Value &value, unsigned long long int &out, const std::string &what,
             ExpressionContext &context)
{
    const satellite_number *number = value.as_number();
    if (number == nullptr) {
        context.refuse(types_do_not_meet, what + " takes a number of pixels, and was given " + value.kind_name());
        return false;
    }
    if (number->negative()) {
        context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                           ", and a window has no negative size");
        return false;
    }
    out = fast::fits_a_count(*number) ? fast::as_count(*number) : ~0ull;
    return true;
}

// A PLACE. Unlike a size, a NEGATIVE place is meaningful -- it is a centre off
// the left or the top of the window, and GtkFixed takes it.
bool place_of(const Value &value, long long int &out, const std::string &what, ExpressionContext &context)
{
    const satellite_number *number = value.as_number();
    if (number == nullptr) {
        context.refuse(types_do_not_meet, what + " takes a number of pixels, and was given " + value.kind_name());
        return false;
    }
    const unsigned long long int size = fast::fits_a_count(*number) ? fast::as_count(*number) : ~0ull;
    if (size > 2147483647ull) {
        context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                           ", which is further than any screen reaches");
        return false;
    }
    out = number->negative() ? -static_cast<long long int>(size) : static_cast<long long int>(size);
    return true;
}

#if SATELLITE_HAS_WINDOW == 0
// THE ONE SENTENCE THIS satl SAYS WHEN IT WAS BUILT WITHOUT A WINDOW. It names
// what to do about it, because "not built" with no way forward is the refusal a
// person can do nothing with.
Value no_window_here(const std::string &what, ExpressionContext &context)
{
    context.refuse(not_built_yet,
                   what + ": this satl was built without a window -- pkg-config found no gtk4 when it "
                          "was made. Install gtk4-devel (AlmaLinux/RHEL: dnf install gtk4-devel; "
                          "Debian/Ubuntu: apt install libgtk-4-dev) and build again");
    return Value();
}
#endif

} // namespace

bool is_window_word(Code code)
{
    return code == window_word() || code == window_new_word() || code == window_button_word();
}

std::size_t window_word_arity(Code code)
{
    if (code == window_new_word()) return 3;
    if (code == window_button_word()) return 1;
    return 0;
}

std::string window_word_takes(Code code)
{
    if (code == window_new_word())
        return "satellite.window.new takes a title, a width and a height: "
               "satellite.window.new(\"my window\", 800, 600)";
    if (code == window_button_word())
        return "satellite.window.button takes the text on it: satellite.window.button(\"press me\")";
    return "satellite.window is the family name and is not a call -- "
           "satellite.window.new(...) makes a window";
}

int window_method_arity(Code method)
{
    switch (method) {
    case token::append_token:  return 3;     // the piece, and where its centre goes
    case token::close_token:   return 0;
    case token::focus_token:   return 0;
    case token::title_token:   return 1;     // written; read with no brackets
    case token::pressed_token: return 1;     // the capsule's name; read with no brackets
    case token::press_token:   return 0;     // a click has nothing to say
    case token::ok_token:      return 0;
    default:                   return -1;
    }
}

bool window_method_takes_a_capsule_name(Code method) { return method == token::pressed_token; }


// ---------------------------------------------------------------------------
// THE WORDS AND THE METHODS. Everything below exists twice -- once for a satl
// with a window and once for one without -- and the two halves must answer the
// same shapes, which is why they sit in one file under one #if rather than in
// two files that could drift apart.
// ---------------------------------------------------------------------------
#if SATELLITE_HAS_WINDOW

Value call_window_word(Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    if (code == window_word()) {
        context.refuse(satl_line_not_understood, window_word_takes(code));
        return Value();
    }
    if (arguments.size() != window_word_arity(code)) {
        context.refuse(satl_line_not_understood, window_word_takes(code) + " -- it was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }

    std::string why;
    if (code == window_button_word()) {
        std::string text;
        if (!text_of(arguments[0], text, "satellite.window.button", context))
            return Value();
        WindowHandle made = window_button(text, why);
        if (made == nullptr) {
            context.refuse(no_display, "satellite.window.button could not be made -- " + why);
            return Value();
        }
        return Value::of_window(std::move(made));
    }

    std::string title;
    unsigned long long int wide = 0, tall = 0;
    if (!text_of(arguments[0], title, "satellite.window.new", context) ||
        !size_of(arguments[1], wide, "satellite.window.new's width", context) ||
        !size_of(arguments[2], tall, "satellite.window.new's height", context))
        return Value();
    WindowHandle made = window_new(title, wide, tall, why);
    if (made == nullptr) {
        // NO SCREEN IS THE MACHINE'S ANSWER AND A BAD SIZE IS THE PROGRAM'S, and
        // they are told apart by which one window_new checked first: it refuses a
        // size before it ever asks for a display.
        const bool the_program = wide == 0 || tall == 0 || wide > 32767 || tall > 32767;
        context.refuse(the_program ? satl_line_not_understood : no_display,
                       "satellite.window.new could not open a window -- " + why);
        return Value();
    }
    return Value::of_window(std::move(made));
}

Value call_window_method(Code method, const WindowHandle &which, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string what = name + "." + std::string(token::method_name_of(method));
    const int wanted = window_method_arity(method);
    if (wanted < 0) {
        context.refuse(not_built_yet, what + " is not built for a window yet");
        return Value();
    }

    // `.title` WITH NO BRACKETS READS IT BACK, and with brackets writes it. That
    // is the one method here that is both, and it is both because the author
    // wrote `my_window.title(...)` and a person who can set a thing expects to be
    // able to ask it. Every other window method is an action and wants its
    // brackets: `.close` alone would read as a thing rather than a doing.
    // `.ok` READS WITH OR WITHOUT ITS BRACKETS, because it is a QUESTION and not a
    // doing -- `f.ok` is how a file is asked the same thing
    // (SATELLITE_FILE_OPERATIONS). `.close` and `.focus` are doings and want
    // their brackets; `.ok` written bare reads exactly as what it means.
    if (method == token::ok_token) {
        satellite_window *asked = which.get();
        return Value::of_bool(asked != nullptr && asked->on_the_screen);
    }
    if (method == token::title_token && !had_parentheses) {
        satellite_window *window = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(window == nullptr ? std::string() : window->title, out, bad_offset);
        return out;
    }
    // `.pressed` WITH NO BRACKETS READS THE CAPSULE'S NAME BACK, and with
    // brackets says what it is -- the same pair as `.title`, and for the author's
    // same reason: a person who can set a thing expects to be able to ask it.
    // Empty for a button that answers nobody, which is what a button is until
    // something says otherwise.
    //
    // READ HERE ON THE INTERPRETER'S THREAD while the desk may be reading it in
    // `clicked`, and that is safe because both are READS: the only writer is
    // window_pressed(), which is this same thread going through on_the_desk().
    if (method == token::pressed_token && !had_parentheses) {
        satellite_window *button = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(button == nullptr ? std::string() : button->when_pressed, out, bad_offset);
        return out;
    }
    if (!had_parentheses && wanted == 0) {
        context.refuse(satl_line_not_understood, what + " is something a window DOES, so write it with "
                                                        "its brackets: " + what + "()");
        return Value();
    }
    if (arguments.size() != static_cast<std::size_t>(wanted)) {
        context.refuse(satl_line_not_understood, what + " takes " + std::to_string(wanted) +
                                                     (wanted == 1 ? " argument, and was given " :
                                                                    " arguments, and was given ") +
                                                     std::to_string(arguments.size()));
        return Value();
    }
    satellite_window *window = which.get();
    if (window == nullptr) {
        context.refuse(window_is_closed, what + ": there is no window here");
        return Value();
    }

    std::string why;
    bool went = false;
    switch (method) {
    case token::press_token: went = window_press(*window, why); break;
    case token::close_token: went = window_close(*window, why); break;
    case token::focus_token: went = window_focus(*window, why); break;
    case token::title_token: {
        std::string title;
        if (!text_of(arguments[0], title, what, context))
            return Value();
        went = window_set_title(*window, title, why);
        break;
    }
    case token::pressed_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_pressed(*window, capsule, why);
        break;
    }
    case token::append_token: {
        const WindowHandle *piece = arguments[0].window_handle();
        if (piece == nullptr) {
            context.refuse(types_do_not_meet, what + " takes a piece to put in the window -- "
                                                     "satellite.window.button(\"text\") makes one -- and was "
                                                     "given " + arguments[0].kind_name());
            return Value();
        }
        long long int x = 0, y = 0;
        if (!place_of(arguments[1], x, what + "'s across", context) ||
            !place_of(arguments[2], y, what + "'s down", context))
            return Value();
        went = window_append(*window, *piece, x, y, why);
        break;
    }
    default: break;
    }
    if (!went) {
        context.refuse(window_is_closed, what + " could not be done -- " + why);
        return Value();
    }
    // A WINDOW METHOD ANSWERS THE WINDOW, so they string together the way a
    // string's do: `w.title("x").focus()` is one line, and expression.cpp's
    // method loop needs no case for it.
    return Value::of_window(which);
}

void windows_hold_the_run_open(bool the_program_finished)
{
    windows_stay_open_until_closed(the_program_finished);
}

signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &run_a_capsule)
{
    APress press;
    while (the_desk_waits_for_a_press(press)) {
        const signed long long int stopped = run_a_capsule(press.capsule, press.piece, press.window);
        // A CAPSULE THAT STOPPED STOPS THE RUN, the same as a line of main
        // would have. The report is already printed by the time this answers,
        // and main() takes the windows down on a code that stops -- a person
        // told their program stopped must not be left pressing a button that
        // still looks alive.
        if (stops_the_program(stopped))
            return stopped;
    }
    return success;
}

#else

Value call_window_word(Code code, const std::vector<Value> &, ExpressionContext &context)
{
    return no_window_here(word::spelling_of(code), context);
}

Value call_window_method(Code method, const WindowHandle &, const std::vector<Value> &, bool,
                         const std::string &name, ExpressionContext &context)
{
    return no_window_here(name + "." + std::string(token::method_name_of(method)), context);
}

// NOTHING TO HOLD OPEN: no window word ever answered a window in this build.
void windows_hold_the_run_open(bool) {}

// AND NOTHING TO PRESS. No window word answered a window, so no button was made
// and no press can be waiting.
signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &)
{
    return success;
}

#endif

} // namespace satellite004
