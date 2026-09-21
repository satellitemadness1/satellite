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

// ---------------------------------------------------------------------------
// EVERY WORD UNDER satellite.window, IN ONE TABLE (GTK-0's recipe).
// ---------------------------------------------------------------------------
//
// A WIDGET IS A ROW HERE AND A `Piece` NEXT DOOR, and nothing else in satl knows
// the numbers. is_window_word, the arity, the sentence a wrong argument count
// gets and `.append`'s own refusal are every one of them written FROM this
// table, so a widget added here cannot be left out of any of them -- which is
// how the hand-written list in program_check.cpp went stale in an afternoon and
// what one table is the fix for.
//
// `satellite.window` ITSELF IS NOT IN HERE. It is the family name, it answers
// nothing, and it is refused by name -- a row with no arity and no Piece would
// be a word that exists, which it is not.
struct AWord {
    unsigned int number;                  // its number under 1 27
    const char *spelling;                 // satellite.window.<this>
    std::size_t arity;
    satellite_window::Piece makes;        // what a program gets back
    const char *takes;                    // and what to say when the count is wrong
};

constexpr AWord kWords[] = {
    {1, "new", 3, satellite_window::window,
     "satellite.window.new takes a title, a width and a height: "
     "satellite.window.new(\"my window\", 800, 600)"},
    {2, "button", 1, satellite_window::button,
     "satellite.window.button takes the text on it: satellite.window.button(\"press me\")"},
    {3, "label", 1, satellite_window::label,
     "satellite.window.label takes the text it shows: satellite.window.label(\"a line of text\")"},
    {4, "text_box", 1, satellite_window::text_box,
     "satellite.window.text_box takes the text already in it, and \"\" for an empty one: "
     "satellite.window.text_box(\"\")"},
    {5, "text_area", 1, satellite_window::text_area,
     "satellite.window.text_area takes the text already in it, and \"\" for an empty one: "
     "satellite.window.text_area(\"\")"},
    {6, "checkbox", 1, satellite_window::checkbox,
     "satellite.window.checkbox takes the text beside it: satellite.window.checkbox(\"I agree\")"},
    {7, "switch", 0, satellite_window::a_switch,
     "satellite.window.switch takes nothing -- a switch says nothing, it is only on or off: "
     "satellite.window.switch()"},
};

// A LINEAR SCAN, AND IT STAYS ONE. This is asked once a window word in a
// program, not once a line, and seven rows -- eighteen one day -- is nothing
// beside the code_of() call it is comparing against.
//
// A WORD THAT TAKES NOTHING IS **TWO ROWS** IN words.tsv, and both answer the
// one row here. That is satellite.infinity's own shape -- `1 26` is the name and
// `1 26 0` is the call -- and it exists for the refusal rather than for the
// call: with only `satellite.window.switch()` registered, writing
// `satellite.window.switch("on")` matches no word at all and is refused as **"no
// capsule named switch"**, which tells a person nothing. Measured 2026-09-21;
// `satellite.window.nosuchword("on")` says exactly the same thing, which is what
// proved it was the unregistered NAME and not the switch.
const AWord *word_at(Code code)
{
    for (const AWord &row : kWords) {
        if (code == word::code_of(1, 27, row.number))
            return &row;
        if (row.arity == 0 && code == word::code_of(1, 27, row.number, 0))
            return &row;
    }
    return nullptr;
}

// THE WORDS THAT MAKE SOMETHING TO PUT IN A WINDOW, written out of the table so
// that `.append`'s refusal names the widget added this morning without anybody
// having remembered to come back here. The window is skipped: a window does not
// go inside a window.
//
// THE FIRST ONE IS SPELLED OUT AND THE REST ARE NOT -- `satellite.window.button,
// .label or .text_box` -- because this goes inside a sentence a person is
// reading at the worst moment, and eighteen full calls would be a paragraph
// where a list was wanted. One of them spelled in full is enough to show the
// shape.
std::string the_words_that_make_a_piece()
{
    std::string out;
    std::size_t left = 0;
    for (const AWord &row : kWords)
        if (row.makes != satellite_window::window)
            ++left;
    for (const AWord &row : kWords) {
        if (row.makes == satellite_window::window)
            continue;
        if (out.empty())
            out = "satellite.window." + std::string(row.spelling) + "(\"text\")";
        else
            out += (left == 1 ? " or ." : ", .") + std::string(row.spelling);
        --left;
    }
    return out;
}

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

