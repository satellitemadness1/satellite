// satellite/bytecode/window_methods.cpp -- A PIECE TOLD TO DO SOMETHING:
// call_window_method, which reads the arguments and does the thing, or answers
// the question through window_questions.cpp. window_readers.hpp says how the
// six window files were cut on 2026-09-22.
//
// A WINDOW METHOD ANSWERS THE WINDOW, so they string together the way a
// string's do. Both halves -- with a window and without -- are in this file,
// and answer the same shapes.

#include "window_readers.hpp"

#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_variable_window/window_desk.hpp"

#include <string>
#include <utility>

namespace satellite004 {

using token::Code;

#if SATELLITE_HAS_WINDOW

Value call_window_method(Code method, const WindowHandle &which, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string what = name + "." + std::string(token::method_name_of(method));
    const int wanted = window_method_arity(method);
    if (wanted < 0) {
        context.refuse(not_built_yet, what + " is not built for a window yet");
        return Value();
    }

    // A QUESTION IS ANSWERED NEXT DOOR, and a refusal made there is a refusal
    // made here: context.code says so and the empty Value goes back.
    Value answered;
    if (answer_a_question(method, which, had_parentheses, name, context, answered))
        return answered;
    if (!had_parentheses && wanted == 0) {
        context.refuse(satl_line_not_understood, what + " is something a window DOES, so write it with "
                                                        "its brackets: " + what + "()");
        return Value();
    }
    if (arguments.size() != static_cast<std::size_t>(wanted) &&
        static_cast<int>(arguments.size()) != window_method_also_takes(method)) {
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
    case token::value_token: {
        // THE KIND IT TAKES IS THE PIECE'S, and the wrong one is REFUSED rather
        // than converted. A bare 50 on a progress bar could mean 50% or it could
        // mean half of one -- and a guess between those two is an answer that is
        // wrong and does not say so, so the refusal names the spelling instead.
        long long int to = 0;
        if (window->piece == satellite_window::progress) {
            const satellite_percentage *asked = arguments[0].as_percentage();
            if (asked == nullptr) {
                context.refuse(types_do_not_meet,
                               what + " takes a percentage, written with its sign on: " + name +
                                   ".value(50%) -- and was given " + arguments[0].kind_name());
                return Value();
            }
            if (!millionths_of(*asked, to)) {
                context.refuse(not_a_position,
                               what + " was given a percentage no bar can be set to");
                return Value();
            }
        } else if (!place_of(arguments[0], to, what, context, "a whole number")) {
            return Value();
        }
        went = window_set_value(*window, to, why);
        break;
    }
    case token::chosen_token: {
        std::string pick;
        if (!text_of(arguments[0], pick, what, context))
            return Value();
        went = window_set_chosen(*window, pick, why);
        break;
    }
    case token::path_token: {
        std::string shown;
        if (!text_of(arguments[0], shown, what, context))
            return Value();
        went = window_set_path(*window, shown, why);
        break;
    }
    case token::changed_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_changed(*window, capsule, why);
        break;
    }
    case token::closed_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_closed(*window, capsule, why);
        break;
    }
    case token::resize_token: {
        long long int wide = 0, tall = 0;
        if (!place_of(arguments[0], wide, what + "'s width", context) ||
            !place_of(arguments[1], tall, what + "'s height", context))
            return Value();
        went = window_resize(*window, wide, tall, why);
        break;
    }
    case token::fullscreen_token: {
        bool on = false;
        if (!on_of(arguments[0], on, what, context))
            return Value();
        went = window_set_fullscreen(*window, on, why);
        break;
    }
    case token::colour_token:
    case token::background_token: {
        std::string colour;
        if (!text_of(arguments[0], colour, what, context))
            return Value();
        went = window_set_colour(*window, colour, method == token::background_token, why);
        break;
    }
    case token::message_token: {
        std::string saying;
        if (!text_of(arguments[0], saying, what, context))
            return Value();
        went = window_message(*window, saying, why);
        break;
    }
    case token::ask_token: {
        std::string capsule, question;
        if (!text_of(arguments[0], capsule, what, context) ||
            !text_of(arguments[1], question, what + "'s question", context))
            return Value();
        went = window_ask(*window, question, capsule, why);
        break;
    }
    case token::key_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_key(*window, capsule, why);
        break;
    }
    case token::item_token: {
        std::string capsule, label;
        if (!text_of(arguments[0], capsule, what, context) ||
            !text_of(arguments[1], label, what + "'s words", context))
            return Value();
        went = window_item(*window, capsule, label, why);
        break;
    }
    case token::menu_token: {
        // THE SAME READER `.append` USES: a handle or a refusal naming the kind.
        const WindowHandle *menu = arguments[0].window_handle();
        if (menu == nullptr) {
            context.refuse(types_do_not_meet, what + " takes a menu to put across the top, or inside "
                                                  "this one -- satellite.window.menu(\"File\") makes "
                                                  "one -- and was given " + arguments[0].kind_name());
            return Value();
        }
        went = window_menu(*window, *menu, why);
        break;
    }
    case token::separator_token: went = window_separator(*window, why); break;
    // WHAT A PROGRAM DRAWS ON A CANVAS (GTK-15). Every number is a place -- a
    // whole number that may be negative and must fit a screen -- read by the
    // one reader every place here goes through; what may NOT be negative is
    // the canvas's own business and is refused there, by name.
    case token::line_token: {
        long long int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (!place_of(arguments[0], x1, what + "'s from across", context) ||
            !place_of(arguments[1], y1, what + "'s from down", context) ||
            !place_of(arguments[2], x2, what + "'s to across", context) ||
            !place_of(arguments[3], y2, what + "'s to down", context))
            return Value();
        went = window_line(*window, x1, y1, x2, y2, why);
        break;
    }
    case token::box_token: {
        long long int x = 0, y = 0, wide = 0, tall = 0;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !place_of(arguments[2], wide, what + "'s width", context) ||
            !place_of(arguments[3], tall, what + "'s height", context))
            return Value();
        went = window_box(*window, x, y, wide, tall, why);
        break;
    }
    case token::circle_token: {
        long long int x = 0, y = 0, radius = 0;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !place_of(arguments[2], radius, what + "'s radius", context))
            return Value();
        went = window_circle(*window, x, y, radius, why);
        break;
    }
    case token::arc_token: {
        long long int x = 0, y = 0, radius = 0, from = 0, to = 0;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !place_of(arguments[2], radius, what + "'s radius", context) ||
            !place_of(arguments[3], from, what + "'s from", context, "a number of degrees") ||
            !place_of(arguments[4], to, what + "'s to", context, "a number of degrees"))
            return Value();
        went = window_arc(*window, x, y, radius, from, to, why);
        break;
    }
    case token::thickness_token: {
        long long int pixels = 0;
        if (!place_of(arguments[0], pixels, what, context))
            return Value();
        went = window_set_thickness(*window, pixels, why);
        break;
    }
    case token::outline_token: {
        bool outline = false;
        if (!on_of(arguments[0], outline, what, context))
            return Value();
        went = window_set_outline(*window, outline, why);
        break;
    }
    case token::write_token: {
        long long int x = 0, y = 0;
        std::string words;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !text_of(arguments[2], words, what + "'s words", context))
            return Value();
        went = window_write(*window, x, y, words, why);
        break;
    }
    case token::clear_token: went = window_clear(*window, why); break;
    case token::save_token: {
        std::string path;
        if (!text_of(arguments[0], path, what, context))
            return Value();
        went = window_save(*window, path, why);
        // A PICTURE THAT COULD NOT BE WRITTEN GETS A FILE'S CODE (GTK-15).
        // file_unwritable is "a save could not write; the reason is said",
        // which is exactly this, and the tail below would have printed
        // types_do_not_meet under a sentence about a file -- the code and the
        // sentence disagreeing, which this module has fixed three times now.
        // Told apart by the sentence window_save wrote, the way
        // call_window_word tells "no display" apart. A closed canvas, a piece
        // that is not a canvas and an empty path still fall to the tail.
        if (!went && window->widget != nullptr && why.find("could not be written to") != std::string::npos) {
            context.refuse(file_unwritable, what + " could not be done -- " + why);
            return Value();
        }
        break;
    }
    case token::clicked_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_clicked(*window, capsule, why);
        break;
    }
    case token::every_token: {
        std::string capsule;
        long long int how_often = 0;
        if (!text_of(arguments[0], capsule, what, context) ||
            !place_of(arguments[1], how_often, what + "'s how often", context, "a number of milliseconds"))
            return Value();
        went = window_every(*window, capsule, how_often, why);
        break;
    }
    case token::font_token: {
        std::string face;
        long long int size = 0;
        if (!text_of(arguments[0], face, what + "'s face", context) ||
            !place_of(arguments[1], size, what + "'s size", context, "a size in pixels"))
            return Value();
        went = window_set_font(*window, face, size, why);
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
        // ONE ARGUMENT OR THREE, and which is right is decided by what the
        // RECEIVER turned out to be -- window_append() is what knows.
        const bool by_place = arguments.size() == 3;
        long long int x = 0, y = 0;
        if (by_place && (!place_of(arguments[1], x, what + "'s across", context) ||
                         !place_of(arguments[2], y, what + "'s down", context)))
            return Value();
        went = window_append(*window, *piece, by_place, x, y, why);
        break;
    }
    default: break;
    }
    if (!went) {
        // THE CODE FOLLOWS WHAT ACTUALLY HAPPENED, not what this tail used to
        // assume. Every doing-method could once fail for one reason -- the
        // window had gone -- and window_is_closed was the whole truth. It is not
        // any more: `a_choice.chosen("purple")` on a choice of red and green
        // fails with the window wide open, and reporting S505 under a sentence
        // saying "there is no purple to choose here" is a code and a sentence
        // disagreeing about what went wrong.
        context.refuse(window->widget == nullptr ? window_is_closed : types_do_not_meet,
                       what + " could not be done -- " + why);
        return Value();
    }
    // A WINDOW METHOD ANSWERS THE WINDOW, so they string together the way a
    // string's do: `w.title("x").focus()` is one line, and expression.cpp's
    // method loop needs no case for it.
    return Value::of_window(which);
}

#else

Value call_window_method(Code method, const WindowHandle &, const std::vector<Value> &, bool,
                         const std::string &name, ExpressionContext &context)
{
    return no_window_here(name + "." + std::string(token::method_name_of(method)), context);
}

#endif

} // namespace satellite004
