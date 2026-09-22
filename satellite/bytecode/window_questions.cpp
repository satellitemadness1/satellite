// satellite/bytecode/window_questions.cpp -- A PIECE ASKED SOMETHING: every
// window method read BARE, and every question that reads either way. Cut out
// of call_window_method on 2026-09-22 (window_readers.hpp says how); it was the
// first half of one 590-line function, and the seam is the one the code already
// drew -- a question answers a Value and a doing answers the window.
//
// A QUESTION READ WITH OR WITHOUT ITS BRACKETS -- `.ok`, `.width`, `.answer`,
// `.across` -- is here whatever was written; a method that is BOTH a read and a
// write -- `.title`, `.text`, `.on`, `.chosen`, `.thickness` -- is here only
// when written bare, and window_methods.cpp has its write.
//
// ANSWERED, OR REFUSED, OR NEITHER. True means `answered` is the answer, or the
// refusal has been made and context.code says so; false means this was a doing
// and the caller carries on. ONLY WHERE A WINDOW IS BUILT: a satl without one
// refuses in window_methods.cpp before it could ask.

#include "window_readers.hpp"

#include "../satellite_variable_window/window_desk.hpp"

#include <string>
#include <utility>

#if SATELLITE_HAS_WINDOW

namespace satellite004 {

bool answer_a_question(token::Code method, const WindowHandle &which, bool had_parentheses,
                       const std::string &name, ExpressionContext &context, Value &answered)
{
    const std::string what = name + "." + std::string(token::method_name_of(method));
    // `.title` WITH NO BRACKETS READS IT BACK, and with brackets writes it. That
    // is the one method here that is both, and it is both because the author
    // wrote `my_window.title(...)` and a person who can set a thing expects to be
    // able to ask it. Every other window method is an action and wants its
    // brackets: `.close` alone would read as a thing rather than a doing.
    // `.ok` READS WITH OR WITHOUT ITS BRACKETS, because it is a QUESTION and not a
    // doing -- `f.ok` is how a file is asked the same thing
    // (SATELLITE_FILE_OPERATIONS). `.close` and `.focus` are doings and want
    // their brackets; `.ok` written bare reads exactly as what it means.
    // `.answer` IS A QUESTION, so it reads with or without its brackets -- the
    // rule `.ok`, `.width` and `.height` already follow. Empty until a person
    // has answered one.
    if (method == token::answer_token) {
        satellite_window *piece = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece == nullptr ? std::string() : piece->last_answer, out, bad_offset);
        answered = out;
        return true;
    }
    if (method == token::ok_token) {
        satellite_window *asked = which.get();
        answered = Value::of_bool(asked != nullptr && asked->on_the_screen);
        return true;
    }
    // `.width` AND `.height` ARE QUESTIONS, so they read with or without their
    // brackets -- the rule `.ok` already follows. A doing wants its brackets; a
    // question written bare reads exactly as what it means.
    if (method == token::width_token || method == token::height_token) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        long long int got = 0;
        std::string why;
        if (!window_size_of(*piece, method == token::height_token, got, why)) {
            // TWO REFUSALS AND TWO CODES, as `.text` has: a closed piece is
            // window_is_closed, and a MENU -- which has no size of its own,
            // open or closed -- is a kind that does not meet (GTK-12). One code
            // for both printed S505 under a sentence that was not about closing.
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        answered = Value::of_number(satellite_number(static_cast<unsigned long long int>(got)));
        return true;
    }
    // `.across` AND `.down` ARE QUESTIONS TOO (GTK-15's leftover, 2026-09-22):
    // where the last click on this piece landed, in its own pixels, which on a
    // canvas are the pixels `.line` draws in. 0 and 0 until a click has
    // happened, as `.key` is "" until a key has. THE HANDLE'S OWN, written by
    // THIS thread off the event as `.key` is, so no desk is asked and a closed
    // piece still answers where it was last clicked.
    //
    // A BUTTON AND A MENU ARE REFUSED BY NAME (a fresh reader, 2026-09-22):
    // neither can ever be `.clicked` -- a button is pressed, a menu's items
    // are picked -- so 0 from them would not be "not yet", it would be never,
    // which is the answer that is wrong and does not say so. Everything else
    // answers 0 until a click has landed, which is only noticed on a piece
    // told `.clicked(a_capsule)`; and a press RELEASED off the piece lands
    // outside it -- negative, or past `.width` -- which is the truth of where
    // it was let go.
    if (method == token::across_token || method == token::down_token) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        if (piece->piece == satellite_window::button || !piece->is_drawn()) {
            context.refuse(types_do_not_meet,
                           what + " -- " + std::string(piece->piece_name()) +
                               (piece->piece == satellite_window::button
                                    ? " is pressed and never clicked, so no click lands on it"
                                    : " is not clicked, its items are picked") +
                               "; .across and .down are where the last click landed on a piece "
                               "told .clicked(a_capsule)");
            answered = Value();
            return true;
        }
        const long long int got = method == token::across_token ? piece->last_across : piece->last_down;
        satellite_number answer(got < 0 ? static_cast<unsigned long long int>(-got)
                                        : static_cast<unsigned long long int>(got),
                                got < 0);
        answered = Value::of_number(std::move(answer));
        return true;
    }
    // `.thickness` AND `.outline` WITH NO BRACKETS READ THE PEN BACK, and with
    // brackets set it -- `.on`'s shape, and it crosses to the desk as `.on`
    // does because the desk is the thread that holds the pen.
    if ((method == token::thickness_token || method == token::outline_token) && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        long long int got = 0;
        std::string why;
        if (!window_pen_of(*piece, method == token::thickness_token, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        if (method == token::outline_token) {
            answered = Value::of_bool(got != 0);
            return true;
        }
        answered = Value::of_number(satellite_number(static_cast<unsigned long long int>(got)));
        return true;
    }
    // `.fullscreen` WITH NO BRACKETS ASKS WHETHER IT FILLS THE SCREEN.
    if (method == token::fullscreen_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        bool got = false;
        std::string why;
        if (!window_fullscreen_of(*piece, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        answered = Value::of_bool(got);
        return true;
    }
    if (method == token::title_token && !had_parentheses) {
        satellite_window *window = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(window == nullptr ? std::string() : window->title, out, bad_offset);
        answered = out;
        return true;
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
    // `.key` READ BARE IS THE ODD ONE: it answers the LAST KEY and not the
    // capsule's name, because which key was pressed is the thing a program
    // actually wants and a capsule it wrote itself is not. Empty until one is.
    if (method == token::key_token && !had_parentheses) {
        satellite_window *piece = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece == nullptr ? std::string() : piece->last_key, out, bad_offset);
        answered = out;
        return true;
    }
    // `.typed` READ BARE IS THE LAST LINE A PERSON FINISHED IN A CONSOLE
    // (GTK-17), `.key`'s shape exactly: the desk read it off the pty, the
    // event carried it, and the interpreter wrote it onto the piece before
    // the capsule ran -- one writer. Empty until a line has been finished.
    // ONLY A CONSOLE has lines typed into it; anything else is refused by
    // name, because "" from a button would not be "not yet", it would be never.
    if (method == token::typed_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece != nullptr && piece->piece != satellite_window::console) {
            context.refuse(types_do_not_meet, what + " -- " + std::string(piece->piece_name()) +
                                                  " is not a console, and only a console has lines "
                                                  "typed into it");
            answered = Value();
            return true;
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece == nullptr ? std::string() : piece->last_typed, out, bad_offset);
        answered = out;
        return true;
    }
    // `.columns` AND `.rows` ARE QUESTIONS (GTK-17), read with or without their
    // brackets as `.width` and `.height` are: how many characters fit across a
    // console and how many lines fit down it, right now, asked of the terminal.
    if (method == token::columns_token || method == token::rows_token) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        long long int got = 0;
        std::string why;
        if (!console_cells_of(*piece, method == token::rows_token, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        answered = Value::of_number(satellite_number(static_cast<unsigned long long int>(got)));
        return true;
    }
    if ((method == token::pressed_token || method == token::changed_token ||
         method == token::closed_token || method == token::clicked_token) && !had_parentheses) {
        satellite_window *piece = which.get();
        std::string named;
        if (piece != nullptr)
            named = method == token::pressed_token   ? piece->when_pressed
                    : method == token::changed_token ? piece->when_changed
                    : method == token::clicked_token ? piece->when_clicked
                                                     : piece->when_closed;
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(named, out, bad_offset);
        answered = out;
        return true;
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
            answered = Value();
            return true;
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
            answered = Value();
            return true;
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(words, out, bad_offset);
        answered = out;
        return true;
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
            answered = Value();
            return true;
        }
        bool on = false;
        std::string why;
        if (!window_on_of(*piece, on, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        answered = Value::of_bool(on);
        return true;
    }
    // `.value` WITH NO BRACKETS ASKS WHAT NUMBER A PIECE IS AT, and what comes
    // back depends on the piece: a slider and a number box answer a NUMBER, and
    // a progress bar answers a PERCENTAGE. That is not two methods wearing one
    // name -- it is one question whose answer has the kind the piece has.
    if (method == token::value_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        long long int got = 0;
        std::string why;
        if (!window_value_of(*piece, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        if (piece->piece == satellite_window::progress) {
            answered = a_percentage_of(got);
            return true;
        }
        satellite_number answer(got < 0 ? static_cast<unsigned long long int>(-got)
                                        : static_cast<unsigned long long int>(got),
                                got < 0);
        answered = Value::of_number(std::move(answer));
        return true;
    }
    // `.path` WITH NO BRACKETS ASKS WHICH FILE A PICTURE SHOWS, and it never
    // crosses to the desk: satellite opened that file, so the path is ours and
    // stays true after the window has gone. A label's rule, not a text box's.
    if (method == token::path_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        std::string shown, why;
        if (!window_path_of(*piece, shown, why)) {
            context.refuse(types_do_not_meet, what + " -- " + why);
            answered = Value();
            return true;
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(shown, out, bad_offset);
        answered = out;
        return true;
    }
    // `.chosen` WITH NO BRACKETS ASKS WHICH ITEM IS PICKED, as TEXT. Nothing
    // picked is "" and not a refusal: a choice a person has not touched is an
    // ordinary state of a choice.
    if (method == token::chosen_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            answered = Value();
            return true;
        }
        std::string picked, why;
        if (!window_chosen_of(*piece, picked, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            answered = Value();
            return true;
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(picked, out, bad_offset);
        answered = out;
        return true;
    }
    return false;
}

} // namespace satellite004

#endif