// ON OR OFF, GOING IN. A NUMBER WHERE A BOOL IS EXPECTED IS 0 FOR OFF AND
// ANYTHING ELSE FOR ON, which is text_of's rule pointing the other way -- and it
// is not a convenience, it is the only way to write one today. SATELLITE HAS NO
// `true` AND NO `false` TO TYPE: a bool comes out of a comparison or out of
// `.ok`, and `c.on(1 < 2)` is not a sentence anybody should have to write. That
// gap is the LANGUAGE's and is written up as the author's in
// GTK_AND_NO_DEPENDENCIES.md GTK-3; accepting a number here is what makes the
// checkbox usable until he rules.
bool on_of(const Value &value, bool &out, const std::string &what, ExpressionContext &context)
{
    if (value.is_bool()) { out = value.as_bool(); return true; }
    if (const satellite_number *number = value.as_number()) { out = !number->is_zero(); return true; }
    context.refuse(types_do_not_meet, what + " takes 1 to turn it on and 0 to turn it off, and was "
                                             "given " + value.kind_name());
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

bool is_window_word(Code code) { return code == window_word() || word_at(code) != nullptr; }

std::size_t window_word_arity(Code code)
{
    const AWord *row = word_at(code);
    return row == nullptr ? 0 : row->arity;
}

std::string window_word_takes(Code code)
{
    const AWord *row = word_at(code);
    if (row != nullptr)
        return row->takes;
    return "satellite.window is the family name and is not a call -- "
           "satellite.window.new(...) makes a window";
}

// THE SENTENCE A PERSON GETS WHEN THEY ASK A WINDOW FOR SOMETHING IT HAS NO
// METHOD FOR, and it lives here rather than in program_check.cpp because this is
// the file that knows. The checker used to carry its own copy of a list like
// this and it went stale the same afternoon it was written.
std::string window_methods_are()
{
    return "a window has .append(piece, across, down), .close(), .focus(), .title(\"text\") and .ok; "
           "a piece in one has .text and, if it is a checkbox or a switch, .on -- both read "
           "bare and written with brackets; and a button has .pressed(a_capsule) and .press() "
           "(GTK_AND_NO_DEPENDENCIES.md Part 2G lists every piece and what it does)";
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
    case token::text_token:    return 1;     // written; read with no brackets (GTK-1)
    case token::on_token:      return 1;     // written; read with no brackets (GTK-3)
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
    const AWord *row = word_at(code);
    if (row == nullptr) {
        context.refuse(satl_line_not_understood, window_word_takes(code));
        return Value();
    }
    if (arguments.size() != row->arity) {
        context.refuse(satl_line_not_understood, std::string(row->takes) + " -- it was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }

    std::string why;
    const std::string called = "satellite.window." + std::string(row->spelling);

    // EVERY PIECE MADE FROM ONE LINE OF TEXT GOES THROUGH HERE, and a widget
    // added to the table above needs no branch of its own -- the Piece in its row
    // is what window_pieces.cpp turns into a GtkWidget. `new` is the one word
    // that is not this shape, and it falls past.
    if (row->makes != satellite_window::window) {
        // A PIECE'S WORD TAKES ITS WORDS OR IT TAKES NOTHING, and the table's own
        // arity is what says which -- `satellite.window.switch()` is the first
        // piece with nothing to say. A third shape one day is a third branch;
        // two do not need one.
        std::string text;
        if (row->arity == 1 && !text_of(arguments[0], text, called, context))
            return Value();
        WindowHandle made = window_piece_of_text(row->makes, text, why);
        if (made == nullptr) {
            context.refuse(no_display, called + " could not be made -- " + why);
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
    // `.text` WITH NO BRACKETS READS THE WORDS ON A PIECE, and since GTK-2 it is
    // a real ASK rather than a look at the handle: a person typing in a text box
    // changes the widget and tells satellite nothing, so window_text_of() goes
    // to the desk, copies what is there and answers that.
    //
    // AND IT CAN REFUSE, which no other bare read here can: a WINDOW has no words
    // -- its words are its title -- and being handed an empty string for one
    // would be an answer that is wrong and does not say so.
    if (method == token::text_token && !had_parentheses) {
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
    // `.on` WITH NO BRACKETS ASKS WHETHER A THING IS TURNED ON, and it is the
    // same shape as `.text`: a person clicking a checkbox changes the widget and
    // tells satellite nothing, so the answer is the widget's and not the
    // handle's. A piece that is neither on nor off is REFUSED rather than
    // answered false -- a label has no such question.
    if (method == token::on_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        bool on = false;
        std::string why;
        if (!window_on_of(*piece, on, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        return Value::of_bool(on);
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
    case token::text_token: {
        std::string text;
        if (!text_of(arguments[0], text, what, context))
            return Value();
        went = window_set_text(*window, text, why);
        break;
    }
    case token::on_token: {
        bool on = false;
        if (!on_of(arguments[0], on, what, context))
            return Value();
        went = window_set_on(*window, on, why);
        break;
    }
    case token::append_token: {
        const WindowHandle *piece = arguments[0].window_handle();
        if (piece == nullptr) {
            context.refuse(types_do_not_meet, what + " takes a piece to put in the window -- " +
                                                  the_words_that_make_a_piece() + " makes one -- and was "
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
