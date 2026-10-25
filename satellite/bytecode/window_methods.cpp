// satellite/bytecode/window_methods.cpp -- A WINDOW'S METHODS RUN BY THEIR NUMBERS (NM-8, 2026-10-11): which
// method runs which number, what a window refuses before any number runs, and the steps every number shares.
// window_readers.hpp says how the window files were cut on 2026-09-22.
//
// THE METHODS THEMSELVES ARE THEIR NUMBERS' FILES SINCE THAT DAY -- satellite-numbers/words/1.6.18.<n>.cpp, each
// moved there whole from the two halves that ran it: call_window_method, which was this file (a piece TOLD to do
// something), and answer_a_question, which was window_questions.cpp beside it (a piece ASKED something -- every
// method read bare, and every question read either way). The author, 2026-10-10: "we have to give everything a
// number and then make it run those numbers ... and it actually runs that number when we do it".
//
// A WINDOW METHOD ANSWERS THE WINDOW, so they string together the way a string's do. Both halves -- with a window
// and without -- are in this file, and answer the same shapes.

#include "window_readers.hpp"
#include "word_codes.hpp"
#include "../machine/thread_stop.hpp"
#include "../../satellite-numbers/number_row.hpp"

#include <string>
#include <utility>

namespace satellite004 {

using token::Code;

// EVERY NUMBER A WINDOW'S METHOD RUNS IS IN words/words.tsv -- said when satl is compiled, so a number that is
// missing is a build that stops and not a method that answers "not built" (string_calls.cpp's
// every_string_word_numbered). 48, 49 and 54 are not here: they are scroll, widgets and window, the first parts of
// the words of parts under them, which run nothing of their own. NOR ARE 49 1 1, 49 1 2 AND 52 SINCE SEC-0
// (SATELLITE_WINDOW_SECTIONS.md rule 8): widgets.display.file, widgets.display.input and section, the old spellings,
// are refused before anything runs, and their code is 54 2 1's, 54 2 2 1's and 54 1's now. Their rows stay in
// words.tsv, because a row taken out would move the code of every word after it (make_words.py).
template <int... Last>
inline constexpr bool every_window_word_numbered = ((word::fixed_code<1, 6, 18, Last> != 0) && ...);
static_assert(every_window_word_numbered<1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                         22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                         41, 42, 43, 44, 45, 46, 47, 50, 51, 53> &&
                  word::fixed_code<1, 6, 18, 48, 1> != 0 && word::fixed_code<1, 6, 18, 48, 2> != 0 &&
                  word::fixed_code<1, 6, 18, 54, 1> != 0 && word::fixed_code<1, 6, 18, 54, 2, 1> != 0 &&
                  word::fixed_code<1, 6, 18, 54, 2, 2, 1> != 0,
              "a window method's number is missing from words/words.tsv");

token::Code window_number_of(token::Code method)
{
    // IN window_method_arity's ORDER (window_shapes.cpp), which is the order a window's methods were built in,
    // and so the order they were numbered in. colour_token is .colour and .color both -- one code, one number.
    switch (method) {
    case token::append_token: return word::fixed_code<1, 6, 18, 1>;
    case token::open_token: return word::fixed_code<1, 6, 18, 2>;
    case token::close_token: return word::fixed_code<1, 6, 18, 3>;
    case token::focus_token: return word::fixed_code<1, 6, 18, 4>;
    case token::title_token: return word::fixed_code<1, 6, 18, 5>;
    case token::pressed_token: return word::fixed_code<1, 6, 18, 6>;
    case token::press_token: return word::fixed_code<1, 6, 18, 7>;
    case token::text_token: return word::fixed_code<1, 6, 18, 8>;
    case token::on_token: return word::fixed_code<1, 6, 18, 9>;
    case token::value_token: return word::fixed_code<1, 6, 18, 10>;
    case token::chosen_token: return word::fixed_code<1, 6, 18, 11>;
    case token::path_token: return word::fixed_code<1, 6, 18, 12>;
    case token::changed_token: return word::fixed_code<1, 6, 18, 13>;
    case token::closed_token: return word::fixed_code<1, 6, 18, 14>;
    case token::resize_token: return word::fixed_code<1, 6, 18, 15>;
    case token::width_token: return word::fixed_code<1, 6, 18, 16>;
    case token::height_token: return word::fixed_code<1, 6, 18, 17>;
    case token::fullscreen_token: return word::fixed_code<1, 6, 18, 18>;
    case token::colour_token: return word::fixed_code<1, 6, 18, 19>;
    case token::background_token: return word::fixed_code<1, 6, 18, 20>;
    case token::font_token: return word::fixed_code<1, 6, 18, 21>;
    case token::every_token: return word::fixed_code<1, 6, 18, 22>;
    case token::key_token: return word::fixed_code<1, 6, 18, 23>;
    case token::clicked_token: return word::fixed_code<1, 6, 18, 24>;
    case token::message_token: return word::fixed_code<1, 6, 18, 25>;
    case token::ask_token: return word::fixed_code<1, 6, 18, 26>;
    case token::choose_a_file_token: return word::fixed_code<1, 6, 18, 27>;
    case token::answer_token: return word::fixed_code<1, 6, 18, 28>;
    case token::menu_token: return word::fixed_code<1, 6, 18, 29>;
    case token::item_token: return word::fixed_code<1, 6, 18, 30>;
    case token::separator_token: return word::fixed_code<1, 6, 18, 31>;
    case token::line_token: return word::fixed_code<1, 6, 18, 32>;
    case token::box_token: return word::fixed_code<1, 6, 18, 33>;
    case token::circle_token: return word::fixed_code<1, 6, 18, 34>;
    case token::write_token: return word::fixed_code<1, 6, 18, 35>;
    case token::arc_token: return word::fixed_code<1, 6, 18, 36>;
    case token::thickness_token: return word::fixed_code<1, 6, 18, 37>;
    case token::outline_token: return word::fixed_code<1, 6, 18, 38>;
    case token::across_token: return word::fixed_code<1, 6, 18, 39>;
    case token::down_token: return word::fixed_code<1, 6, 18, 40>;
    case token::clear_token: return word::fixed_code<1, 6, 18, 41>;
    case token::save_token: return word::fixed_code<1, 6, 18, 42>;
    case token::display_token: return word::fixed_code<1, 6, 18, 43>;
    case token::typed_token: return word::fixed_code<1, 6, 18, 44>;
    case token::home_token: return word::fixed_code<1, 6, 18, 45>;
    case token::columns_token: return word::fixed_code<1, 6, 18, 46>;
    case token::rows_token: return word::fixed_code<1, 6, 18, 47>;
    case token::scroll_vertical_token: return word::fixed_code<1, 6, 18, 48, 1>;
    case token::scroll_horizontal_token: return word::fixed_code<1, 6, 18, 48, 2>;
    case token::directory_token: return word::fixed_code<1, 6, 18, 50>;
    case token::listen_token: return word::fixed_code<1, 6, 18, 51>;
    case token::ok_token: return word::fixed_code<1, 6, 18, 53>;
    // SEC-0 (SATELLITE_WINDOW_SECTIONS.md): a window's sections and displays are under satellite.window. The old
    // spellings run no number -- widgets_display_file_token, widgets_display_input_token and section_token, which is
    // an object's own capsule's name as well -- and are refused before the period's rule by the parts rule
    // (window_shapes.cpp's a_word_in_parts), naming these.
    case token::window_section_token: return word::fixed_code<1, 6, 18, 54, 1>;
    case token::window_display_file_token: return word::fixed_code<1, 6, 18, 54, 2, 1>;
    case token::window_display_text_input_token: return word::fixed_code<1, 6, 18, 54, 2, 2, 1>;
    default: return 0;
    }
}

#if SATELLITE_HAS_WINDOW

Value run_window_number(Code method, Code number, const Value &receiver, const std::vector<Value> &arguments,
                        bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    // A WINDOW IS THE MAIN THREAD'S (THREADS.md T3), however a thread came to hold one --
    // inside an object it shares, since objects are shared (the second review, 2026-09-24).
    if (on_a_program_thread()) {
        context.refuse(thread_cannot_share_yet, "a window belongs to the main thread, and a thread the program "
                                                "started may not use one yet");
        return Value();
    }
    // A METHOD NO WINDOW HAS -- written with a plain period, since `.satellite.` before one is refused before
    // this -- is said as it always was.
    if (number == 0) {
        context.refuse(not_built_yet, name + "." + std::string(token::method_name_of(method)) +
                                          " is not built for a window yet");
        return Value();
    }
    // THE NUMBER RUNS: table[code], one subtraction and one load (function_table.hpp), and the function its file
    // under satellite-numbers/words/ put in the row at start-up.
    const NumberRow *row = context.functions.method(number);
    if (row == nullptr || row->scenarios.method_call == nullptr) {
        context.refuse(not_built_yet, name + ".satellite." + std::string(token::method_name_of(method)) +
                                          " is not built for a window yet");
        return Value();
    }
    return row->scenarios.method_call(receiver, nullptr, nullptr, arguments, had_parentheses, name, context);
}

namespace window_method_parts {

satellite_window *ready_to_do(Code method, const WindowHandle &which, const std::vector<Value> &arguments,
                              bool had_parentheses, const std::string &what, ExpressionContext &context)
{
    const int wanted = window_method_arity(method);
    if (!had_parentheses && wanted == 0) {
        context.refuse(satl_line_not_understood, what + " is something a window DOES, so write it with "
                                                        "its brackets: " + what + "()");
        return nullptr;
    }
    if (arguments.size() != static_cast<std::size_t>(wanted) &&
        static_cast<int>(arguments.size()) != window_method_also_takes(method)) {
        context.refuse(satl_line_not_understood, what + " takes " + window_method_takes(method) +
                                                     ", and was given " + std::to_string(arguments.size()));
        return nullptr;
    }
    satellite_window *window = which.get();
    if (window == nullptr) {
        context.refuse(window_is_closed, what + ": there is no window here");
        return nullptr;
    }
    return window;
}

Value done(bool went, const std::string &why, const satellite_window &window, const WindowHandle &which,
           const std::string &what, ExpressionContext &context)
{
    if (!went) {
        // THE CODE FOLLOWS WHAT ACTUALLY HAPPENED, not what this tail used to
        // assume. Every doing-method could once fail for one reason -- the
        // window had gone -- and window_is_closed was the whole truth. It is not
        // any more: `a_choice.chosen("purple")` on a choice of red and green
        // fails with the window wide open, and reporting S505 under a sentence
        // saying "there is no purple to choose here" is a code and a sentence
        // disagreeing about what went wrong.
        context.refuse(window.widget == nullptr ? window_is_closed : types_do_not_meet,
                       what + " could not be done -- " + why);
        return Value();
    }
    // A WINDOW METHOD ANSWERS THE WINDOW, so they string together the way a
    // string's do: `w.title("x").focus()` is one line, and expression.cpp's
    // method loop needs no case for it.
    return Value::of_window(which);
}

// WHAT HOLDS A SECTION, IN PIXELS, ACROSS OR DOWN: a window's inside as it was asked for -- new's size, or the last
// `.resize` (SATELLITE_WINDOW_SECTIONS.md rule 3: a window's size is its inside) -- and a section's own share of its
// holder, cut down as the layout cuts it (window_widgets.cpp's pixels_of), or its pixels. 0 when it is neither, and
// then nothing is judged here: window_widget_made refuses a section anywhere else.
long long int pixels_held_by(const satellite_window &holder, bool tall)
{
    if (holder.is_a_window())
        return tall ? holder.asked_tall : holder.asked_wide;
    if (holder.piece != satellite_window::a_section)
        return 0;
    const AnExtent &extent = tall ? holder.size.tall : holder.size.wide;
    if (!extent.share)
        return extent.amount;
    const WindowHandle above = holder.inside_of.lock();
    return above == nullptr ? 0 : pixels_held_by(*above, tall) * extent.amount / 1000000;
}

// PIXELS FROM 1 TO WHAT HOLDS THEM (the author, 2026-10-20: "from 0pixels to the width/height of the window"; rule 2,
// SEC-1): judged when the line runs, because only then is the holder known. A share needs nothing here -- at most all
// of its holder, extent_of saw to that.
bool pixels_fit_the_holder(const satellite_window &holder, const AnExtent &extent, bool tall, const std::string &what,
                           ExpressionContext &context)
{
    if (extent.share)
        return true;
    const long long int room = pixels_held_by(holder, tall);
    if (room <= 0 || extent.amount <= room)
        return true;
    context.refuse(not_a_position, what + " was given " + std::to_string(extent.amount) + " pixels, and what holds it is " +
                                       std::to_string(room) + (tall ? " tall" : " wide") + " -- up to " +
                                       std::to_string(room) + " pixels, or a share of it, 50% or 1/2");
    return false;
}

// THE AUTHOR'S DISPLAYS (2026-10-09): `.satellite.window.display.file("a_name")` and
// `.satellite.window.display.text.input("a_name")` make one in this window, section or file display -- and ANSWER
// IT, so a chain goes on with the display made: files.satellite.window.display.text.input("folder").
//
// AND `.satellite.window.section("a_name", 100%, 50%)`, which always has a size: a share of what holds it, or pixels
// (the author, 2026-10-10: "we can accept plain numbers as pixels, and percentages get converted into pixels").
// window_widgets.cpp lays it out. A display takes no size since SEC-0: it fills its section (the author, 2026-10-25:
// "a display always fills the section"), so only a section's line has three arguments.
//
// ITS NAME IS A NAME (SATELLITE_WINDOW_SECTIONS.md SEC-0, rule 4; the author, 2026-10-25, of every display: "always
// take a name, because what if the user wants to change an image that is displayed in the image display?"): the
// line declares it in the running capsule's own table, holding what it made, so the lines after it write it bare --
// `top.satellite.window.display.file("files")` -- as they write a `satellite.variable.window` they declared, and
// `main_window.top` still reaches it through the window. declare() lists it for its block's }, as every name a
// line declares inside a block is (value.hpp), so it ends where a declaration ends. The checker refused before
// anything ran a name this capsule had already (program_check.cpp's the_names_a_line_makes), so nothing is
// replaced here that a line could still mean.
Value a_piece_made(satellite_window::Piece kind, satellite_window &window, const std::vector<Value> &arguments,
                   const std::string &what, ExpressionContext &context)
{
    std::string named;
    if (!text_of(arguments[0], named, what + "'s name", context))
        return Value();
    ASize size;
    const bool sized = arguments.size() == 3;
    if (sized && (!extent_of(arguments[1], size.wide, named + "'s width", context) ||
                  !extent_of(arguments[2], size.tall, named + "'s height", context) ||
                  !pixels_fit_the_holder(window, size.wide, false, named + "'s width", context) ||
                  !pixels_fit_the_holder(window, size.tall, true, named + "'s height", context)))
        return Value();
    std::string why;
    WindowHandle made = window_widget_made(window, kind, named, sized ? &size : nullptr, why);
    if (made == nullptr) {
        context.refuse(window.widget == nullptr ? window_is_closed : satl_line_not_understood,
                       what + " could not be done -- " + why);
        return Value();
    }
    constexpr Code kAWindow = word::fixed_code<1, 6, 18>;
    context.variables.declare(named, kAWindow, plain_shape(kAWindow), Value::of_window(made));
    return Value::of_window(std::move(made));
}

} // namespace window_method_parts

Value a_widget_by_name(const WindowHandle &which, const std::string &member, const std::string &name,
                       ExpressionContext &context)
{
    if (on_a_program_thread()) {
        context.refuse(thread_cannot_share_yet, "a window belongs to the main thread, and a thread the program "
                                                "started may not use one yet");
        return Value();
    }
    const satellite_window *owner = which.get();
    std::string why;
    WindowHandle found = owner != nullptr ? window_widget_named(*owner, member, why) : nullptr;
    if (found == nullptr) {
        // A NAME NOBODY MADE IS name_not_declared, as a variable's is; ONE MADE IN A WINDOW THAT HAS
        // CLOSED since is window_is_closed -- the widget closed with it.
        context.refuse(owner == nullptr ? window_is_closed : name_not_declared,
                       name + "." + member + " -- " + (owner == nullptr ? std::string("there is no window here") : why));
        return Value();
    }
    return Value::of_window(std::move(found));
}

#else

Value run_window_number(Code method, Code number, const Value &, const std::vector<Value> &, bool,
                        const std::string &name, ExpressionContext &context)
{
    return no_window_here(name + (number != 0 ? ".satellite." : ".") + std::string(token::method_name_of(method)),
                          context);
}

namespace window_method_parts {

Value no_window(const Value &, Value *, const TypeShape *, const std::vector<Value> &, bool, const std::string &name,
                ExpressionContext &context)
{
    return no_window_here(name, context);
}

} // namespace window_method_parts

Value a_widget_by_name(const WindowHandle &, const std::string &member, const std::string &name,
                       ExpressionContext &context)
{
    return no_window_here(name + "." + member, context);
}

#endif

} // namespace satellite004
